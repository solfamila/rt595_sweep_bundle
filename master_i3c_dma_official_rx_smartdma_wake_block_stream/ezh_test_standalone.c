/*
 * Minimal RT595 master experiment that preserves the official classic DMA I3C
 * RX proof while issuing a small request write for each block and routing the
 * following bounded official DMA read completion to SmartDMA with zero CM33
 * payload IRQ service.
 */

#include "fsl_common.h"
#include "fsl_inputmux.h"
#include "fsl_power.h"
#include "fsl_reset.h"
#include "fsl_smartdma.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "board.h"
#include "experiment_led.h"
#include "fsl_debug_console.h"
#include "fsl_i3c_dma.h"

#define EXAMPLE_DMA DMA0
#define EXAMPLE_I3C_RX_CHANNEL 24U
#define EXAMPLE_I3C_TX_CHANNEL 25U

#define I3C_MASTER_SLAVE_ADDR_7BIT 0x1EU
#define I3C_STREAM_REQUEST_TOKEN 0x53U
#ifndef I3C_STREAM_BLOCK_BYTES
#define I3C_STREAM_BLOCK_BYTES 240U
#endif
#ifndef I3C_STREAM_TOTAL_BYTES
#ifndef I3C_STREAM_BLOCK_COUNT
#define I3C_STREAM_BLOCK_COUNT 4U
#endif
#define I3C_STREAM_TOTAL_BYTES (I3C_STREAM_BLOCK_BYTES * I3C_STREAM_BLOCK_COUNT)
#else
#ifndef I3C_STREAM_BLOCK_COUNT
#define I3C_STREAM_BLOCK_COUNT ((I3C_STREAM_TOTAL_BYTES + I3C_STREAM_BLOCK_BYTES - 1U) / I3C_STREAM_BLOCK_BYTES)
#endif
#endif
#define I3C_DATA_LENGTH I3C_STREAM_BLOCK_BYTES
#define I3C_LOGICAL_TOTAL_BYTES I3C_STREAM_TOTAL_BYTES
#define I3C_LOGICAL_CHUNK_COUNT I3C_STREAM_BLOCK_COUNT
#define I3C_LOGICAL_DATA_LENGTH I3C_STREAM_TOTAL_BYTES
#define I3C_PACKET_LENGTH 2U
#if (I3C_STREAM_BLOCK_BYTES == 0U) || (I3C_STREAM_BLOCK_BYTES > 255U)
#error "I3C_STREAM_BLOCK_BYTES must be in the range 1..255"
#endif
#define I3C_DMA_OFFICIAL_TIMEOUT 100000000U
#define I3C_DMA_OFFICIAL_LED_PULSE_US 120000U
#define I3C_SLAVE_SESSION_RESET_TOKEN 0xFFU
#ifndef I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US
#define I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US 10000U
#endif
#define I3C_DMA_WAKE_API_INDEX 0U
#define I3C_DMA_RX_SEED_TAIL_API_INDEX 2U
#define I3C_DMA_WAKE_MAILBOX_COMPLETE 1U
#define I3C_DMA_SEED_TAIL_IBI_PRE_READ_DELAY_US 10U
#define SMART_DMA_TRIGGER_CHANNEL 0U

#ifndef EXPERIMENT_ALLOW_CM33_DMA_IRQ
#define EXPERIMENT_ALLOW_CM33_DMA_IRQ 0U
#endif

#ifndef EXPERIMENT_DISABLE_RX_TAIL_RECOVERY
#define EXPERIMENT_DISABLE_RX_TAIL_RECOVERY 0U
#endif

#ifndef EXPERIMENT_IBI_GENERATION_TAG
#define EXPERIMENT_IBI_GENERATION_TAG 0U
#endif

#define I3C_MASTER_PROTOCOL_CLEAR_MASK ((uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag | \
                                        (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag | \
                                        (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag)

#define I3C_MASTER_ERROR_CLEAR_MASK ((uint32_t)kI3C_MasterErrorNackFlag | (uint32_t)kI3C_MasterErrorWriteAbortFlag | \
                                     (uint32_t)kI3C_MasterErrorTermFlag | (uint32_t)kI3C_MasterErrorParityFlag | \
                                     (uint32_t)kI3C_MasterErrorCrcFlag | (uint32_t)kI3C_MasterErrorReadFlag | \
                                     (uint32_t)kI3C_MasterErrorWriteFlag | (uint32_t)kI3C_MasterErrorMsgFlag | \
                                     (uint32_t)kI3C_MasterErrorInvalidReqFlag | (uint32_t)kI3C_MasterErrorTimeoutFlag)

enum
{
    kDmaOfficialStageReset = 0U,
    kDmaOfficialStageInit = 1U,
    kDmaOfficialStageMasterReady = 2U,
    kDmaOfficialStageDaaCccDone = 3U,
    kDmaOfficialStageDaaDone = 4U,
    kDmaOfficialStageIbiRegistered = 5U,
    kDmaOfficialStageWriteDone = 6U,
    kDmaOfficialStageIbiSeen = 7U,
    kDmaOfficialStageSmartDmaWakeArmed = 8U,
    kDmaOfficialStageReadDone = 9U,
    kDmaOfficialStageSmartDmaWakeSeen = 10U,
    kDmaOfficialStageValidated = 11U,
};

enum
{
    kDmaOfficialOutcomeNone = 0U,
    kDmaOfficialOutcomeSuccess = 1U,
    kDmaOfficialOutcomeFailure = 2U,
};

enum
{
    kDmaOfficialResultSuccess = 0,
    kDmaOfficialResultTimeoutCompletion = -1,
    kDmaOfficialResultTimeoutIbi = -2,
    kDmaOfficialResultNoSlaveAddr = -3,
    kDmaOfficialResultDataMismatch = -4,
    kDmaOfficialResultTimeoutSmartDmaWake = -5,
    kDmaOfficialResultUnexpectedSmartDmaWake = -6,
    kDmaOfficialResultUnexpectedChunkLength = -7,
    kDmaOfficialResultCm33DmaIrqObserved = -8,
    kDmaOfficialResultCm33DataIrqObserved = -9,
    kDmaOfficialResultCm33RxCallbackObserved = -10,
    kDmaOfficialResultUnexpectedIbiGeneration = -11,
};

enum
{
    kI3cDmaStateWaitForCompletion = 6U,
};

typedef struct _i3c_dma_smartdma_wake_param
{
    volatile uint32_t mailbox;
    volatile uint32_t wakeCount;
    volatile uint32_t dmaIntaCount;
    volatile uint32_t dmaIntaSnapshot;
    volatile uint32_t i3cMdmaCtrlSnapshot;
    volatile uint32_t i3cMstatusSnapshot;
    volatile uint32_t i3cMdataCtrlSnapshot;
    uint32_t i3cBaseAddress;
    uint32_t dmaIntaAddress;
} i3c_dma_smartdma_wake_param_t;

typedef struct _i3c_dma_seed_tail_read_param
{
    volatile uint32_t mailbox;
    uint32_t expectedWakeCount;
    volatile uint32_t wakeCount;
    volatile uint32_t dmaSeedBytes;
    volatile uint32_t smartdmaBytes;
    volatile uint32_t dmaIntaCount;
    uint32_t nextByteAddress;
    uint32_t remainingCount;
    uint32_t i3cBaseAddress;
    uint32_t dmaIntaAddress;
    uint32_t dmaChannelMask;
} i3c_dma_seed_tail_read_param_t;

extern uint8_t __smartdma_start__[];
extern uint8_t __smartdma_end__[];

void keep_smartdma_api_alive(void);

extern volatile uint32_t g_dma0_dbg_irq_count;
extern volatile uint32_t g_dma0_dbg_first_intstat;
extern volatile uint32_t g_dma0_dbg_first_inta;
extern volatile uint32_t g_dma0_dbg_first_active;
extern volatile uint32_t g_dma0_dbg_last_intstat;
extern volatile uint32_t g_dma0_dbg_last_inta;
extern volatile uint32_t g_dma0_dbg_last_active;
extern volatile uint32_t g_i3c_dbg_irq_count;
extern volatile uint32_t g_i3c_dbg_irq_data_count;
extern volatile uint32_t g_i3c_dbg_irq_protocol_count;
extern volatile uint32_t g_i3c_dbg_irq_last_pending;
extern volatile uint32_t g_i3c_dbg_irq_first_data_pending;
extern volatile uint32_t g_i3c_dbg_dma_callback_rx_count;

static __NO_INIT volatile uint32_t s_dma_official_stage;
static __NO_INIT volatile uint32_t s_dma_official_outcome;
static __NO_INIT volatile int32_t s_dma_official_result;
static __NO_INIT volatile uint32_t s_dma_official_completion_status;
static __NO_INIT volatile uint32_t s_dma_official_slave_addr;
static __NO_INIT volatile uint32_t s_dma_official_dev_count;
static __NO_INIT volatile uint32_t s_dma_official_expected_chunk_count;
static __NO_INIT volatile uint32_t s_dma_official_completed_chunk_count;
static __NO_INIT volatile uint32_t s_dma_official_current_chunk_index;
static __NO_INIT volatile uint32_t s_dma_official_ibi_payload_size;
static __NO_INIT volatile uint32_t s_dma_official_ibi_data0;
static __NO_INIT volatile uint32_t s_dma_official_ibi_data1;
static __NO_INIT volatile uint32_t s_dma_official_rx_size;
static __NO_INIT volatile uint32_t s_dma_official_mismatch_index;
static __NO_INIT volatile uint32_t s_dma_official_rx_first;
static __NO_INIT volatile uint32_t s_dma_official_rx_last;
static __NO_INIT volatile uint32_t s_dma_official_tail_recovery_count;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mailbox;
static __NO_INIT volatile uint32_t s_dma_official_total_smartdma_wake_count;
static __NO_INIT volatile uint32_t s_dma_official_total_smartdma_dma_inta_count;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_dma_inta_snapshot;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mdmactrl;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mstatus;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mdatactrl;
static __NO_INIT volatile uint32_t s_dma_official_total_chunk_time_us;
static __NO_INIT volatile uint32_t s_dma_official_total_write_wait_us;
static __NO_INIT volatile uint32_t s_dma_official_total_ibi_wait_us;
static __NO_INIT volatile uint32_t s_dma_official_total_read_wait_us;
static __NO_INIT volatile uint32_t s_dma_official_total_smartdma_arm_us;
static __NO_INIT volatile uint32_t s_dma_official_total_smartdma_wait_us;
static __NO_INIT volatile uint32_t s_dma_official_total_dma0_irq_count;
static __NO_INIT volatile uint32_t s_dma_official_total_data_irq_count;
static __NO_INIT volatile uint32_t s_dma_official_total_protocol_irq_count;
static __NO_INIT volatile uint32_t s_dma_official_total_rx_dma_callback_count;
static __NO_INIT volatile uint8_t s_dma_official_rx_snapshot[I3C_LOGICAL_DATA_LENGTH];

AT_NONCACHEABLE_SECTION_ALIGN(static i3c_dma_smartdma_wake_param_t s_smartdma_wake_param, 4);
AT_NONCACHEABLE_SECTION_ALIGN(static i3c_dma_seed_tail_read_param_t s_rx_seed_read_param, 4);

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_master_txBuff[I3C_PACKET_LENGTH], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_chunk_rxBuff[I3C_DATA_LENGTH], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_master_rxBuff[I3C_LOGICAL_DATA_LENGTH], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_master_ibiBuff[10U], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_ibiBuff[10U], 4);
static uint8_t g_ibiPayloadSize;
static i3c_master_dma_handle_t g_i3cMasterHandle;
static dma_handle_t g_txDmaHandle;
static dma_handle_t g_rxDmaHandle;
static dma_handle_t g_seedReadDmaHandle;
static volatile bool g_masterCompletionFlag;
static volatile bool g_ibiWonFlag;
static volatile status_t g_completionStatus = kStatus_Success;
static bool g_smartdmaWakeProbeInitialized;

static void i3c_master_ibi_callback(I3C_Type *base,
                                    i3c_master_dma_handle_t *handle,
                                    i3c_ibi_type_t ibiType,
                                    i3c_ibi_state_t ibiState);
static void i3c_master_callback(I3C_Type *base,
                                i3c_master_dma_handle_t *handle,
                                status_t status,
                                void *userData);
static uint32_t block_stream_dma_seed_byte_count(uint32_t totalSize);
static i3c_rx_trigger_level_t block_stream_dma_seed_trigger_level(uint32_t seedByteCount);
static void clear_dma0_channel_state(void);
static status_t wait_for_post_ibi_read_ctrl_done(I3C_Type *base);
static status_t wait_for_post_ibi_read_complete(I3C_Type *base);
static status_t wait_for_seed_tail_mailbox(I3C_Type *base);
static status_t wait_for_master_idle_simple(I3C_Type *base);
static status_t quiesce_post_ibi_to_idle(I3C_Type *base);
static status_t run_chunk_seed_tail_read(uint8_t slaveAddr, uint32_t expectedReadSize);
static void arm_smartdma_wake_probe(void);
static status_t wait_for_smartdma_wake(uint32_t timeout);
static void suppress_read_data_irq_bounce(void);
static void scrub_master_chunk_boundary_state(void);

static void rearm_smartdma_wake_probe(void)
{
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    SMARTDMA->CTRL = 0xC0DE0000U | (1U << 4U);
}

static inline void init_cycle_counter(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;
}

static inline uint32_t read_cycle_counter(void)
{
    return DWT->CYCCNT;
}

static inline uint32_t cycles_to_us(uint32_t elapsedCycles)
{
    return (uint32_t)(((uint64_t)elapsedCycles * 1000000ULL) / SystemCoreClock);
}

static const i3c_master_dma_callback_t s_masterCallback = {
    .slave2Master = NULL,
    .ibiCallback = i3c_master_ibi_callback,
    .transferComplete = i3c_master_callback,
};

static void init_transfer_led(void)
{
    EXP_LED_Init();
}

static void run_boot_led_self_test(void)
{
    EXP_LED_Blink(false, false, true, 2U, I3C_DMA_OFFICIAL_LED_PULSE_US);
    EXP_LED_Set(false, false, true);
}

static void set_success_led(void)
{
    EXP_LED_Blink(false, true, false, 3U, I3C_DMA_OFFICIAL_LED_PULSE_US);
    EXP_LED_Set(false, true, false);
}

static void set_failure_led(void)
{
    EXP_LED_Set(true, false, false);
}

static void reset_retained_state(void)
{
    s_dma_official_stage = kDmaOfficialStageReset;
    s_dma_official_outcome = kDmaOfficialOutcomeNone;
    s_dma_official_result = 0;
    s_dma_official_completion_status = (uint32_t)kStatus_Success;
    s_dma_official_slave_addr = 0U;
    s_dma_official_dev_count = 0U;
    s_dma_official_expected_chunk_count = I3C_LOGICAL_CHUNK_COUNT;
    s_dma_official_completed_chunk_count = 0U;
    s_dma_official_current_chunk_index = 0U;
    s_dma_official_ibi_payload_size = 0U;
    s_dma_official_ibi_data0 = 0U;
    s_dma_official_ibi_data1 = 0U;
    s_dma_official_rx_size = 0U;
    s_dma_official_mismatch_index = 0xFFFFFFFFU;
    s_dma_official_rx_first = 0U;
    s_dma_official_rx_last = 0U;
    s_dma_official_tail_recovery_count = 0U;
    s_dma_official_smartdma_mailbox = 0U;
    s_dma_official_total_smartdma_wake_count = 0U;
    s_dma_official_total_smartdma_dma_inta_count = 0U;
    s_dma_official_smartdma_dma_inta_snapshot = 0U;
    s_dma_official_smartdma_mdmactrl = 0U;
    s_dma_official_smartdma_mstatus = 0U;
    s_dma_official_smartdma_mdatactrl = 0U;
    s_dma_official_total_chunk_time_us = 0U;
    s_dma_official_total_write_wait_us = 0U;
    s_dma_official_total_ibi_wait_us = 0U;
    s_dma_official_total_read_wait_us = 0U;
    s_dma_official_total_smartdma_arm_us = 0U;
    s_dma_official_total_smartdma_wait_us = 0U;
    s_dma_official_total_dma0_irq_count = 0U;
    s_dma_official_total_data_irq_count = 0U;
    s_dma_official_total_protocol_irq_count = 0U;
    s_dma_official_total_rx_dma_callback_count = 0U;
    memset((void *)s_dma_official_rx_snapshot, 0, sizeof(s_dma_official_rx_snapshot));
    memset((void *)&s_smartdma_wake_param, 0, sizeof(s_smartdma_wake_param));
}

static void mark_failure(uint32_t stage, int32_t result)
{
    s_dma_official_stage = stage;
    s_dma_official_outcome = kDmaOfficialOutcomeFailure;
    s_dma_official_result = result;
}

static status_t wait_for_transfer_complete(uint32_t timeout)
{
    while (!g_masterCompletionFlag)
    {
        if (g_completionStatus != kStatus_Success)
        {
            return g_completionStatus;
        }

        if (timeout == 0U)
        {
            return kStatus_Timeout;
        }

        timeout--;
    }

    g_masterCompletionFlag = false;
    return g_completionStatus;
}

static status_t wait_for_ibi(uint32_t timeout)
{
    while (!g_ibiWonFlag)
    {
        if ((g_completionStatus != kStatus_Success) && (g_completionStatus != kStatus_I3C_IBIWon))
        {
            return g_completionStatus;
        }

        if (timeout == 0U)
        {
            return kStatus_Timeout;
        }

        timeout--;
    }

    g_ibiWonFlag = false;
    g_completionStatus = kStatus_Success;
    return kStatus_Success;
}

static uint32_t recover_master_rx_tail(uint8_t *rxBuff, uint32_t startIndex, uint32_t totalSize)
{
    uint32_t writeIndex = startIndex;
    uint32_t timeout = I3C_DMA_OFFICIAL_TIMEOUT;

    while ((writeIndex < totalSize) && (timeout != 0U))
    {
        uint32_t rxCount =
            (EXAMPLE_MASTER->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;

        while ((rxCount != 0U) && (writeIndex < totalSize))
        {
            rxBuff[writeIndex++] = (uint8_t)(EXAMPLE_MASTER->MRDATAB & I3C_MRDATAB_VALUE_MASK);
            rxCount--;
        }

        if (writeIndex >= totalSize)
        {
            break;
        }

        if ((I3C_MasterGetStatusFlags(EXAMPLE_MASTER) & (uint32_t)kI3C_MasterRxReadyFlag) == 0U)
        {
            timeout--;
        }
    }

    return writeIndex - startIndex;
}

static uint32_t block_stream_dma_seed_byte_count(uint32_t totalSize)
{
    if (totalSize >= 6U)
    {
        return 6U;
    }

    if (totalSize >= 4U)
    {
        return 4U;
    }

    if (totalSize >= 2U)
    {
        return 2U;
    }

    return 1U;
}

static i3c_rx_trigger_level_t block_stream_dma_seed_trigger_level(uint32_t seedByteCount)
{
    if (seedByteCount >= 6U)
    {
        return kI3C_RxTriggerUntilThreeQuarterOrMore;
    }

    if (seedByteCount >= 4U)
    {
        return kI3C_RxTriggerUntilOneHalfOrMore;
    }

    if (seedByteCount >= 2U)
    {
        return kI3C_RxTriggerUntilOneQuarterOrMore;
    }

    return kI3C_RxTriggerOnNotEmpty;
}

static void clear_dma0_channel_state(void)
{
    const uint32_t channelMask = (1UL << EXAMPLE_I3C_TX_CHANNEL) | (1UL << EXAMPLE_I3C_RX_CHANNEL);

    EXAMPLE_DMA->COMMON[0].INTA = channelMask;
    EXAMPLE_DMA->COMMON[0].INTB = channelMask;
    EXAMPLE_DMA->COMMON[0].ERRINT = channelMask;
    EXAMPLE_DMA->COMMON[0].INTENCLR = channelMask;
    EXAMPLE_DMA->COMMON[0].ENABLECLR = channelMask;
}

static status_t wait_for_post_ibi_read_ctrl_done(I3C_Type *base)
{
    uint32_t timeout = I3C_DMA_OFFICIAL_TIMEOUT;

    while (timeout-- != 0U)
    {
        uint32_t status = I3C_MasterGetStatusFlags(base);
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        if ((status & (uint32_t)kI3C_MasterControlDoneFlag) != 0U)
        {
            I3C_MasterClearStatusFlags(base, (uint32_t)kI3C_MasterControlDoneFlag);
            if (errStatus != 0U)
            {
                I3C_MasterClearErrorStatusFlags(base, errStatus);
            }
            return kStatus_Success;
        }

        if ((errStatus & ~((uint32_t)kI3C_MasterErrorNackFlag)) != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }
    }

    return kStatus_Timeout;
}

static status_t wait_for_post_ibi_read_complete(I3C_Type *base)
{
    uint32_t timeout = I3C_DMA_OFFICIAL_TIMEOUT;

    while (timeout-- != 0U)
    {
        uint32_t status = I3C_MasterGetStatusFlags(base);
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        if ((status & (uint32_t)kI3C_MasterCompleteFlag) != 0U)
        {
            I3C_MasterClearStatusFlags(base, (uint32_t)kI3C_MasterCompleteFlag);
            if (errStatus != 0U)
            {
                I3C_MasterClearErrorStatusFlags(base, errStatus);
            }
            return kStatus_Success;
        }

        if ((errStatus & ~((uint32_t)kI3C_MasterErrorNackFlag)) != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }
    }

    return kStatus_Timeout;
}

static status_t wait_for_seed_tail_mailbox(I3C_Type *base)
{
    uint32_t timeout = I3C_DMA_OFFICIAL_TIMEOUT;

    while ((s_rx_seed_read_param.mailbox == 0U) && (timeout-- != 0U))
    {
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        if (errStatus != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }
    }

    return (s_rx_seed_read_param.mailbox != 0U) ? kStatus_Success : kStatus_Timeout;
}

static status_t wait_for_master_idle_simple(I3C_Type *base)
{
    uint32_t timeout = I3C_DMA_OFFICIAL_TIMEOUT;

    while (timeout-- != 0U)
    {
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        if ((errStatus & ~((uint32_t)kI3C_MasterErrorNackFlag)) != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }

        if ((I3C_MasterGetState(base) == kI3C_MasterStateIdle) && I3C_MasterGetBusIdleState(base))
        {
            return kStatus_Success;
        }
    }

    return kStatus_Timeout;
}

static status_t quiesce_post_ibi_to_idle(I3C_Type *base)
{
    status_t result;

    if ((I3C_MasterGetState(base) == kI3C_MasterStateIdle) && I3C_MasterGetBusIdleState(base))
    {
        return kStatus_Success;
    }

    result = I3C_MasterStop(base);
    if (result == kStatus_Success)
    {
        result = wait_for_post_ibi_read_ctrl_done(base);
        if ((result == kStatus_Success) && (I3C_MasterGetState(base) == kI3C_MasterStateIdle))
        {
            return kStatus_Success;
        }
    }
    else if ((result == kStatus_I3C_InvalidReq) && (I3C_MasterGetState(base) == kI3C_MasterStateIdle))
    {
        return kStatus_Success;
    }

    I3C_MasterEmitRequest(base, kI3C_RequestForceExit);
    result = wait_for_post_ibi_read_ctrl_done(base);
    if ((result == kStatus_Success) && (I3C_MasterGetState(base) == kI3C_MasterStateIdle))
    {
        return kStatus_Success;
    }

    return result;
}

static status_t run_chunk_seed_tail_read(uint8_t slaveAddr, uint32_t expectedReadSize)
{
    const uint32_t channelMask = (1UL << EXAMPLE_I3C_RX_CHANNEL);
    const uint32_t seedByteCount = block_stream_dma_seed_byte_count(expectedReadSize);
    const uint32_t tailByteCount = expectedReadSize - seedByteCount;
    dma_transfer_config_t dmaTransfer;
    status_t result;
    uint32_t armCycleStart = read_cycle_counter();

    SDK_DelayAtLeastUs(I3C_DMA_SEED_TAIL_IBI_PRE_READ_DELAY_US, SystemCoreClock);

    memset((void *)&s_rx_seed_read_param, 0, sizeof(s_rx_seed_read_param));
    memset(g_chunk_rxBuff, 0, sizeof(g_chunk_rxBuff));

    g_dma0_dbg_irq_count = 0U;
    g_dma0_dbg_first_intstat = 0U;
    g_dma0_dbg_first_inta = 0U;
    g_dma0_dbg_first_active = 0U;
    g_dma0_dbg_last_intstat = 0U;
    g_dma0_dbg_last_inta = 0U;
    g_dma0_dbg_last_active = 0U;
    g_i3c_dbg_irq_count = 0U;
    g_i3c_dbg_irq_data_count = 0U;
    g_i3c_dbg_irq_protocol_count = 0U;
    g_i3c_dbg_irq_last_pending = 0U;
    g_i3c_dbg_irq_first_data_pending = 0U;

    result = quiesce_post_ibi_to_idle(EXAMPLE_MASTER);
    if (result != kStatus_Success)
    {
        return result;
    }

    clear_dma0_channel_state();
    I3C_MasterEnableDMA(EXAMPLE_MASTER, false, false, 1U);

    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, SMART_DMA_TRIGGER_CHANNEL, kINPUTMUX_Dma0IrqToSmartDmaInput);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0RxToDmac0Ch24RequestEna, true);
    INPUTMUX_Deinit(INPUTMUX);

    NVIC_DisableIRQ(DMA0_IRQn);
    NVIC_ClearPendingIRQ(DMA0_IRQn);
    NVIC_DisableIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);

    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_I3C_RX_CHANNEL);
    DMA_CreateHandle(&g_seedReadDmaHandle, EXAMPLE_DMA, EXAMPLE_I3C_RX_CHANNEL);
    EXAMPLE_DMA->COMMON[0].INTENCLR = channelMask;
    NVIC_DisableIRQ(DMA0_IRQn);
    NVIC_ClearPendingIRQ(DMA0_IRQn);
    DMA_PrepareTransfer(&dmaTransfer,
                        (uint32_t *)(uint32_t)&EXAMPLE_MASTER->MRDATAB,
                        g_chunk_rxBuff,
                        sizeof(uint8_t),
                        seedByteCount,
                        kDMA_PeripheralToMemory,
                        NULL);
    result = DMA_SubmitTransfer(&g_seedReadDmaHandle, &dmaTransfer);
    if (result != kStatus_Success)
    {
        goto exit;
    }

    s_rx_seed_read_param.expectedWakeCount = 1U;
    s_rx_seed_read_param.nextByteAddress = (uint32_t)(uintptr_t)&g_chunk_rxBuff[seedByteCount];
    s_rx_seed_read_param.remainingCount = tailByteCount;
    s_rx_seed_read_param.i3cBaseAddress = (uint32_t)(uintptr_t)EXAMPLE_MASTER;
    s_rx_seed_read_param.dmaIntaAddress = (uint32_t)(uintptr_t)&EXAMPLE_DMA->COMMON[0].INTA;
    s_rx_seed_read_param.dmaChannelMask = channelMask;

    if (!g_smartdmaWakeProbeInitialized)
    {
        POWER_DisablePD(kPDRUNCFG_APD_SMARTDMA_SRAM);
        POWER_DisablePD(kPDRUNCFG_PPD_SMARTDMA_SRAM);
        POWER_ApplyPD();

        keep_smartdma_api_alive();
        SMARTDMA_Init(
            SMARTDMA_SRAM_ADDR, __smartdma_start__, (uint32_t)((uintptr_t)__smartdma_end__ - (uintptr_t)__smartdma_start__));
        g_smartdmaWakeProbeInitialized = true;
    }

    SMARTDMA_Reset();
    SMARTDMA_Boot(I3C_DMA_RX_SEED_TAIL_API_INDEX, &s_rx_seed_read_param, 0U);
    s_dma_official_total_smartdma_arm_us += cycles_to_us(read_cycle_counter() - armCycleStart);

    I3C_MasterClearErrorStatusFlags(EXAMPLE_MASTER, I3C_MasterGetErrorStatusFlags(EXAMPLE_MASTER));
    I3C_MasterDisableInterrupts(EXAMPLE_MASTER,
                                I3C_MASTER_PROTOCOL_CLEAR_MASK | (uint32_t)kI3C_MasterTxReadyFlag |
                                    (uint32_t)kI3C_MasterRxReadyFlag);
    I3C_MasterClearStatusFlags(EXAMPLE_MASTER, I3C_MASTER_PROTOCOL_CLEAR_MASK);
    EXAMPLE_MASTER->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    EXAMPLE_MASTER->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    I3C_MasterEmitRequest(EXAMPLE_MASTER, kI3C_RequestNone);
    I3C_MasterSetWatermarks(EXAMPLE_MASTER,
                            kI3C_TxTriggerOnEmpty,
                            block_stream_dma_seed_trigger_level(seedByteCount),
                            false,
                            false);

    result = I3C_MasterStartWithRxSize(EXAMPLE_MASTER, kI3C_TypeI3CSdr, slaveAddr, kI3C_Read, (uint8_t)expectedReadSize);
    if (result != kStatus_Success)
    {
        goto exit;
    }

    result = wait_for_post_ibi_read_ctrl_done(EXAMPLE_MASTER);
    if (result != kStatus_Success)
    {
        goto exit;
    }

    DMA_StartTransfer(&g_seedReadDmaHandle);
    I3C_MasterEnableDMA(EXAMPLE_MASTER, false, true, 1U);

    armCycleStart = read_cycle_counter();
    result = wait_for_seed_tail_mailbox(EXAMPLE_MASTER);
    s_dma_official_total_smartdma_wait_us += cycles_to_us(read_cycle_counter() - armCycleStart);
    if (result != kStatus_Success)
    {
        goto exit;
    }

    result = wait_for_post_ibi_read_complete(EXAMPLE_MASTER);
    if (result != kStatus_Success)
    {
        goto exit;
    }

    result = I3C_MasterStop(EXAMPLE_MASTER);
    if (result == kStatus_Success)
    {
        result = wait_for_post_ibi_read_ctrl_done(EXAMPLE_MASTER);
    }
    else if ((result == kStatus_I3C_InvalidReq) && (I3C_MasterGetState(EXAMPLE_MASTER) == kI3C_MasterStateIdle))
    {
        result = kStatus_Success;
    }
    if (result != kStatus_Success)
    {
        goto exit;
    }

    if (tailByteCount != 0U)
    {
        uint8_t reorderedData[I3C_DATA_LENGTH];

        memcpy(reorderedData, g_chunk_rxBuff, expectedReadSize);
        memcpy(g_chunk_rxBuff, &reorderedData[seedByteCount], tailByteCount);
        memcpy(&g_chunk_rxBuff[tailByteCount], reorderedData, seedByteCount);
    }

    s_dma_official_smartdma_mailbox = s_rx_seed_read_param.mailbox;
    s_dma_official_smartdma_dma_inta_snapshot = EXAMPLE_DMA->COMMON[0].INTA;
    s_dma_official_smartdma_mdmactrl = EXAMPLE_MASTER->MDMACTRL;
    s_dma_official_smartdma_mstatus = EXAMPLE_MASTER->MSTATUS;
    s_dma_official_smartdma_mdatactrl = EXAMPLE_MASTER->MDATACTRL;

    if ((s_rx_seed_read_param.mailbox == 0U) || (s_rx_seed_read_param.wakeCount != 1U) ||
        (s_rx_seed_read_param.dmaIntaCount != 1U) || (s_rx_seed_read_param.remainingCount != 0U))
    {
        result = kStatus_Fail;
        goto exit;
    }

    s_dma_official_total_smartdma_wake_count += s_rx_seed_read_param.wakeCount;
    s_dma_official_total_smartdma_dma_inta_count += s_rx_seed_read_param.dmaIntaCount;

    result = wait_for_master_idle_simple(EXAMPLE_MASTER);

exit:
    I3C_MasterEnableDMA(EXAMPLE_MASTER, false, false, 1U);
    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, true);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0RxToDmac0Ch24RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);
    NVIC_ClearPendingIRQ(DMA0_IRQn);
    if (EXPERIMENT_ALLOW_CM33_DMA_IRQ == 0U)
    {
        NVIC_DisableIRQ(DMA0_IRQn);
    }
    clear_dma0_channel_state();
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    return result;
}

static void snapshot_smartdma_wake_state(void)
{
    s_dma_official_smartdma_mailbox = s_smartdma_wake_param.mailbox;
    s_dma_official_smartdma_dma_inta_snapshot = s_smartdma_wake_param.dmaIntaSnapshot;
    s_dma_official_smartdma_mdmactrl = s_smartdma_wake_param.i3cMdmaCtrlSnapshot;
    s_dma_official_smartdma_mstatus = s_smartdma_wake_param.i3cMstatusSnapshot;
    s_dma_official_smartdma_mdatactrl = s_smartdma_wake_param.i3cMdataCtrlSnapshot;
}

static uint32_t get_chunk_data_length(uint32_t chunkIndex)
{
    uint32_t chunkOffset = chunkIndex * I3C_DATA_LENGTH;

    if (chunkOffset >= I3C_LOGICAL_DATA_LENGTH)
    {
        return 0U;
    }

    if ((I3C_LOGICAL_DATA_LENGTH - chunkOffset) < I3C_DATA_LENGTH)
    {
        return I3C_LOGICAL_DATA_LENGTH - chunkOffset;
    }

    return I3C_DATA_LENGTH;
}

static uint8_t expected_stream_byte(uint32_t chunkIndex, uint32_t byteIndex)
{
    return (uint8_t)(((chunkIndex * I3C_STREAM_BLOCK_BYTES) + byteIndex) & 0xFFU);
}

static void prepare_chunk_write_payload(uint32_t chunkDataLength)
{
    memset(g_master_txBuff, 0, sizeof(g_master_txBuff));
    g_master_txBuff[0] = I3C_STREAM_REQUEST_TOKEN;
    g_master_txBuff[1] = (uint8_t)chunkDataLength;
}

static status_t reset_slave_session_generation(uint8_t slaveAddr)
{
    status_t result;
    i3c_master_transfer_t masterXfer;
    uint8_t resetToken = I3C_SLAVE_SESSION_RESET_TOKEN;

    g_masterCompletionFlag = false;
    g_completionStatus = kStatus_Success;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.data = &resetToken;
    masterXfer.dataSize = sizeof(resetToken);
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;

    result = I3C_MasterTransferDMA(EXAMPLE_MASTER, &g_i3cMasterHandle, &masterXfer);
    if (result != kStatus_Success)
    {
        return result;
    }

    return wait_for_transfer_complete(I3C_DMA_OFFICIAL_TIMEOUT);
}

static status_t run_chunk_roundtrip(uint8_t slaveAddr, uint32_t chunkIndex)
{
    status_t result;
    i3c_master_transfer_t masterXfer;
    uint32_t chunkOffset = chunkIndex * I3C_DATA_LENGTH;
    uint32_t expectedReadSize = get_chunk_data_length(chunkIndex);
    uint32_t chunkPacketSize = sizeof(g_master_txBuff);
    uint32_t rxCallbackBaseline = g_i3c_dbg_dma_callback_rx_count;
    uint32_t chunkCycleStart = read_cycle_counter();
    uint32_t phaseCycleStart;

    s_dma_official_current_chunk_index = chunkIndex;
    prepare_chunk_write_payload(expectedReadSize);
    memset(g_chunk_rxBuff, 0, sizeof(g_chunk_rxBuff));
    memset(g_master_ibiBuff, 0, sizeof(g_master_ibiBuff));
    memset(g_ibiBuff, 0, sizeof(g_ibiBuff));
    g_masterCompletionFlag = false;
    g_ibiWonFlag = false;
    g_completionStatus = kStatus_Success;
    g_ibiPayloadSize = 0U;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.data = g_master_txBuff;
    masterXfer.dataSize = chunkPacketSize;
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;

    result = I3C_MasterTransferDMA(EXAMPLE_MASTER, &g_i3cMasterHandle, &masterXfer);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageIbiRegistered, (int32_t)result);
        return result;
    }

    phaseCycleStart = read_cycle_counter();
    result = wait_for_transfer_complete(I3C_DMA_OFFICIAL_TIMEOUT);
    s_dma_official_total_write_wait_us += cycles_to_us(read_cycle_counter() - phaseCycleStart);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageIbiRegistered,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutCompletion : (int32_t)result);
        return result;
    }
    s_dma_official_stage = kDmaOfficialStageWriteDone;

    phaseCycleStart = read_cycle_counter();
    result = wait_for_ibi(I3C_DMA_OFFICIAL_TIMEOUT);
    s_dma_official_total_ibi_wait_us += cycles_to_us(read_cycle_counter() - phaseCycleStart);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageWriteDone,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutIbi : (int32_t)result);
        return result;
    }
    s_dma_official_stage = kDmaOfficialStageIbiSeen;

#if EXPERIMENT_IBI_GENERATION_TAG
    if ((g_ibiPayloadSize != 1U) || (g_ibiBuff[0] != (uint8_t)(chunkIndex + 1U)))
    {
        mark_failure(kDmaOfficialStageIbiSeen, kDmaOfficialResultUnexpectedIbiGeneration);
        return kStatus_Fail;
    }
#else
    if (g_ibiBuff[0] != expectedReadSize)
    {
        mark_failure(kDmaOfficialStageIbiSeen, kDmaOfficialResultUnexpectedChunkLength);
        return kStatus_Fail;
    }
#endif

    s_dma_official_stage = kDmaOfficialStageSmartDmaWakeArmed;

    phaseCycleStart = read_cycle_counter();
    result = run_chunk_seed_tail_read(slaveAddr, expectedReadSize);
    s_dma_official_total_read_wait_us += cycles_to_us(read_cycle_counter() - phaseCycleStart);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageSmartDmaWakeArmed,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutCompletion : (int32_t)result);
        return result;
    }

    for (uint32_t index = 0U; index < expectedReadSize; index++)
    {
        if (g_chunk_rxBuff[index] != expected_stream_byte(chunkIndex, index))
        {
#if !EXPERIMENT_DISABLE_RX_TAIL_RECOVERY
            s_dma_official_tail_recovery_count += recover_master_rx_tail(g_chunk_rxBuff, index, expectedReadSize);
#endif
            break;
        }
    }

    s_dma_official_stage = kDmaOfficialStageReadDone;

    if ((s_rx_seed_read_param.mailbox == 0U) || (s_rx_seed_read_param.wakeCount != 1U) ||
        (s_rx_seed_read_param.dmaIntaCount != 1U) || (s_rx_seed_read_param.remainingCount != 0U))
    {
        mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultUnexpectedSmartDmaWake);
        return kStatus_Fail;
    }

    s_dma_official_total_dma0_irq_count += g_dma0_dbg_irq_count;
    s_dma_official_total_data_irq_count += g_i3c_dbg_irq_data_count;
    s_dma_official_total_protocol_irq_count += g_i3c_dbg_irq_protocol_count;
    s_dma_official_total_rx_dma_callback_count += (g_i3c_dbg_dma_callback_rx_count - rxCallbackBaseline);

    if ((EXPERIMENT_ALLOW_CM33_DMA_IRQ == 0U) && (g_dma0_dbg_irq_count != 0U))
    {
        mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultCm33DmaIrqObserved);
        return kStatus_Fail;
    }

    if (g_i3c_dbg_irq_data_count != 0U)
    {
        mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultCm33DataIrqObserved);
        return kStatus_Fail;
    }

    if ((EXPERIMENT_ALLOW_CM33_DMA_IRQ == 0U) && ((g_i3c_dbg_dma_callback_rx_count - rxCallbackBaseline) != 0U))
    {
        mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultCm33RxCallbackObserved);
        return kStatus_Fail;
    }

    s_dma_official_stage = kDmaOfficialStageSmartDmaWakeSeen;

    for (uint32_t index = 0U; index < expectedReadSize; index++)
    {
        if (g_chunk_rxBuff[index] != expected_stream_byte(chunkIndex, index))
        {
            s_dma_official_mismatch_index = chunkOffset + index;
            mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultDataMismatch);
            return kStatus_Fail;
        }
    }

    memcpy(&g_master_rxBuff[chunkOffset], g_chunk_rxBuff, expectedReadSize);

    s_dma_official_rx_size += expectedReadSize;
    s_dma_official_completed_chunk_count = chunkIndex + 1U;
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_EnableIRQ(I3C0_IRQn);
    s_dma_official_total_chunk_time_us += cycles_to_us(read_cycle_counter() - chunkCycleStart);
    return kStatus_Success;
}

static void arm_smartdma_wake_probe(void)
{
    if (!g_smartdmaWakeProbeInitialized)
    {
        POWER_DisablePD(kPDRUNCFG_APD_SMARTDMA_SRAM);
        POWER_DisablePD(kPDRUNCFG_PPD_SMARTDMA_SRAM);
        POWER_ApplyPD();

        keep_smartdma_api_alive();

        INPUTMUX_Init(INPUTMUX);
        INPUTMUX_AttachSignal(INPUTMUX, SMART_DMA_TRIGGER_CHANNEL, kINPUTMUX_Dma0IrqToSmartDmaInput);
        INPUTMUX_Deinit(INPUTMUX);

        NVIC_ClearPendingIRQ(DMA0_IRQn);
        if (EXPERIMENT_ALLOW_CM33_DMA_IRQ == 0U)
        {
            NVIC_DisableIRQ(DMA0_IRQn);
        }
        NVIC_ClearPendingIRQ(SDMA_IRQn);
        NVIC_DisableIRQ(SDMA_IRQn);

        SMARTDMA_Init(
            SMARTDMA_SRAM_ADDR, __smartdma_start__, (uint32_t)((uintptr_t)__smartdma_end__ - (uintptr_t)__smartdma_start__));
        g_smartdmaWakeProbeInitialized = true;
    }

    memset((void *)&s_smartdma_wake_param, 0, sizeof(s_smartdma_wake_param));
    s_smartdma_wake_param.i3cBaseAddress = (uint32_t)(uintptr_t)EXAMPLE_MASTER;
    s_smartdma_wake_param.dmaIntaAddress = (uint32_t)(uintptr_t)&EXAMPLE_DMA->COMMON[0].INTA;

    g_dma0_dbg_irq_count = 0U;
    g_dma0_dbg_first_intstat = 0U;
    g_dma0_dbg_first_inta = 0U;
    g_dma0_dbg_first_active = 0U;
    g_dma0_dbg_last_intstat = 0U;
    g_dma0_dbg_last_inta = 0U;
    g_dma0_dbg_last_active = 0U;
    g_i3c_dbg_irq_count = 0U;
    g_i3c_dbg_irq_data_count = 0U;
    g_i3c_dbg_irq_protocol_count = 0U;
    g_i3c_dbg_irq_last_pending = 0U;
    g_i3c_dbg_irq_first_data_pending = 0U;

    rearm_smartdma_wake_probe();
    SMARTDMA_Boot(I3C_DMA_WAKE_API_INDEX, &s_smartdma_wake_param, 0U);
}

static status_t wait_for_smartdma_wake(uint32_t timeout)
{
    while ((s_smartdma_wake_param.mailbox == 0U) && (timeout != 0U))
    {
        timeout--;
    }

    snapshot_smartdma_wake_state();

    if (s_smartdma_wake_param.mailbox == 0U)
    {
        return kStatus_Timeout;
    }

    return kStatus_Success;
}

static void suppress_read_data_irq_bounce(void)
{
    I3C_MasterDisableInterrupts(EXAMPLE_MASTER,
                                (uint32_t)kI3C_MasterTxReadyFlag | (uint32_t)kI3C_MasterRxReadyFlag);
    I3C_MasterClearStatusFlags(EXAMPLE_MASTER,
                               (uint32_t)kI3C_MasterTxReadyFlag | (uint32_t)kI3C_MasterRxReadyFlag);
    g_i3cMasterHandle.state = (uint8_t)kI3cDmaStateWaitForCompletion;
    NVIC_ClearPendingIRQ(I3C0_IRQn);
}

static void scrub_master_chunk_boundary_state(void)
{
    I3C_MasterDisableInterrupts(EXAMPLE_MASTER,
                                (uint32_t)kI3C_MasterTxReadyFlag | (uint32_t)kI3C_MasterRxReadyFlag);
    I3C_MasterClearStatusFlags(EXAMPLE_MASTER,
                               I3C_MASTER_PROTOCOL_CLEAR_MASK | (uint32_t)kI3C_MasterTxReadyFlag |
                                   (uint32_t)kI3C_MasterRxReadyFlag);
    I3C_MasterClearErrorStatusFlags(EXAMPLE_MASTER, I3C_MASTER_ERROR_CLEAR_MASK);
    EXAMPLE_MASTER->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    NVIC_ClearPendingIRQ(I3C0_IRQn);
}

static void i3c_master_ibi_callback(I3C_Type *base,
                                    i3c_master_dma_handle_t *handle,
                                    i3c_ibi_type_t ibiType,
                                    i3c_ibi_state_t ibiState)
{
    (void)base;

    switch (ibiType)
    {
        case kI3C_IbiNormal:
            if (ibiState == kI3C_IbiDataBuffNeed)
            {
                handle->ibiBuff = g_master_ibiBuff;
            }
            else
            {
                memcpy(g_ibiBuff, (void *)handle->ibiBuff, handle->ibiPayloadSize);
                g_ibiPayloadSize = (uint8_t)handle->ibiPayloadSize;
                s_dma_official_ibi_payload_size = (uint32_t)handle->ibiPayloadSize;
                s_dma_official_ibi_data0 = (handle->ibiPayloadSize != 0U) ? g_ibiBuff[0] : 0U;
                s_dma_official_ibi_data1 = (handle->ibiPayloadSize > 1U) ? g_ibiBuff[1] : 0U;
            }
            break;

        default:
            break;
    }
}

static void i3c_master_callback(I3C_Type *base,
                                i3c_master_dma_handle_t *handle,
                                status_t status,
                                void *userData)
{
    (void)base;
    (void)handle;
    (void)userData;

    if (status == kStatus_Success)
    {
        g_masterCompletionFlag = true;
    }

    if (status == kStatus_I3C_IBIWon)
    {
        g_ibiWonFlag = true;
    }

    g_completionStatus = status;
    s_dma_official_completion_status = (uint32_t)status;
}

int main(void)
{
    status_t result;
    i3c_master_config_t masterConfig;
    i3c_master_transfer_t masterXfer;
    i3c_register_ibi_addr_t ibiRecord;
    uint8_t addressList[6] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U};
    uint8_t devCount = 0U;
    i3c_device_info_t *devList;
    uint8_t slaveAddr = 0U;

    reset_retained_state();
    s_dma_official_stage = kDmaOfficialStageInit;

    BOARD_InitHardware();
    init_cycle_counter();
    init_transfer_led();
    run_boot_led_self_test();

    PRINTF("MCUX SDK version: %s\r\n", MCUXSDK_VERSION_FULL_STR);
    PRINTF("\r\nI3C DMA official RX SmartDMA wake block stream -- Master transfer.\r\n");

    g_masterCompletionFlag = false;
    g_ibiWonFlag = false;
    g_completionStatus = kStatus_Success;
    g_ibiPayloadSize = 0U;

    memset(g_master_rxBuff, 0, sizeof(g_master_rxBuff));
    memset(g_master_ibiBuff, 0, sizeof(g_master_ibiBuff));
    memset(g_ibiBuff, 0, sizeof(g_ibiBuff));

    I3C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Hz.i2cBaud = EXAMPLE_I2C_BAUDRATE;
    masterConfig.baudRate_Hz.i3cPushPullBaud = EXAMPLE_I3C_PP_BAUDRATE;
    masterConfig.baudRate_Hz.i3cOpenDrainBaud = EXAMPLE_I3C_OD_BAUDRATE;
    masterConfig.enableOpenDrainStop = false;
    I3C_MasterInit(EXAMPLE_MASTER, &masterConfig, I3C_MASTER_CLOCK_FREQUENCY);

    DMA_Init(EXAMPLE_DMA);
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_I3C_RX_CHANNEL);
    DMA_CreateHandle(&g_rxDmaHandle, EXAMPLE_DMA, EXAMPLE_I3C_RX_CHANNEL);
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_I3C_TX_CHANNEL);
    DMA_CreateHandle(&g_txDmaHandle, EXAMPLE_DMA, EXAMPLE_I3C_TX_CHANNEL);
    I3C_MasterTransferCreateHandleDMA(EXAMPLE_MASTER,
                                      &g_i3cMasterHandle,
                                      &s_masterCallback,
                                      NULL,
                                      &g_rxDmaHandle,
                                      &g_txDmaHandle);
    s_dma_official_stage = kDmaOfficialStageMasterReady;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = 0x7EU;
    masterXfer.subaddress = 0x06U;
    masterXfer.subaddressSize = 1U;
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;

    result = I3C_MasterTransferDMA(EXAMPLE_MASTER, &g_i3cMasterHandle, &masterXfer);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageMasterReady, (int32_t)result);
        goto fail;
    }

    result = wait_for_transfer_complete(I3C_DMA_OFFICIAL_TIMEOUT);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageMasterReady,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutCompletion : (int32_t)result);
        goto fail;
    }
    s_dma_official_stage = kDmaOfficialStageDaaCccDone;

    result = I3C_MasterProcessDAA(EXAMPLE_MASTER, addressList, sizeof(addressList));
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageDaaCccDone, (int32_t)result);
        goto fail;
    }

    devList = I3C_MasterGetDeviceListAfterDAA(EXAMPLE_MASTER, &devCount);
    s_dma_official_dev_count = devCount;
    for (uint8_t devIndex = 0U; devIndex < devCount; devIndex++)
    {
        if (devList[devIndex].vendorID == 0x123U)
        {
            slaveAddr = devList[devIndex].dynamicAddr;
            break;
        }
    }

    if (slaveAddr == 0U)
    {
        mark_failure(kDmaOfficialStageDaaCccDone, kDmaOfficialResultNoSlaveAddr);
        goto fail;
    }

    s_dma_official_slave_addr = slaveAddr;
    s_dma_official_stage = kDmaOfficialStageDaaDone;

    result = reset_slave_session_generation(slaveAddr);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageDaaDone,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutCompletion : (int32_t)result);
        goto fail;
    }

    memset(&ibiRecord, 0, sizeof(ibiRecord));
    ibiRecord.address[0] = slaveAddr;
    ibiRecord.ibiHasPayload = true;
    I3C_MasterRegisterIBI(EXAMPLE_MASTER, &ibiRecord);

    s_dma_official_stage = kDmaOfficialStageIbiRegistered;

    for (uint32_t chunkIndex = 0U; chunkIndex < I3C_LOGICAL_CHUNK_COUNT; chunkIndex++)
    {
        result = run_chunk_roundtrip(slaveAddr, chunkIndex);
        if (result != kStatus_Success)
        {
            goto fail;
        }

        if ((chunkIndex + 1U) < I3C_LOGICAL_CHUNK_COUNT)
        {
            scrub_master_chunk_boundary_state();
            SDK_DelayAtLeastUs(I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US, SystemCoreClock);
        }
    }

    s_dma_official_rx_first = (s_dma_official_rx_size != 0U) ? g_master_rxBuff[0] : 0U;
    s_dma_official_rx_last = (s_dma_official_rx_size != 0U) ? g_master_rxBuff[s_dma_official_rx_size - 1U] : 0U;
    memcpy((void *)s_dma_official_rx_snapshot, g_master_rxBuff, sizeof(g_master_rxBuff));

    if (s_dma_official_total_smartdma_wake_count != I3C_LOGICAL_CHUNK_COUNT)
    {
        mark_failure(kDmaOfficialStageSmartDmaWakeSeen, kDmaOfficialResultUnexpectedSmartDmaWake);
        goto fail;
    }

    s_dma_official_stage = kDmaOfficialStageValidated;
    s_dma_official_outcome = kDmaOfficialOutcomeSuccess;
    s_dma_official_result = kDmaOfficialResultSuccess;
    PRINTF("I3C DMA official RX SmartDMA wake block stream successful\r\n");
    set_success_led();
    while (1)
    {
        __NOP();
    }

fail:
    snapshot_smartdma_wake_state();
    PRINTF("I3C DMA official RX SmartDMA wake block stream failed: stage=%lu result=%ld status=%lu\r\n",
           (unsigned long)s_dma_official_stage,
           (long)s_dma_official_result,
           (unsigned long)s_dma_official_completion_status);
    set_failure_led();
    while (1)
    {
        __NOP();
    }
}