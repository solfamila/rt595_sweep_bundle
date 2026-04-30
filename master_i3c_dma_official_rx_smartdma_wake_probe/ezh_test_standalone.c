/*
 * Minimal RT595 master experiment that preserves the official classic DMA I3C
 * RX proof while adding a DMA0 IRQ -> SmartDMA mailbox wake probe.
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
#define I3C_DATA_LENGTH 6U
#define I3C_PACKET_LENGTH (I3C_DATA_LENGTH + 1U)
#define I3C_DMA_OFFICIAL_TIMEOUT 100000000U
#define I3C_DMA_OFFICIAL_LED_PULSE_US 120000U
#define I3C_DMA_WAKE_API_INDEX 0U
#define I3C_DMA_WAKE_MAILBOX_COMPLETE 1U
#define SMART_DMA_TRIGGER_CHANNEL 0U

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

static __NO_INIT volatile uint32_t s_dma_official_stage;
static __NO_INIT volatile uint32_t s_dma_official_outcome;
static __NO_INIT volatile int32_t s_dma_official_result;
static __NO_INIT volatile uint32_t s_dma_official_completion_status;
static __NO_INIT volatile uint32_t s_dma_official_slave_addr;
static __NO_INIT volatile uint32_t s_dma_official_dev_count;
static __NO_INIT volatile uint32_t s_dma_official_ibi_payload_size;
static __NO_INIT volatile uint32_t s_dma_official_ibi_data0;
static __NO_INIT volatile uint32_t s_dma_official_rx_size;
static __NO_INIT volatile uint32_t s_dma_official_mismatch_index;
static __NO_INIT volatile uint32_t s_dma_official_rx_first;
static __NO_INIT volatile uint32_t s_dma_official_rx_last;
static __NO_INIT volatile uint32_t s_dma_official_tail_recovery_count;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mailbox;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_wake_count;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_dma_inta_count;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_dma_inta_snapshot;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mdmactrl;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mstatus;
static __NO_INIT volatile uint32_t s_dma_official_smartdma_mdatactrl;
static __NO_INIT volatile uint8_t s_dma_official_rx_snapshot[I3C_PACKET_LENGTH];

AT_NONCACHEABLE_SECTION_ALIGN(static i3c_dma_smartdma_wake_param_t s_smartdma_wake_param, 4);

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_master_txBuff[I3C_PACKET_LENGTH], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_master_rxBuff[I3C_PACKET_LENGTH], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_master_ibiBuff[10U], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t g_ibiBuff[10U], 4);
static uint8_t g_ibiPayloadSize;
static i3c_master_dma_handle_t g_i3cMasterHandle;
static dma_handle_t g_txDmaHandle;
static dma_handle_t g_rxDmaHandle;
static volatile bool g_masterCompletionFlag;
static volatile bool g_ibiWonFlag;
static volatile status_t g_completionStatus = kStatus_Success;

static void i3c_master_ibi_callback(I3C_Type *base,
                                    i3c_master_dma_handle_t *handle,
                                    i3c_ibi_type_t ibiType,
                                    i3c_ibi_state_t ibiState);
static void i3c_master_callback(I3C_Type *base,
                                i3c_master_dma_handle_t *handle,
                                status_t status,
                                void *userData);

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
    s_dma_official_ibi_payload_size = 0U;
    s_dma_official_ibi_data0 = 0U;
    s_dma_official_rx_size = 0U;
    s_dma_official_mismatch_index = 0xFFFFFFFFU;
    s_dma_official_rx_first = 0U;
    s_dma_official_rx_last = 0U;
    s_dma_official_tail_recovery_count = 0U;
    s_dma_official_smartdma_mailbox = 0U;
    s_dma_official_smartdma_wake_count = 0U;
    s_dma_official_smartdma_dma_inta_count = 0U;
    s_dma_official_smartdma_dma_inta_snapshot = 0U;
    s_dma_official_smartdma_mdmactrl = 0U;
    s_dma_official_smartdma_mstatus = 0U;
    s_dma_official_smartdma_mdatactrl = 0U;
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

static uint32_t recover_master_rx_tail(uint32_t startIndex, uint32_t totalSize)
{
    uint32_t writeIndex = startIndex;
    uint32_t timeout = I3C_DMA_OFFICIAL_TIMEOUT;

    while ((writeIndex < totalSize) && (timeout != 0U))
    {
        uint32_t rxCount =
            (EXAMPLE_MASTER->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;

        while ((rxCount != 0U) && (writeIndex < totalSize))
        {
            g_master_rxBuff[writeIndex++] = (uint8_t)(EXAMPLE_MASTER->MRDATAB & I3C_MRDATAB_VALUE_MASK);
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

static void snapshot_smartdma_wake_state(void)
{
    s_dma_official_smartdma_mailbox = s_smartdma_wake_param.mailbox;
    s_dma_official_smartdma_wake_count = s_smartdma_wake_param.wakeCount;
    s_dma_official_smartdma_dma_inta_count = s_smartdma_wake_param.dmaIntaCount;
    s_dma_official_smartdma_dma_inta_snapshot = s_smartdma_wake_param.dmaIntaSnapshot;
    s_dma_official_smartdma_mdmactrl = s_smartdma_wake_param.i3cMdmaCtrlSnapshot;
    s_dma_official_smartdma_mstatus = s_smartdma_wake_param.i3cMstatusSnapshot;
    s_dma_official_smartdma_mdatactrl = s_smartdma_wake_param.i3cMdataCtrlSnapshot;
}

static void arm_smartdma_wake_probe(void)
{
    POWER_DisablePD(kPDRUNCFG_APD_SMARTDMA_SRAM);
    POWER_DisablePD(kPDRUNCFG_PPD_SMARTDMA_SRAM);
    POWER_ApplyPD();

    keep_smartdma_api_alive();

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

    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, SMART_DMA_TRIGGER_CHANNEL, kINPUTMUX_Dma0IrqToSmartDmaInput);
    INPUTMUX_Deinit(INPUTMUX);

    NVIC_ClearPendingIRQ(DMA0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);

    SMARTDMA_Init(
        SMARTDMA_SRAM_ADDR, __smartdma_start__, (uint32_t)((uintptr_t)__smartdma_end__ - (uintptr_t)__smartdma_start__));
    SMARTDMA_Reset();
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
    uint8_t addressList[6] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U};
    uint8_t devCount = 0U;
    i3c_device_info_t *devList;
    uint8_t slaveAddr = 0U;
    i3c_register_ibi_addr_t ibiRecord;

    reset_retained_state();
    s_dma_official_stage = kDmaOfficialStageInit;

    BOARD_InitHardware();
    init_transfer_led();
    run_boot_led_self_test();

    PRINTF("MCUX SDK version: %s\r\n", MCUXSDK_VERSION_FULL_STR);
    PRINTF("\r\nI3C DMA official RX SmartDMA wake probe -- Master transfer.\r\n");

    g_masterCompletionFlag = false;
    g_ibiWonFlag = false;
    g_completionStatus = kStatus_Success;
    g_ibiPayloadSize = 0U;

    g_master_txBuff[0] = I3C_DATA_LENGTH;
    for (uint32_t index = 1U; index < I3C_PACKET_LENGTH; index++)
    {
        g_master_txBuff[index] = (uint8_t)(index - 1U);
    }
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

    memset(&ibiRecord, 0, sizeof(ibiRecord));
    ibiRecord.address[0] = slaveAddr;
    ibiRecord.ibiHasPayload = true;
    I3C_MasterRegisterIBI(EXAMPLE_MASTER, &ibiRecord);
    s_dma_official_stage = kDmaOfficialStageIbiRegistered;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.data = g_master_txBuff;
    masterXfer.dataSize = I3C_PACKET_LENGTH;
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;

    result = I3C_MasterTransferDMA(EXAMPLE_MASTER, &g_i3cMasterHandle, &masterXfer);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageIbiRegistered, (int32_t)result);
        goto fail;
    }

    result = wait_for_transfer_complete(I3C_DMA_OFFICIAL_TIMEOUT);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageIbiRegistered,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutCompletion : (int32_t)result);
        goto fail;
    }
    s_dma_official_stage = kDmaOfficialStageWriteDone;

    result = wait_for_ibi(I3C_DMA_OFFICIAL_TIMEOUT);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageWriteDone,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutIbi : (int32_t)result);
        goto fail;
    }
    s_dma_official_stage = kDmaOfficialStageIbiSeen;

    memset(g_master_rxBuff, 0, sizeof(g_master_rxBuff));
    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.data = g_master_rxBuff;
    masterXfer.dataSize = g_ibiBuff[0];
    masterXfer.direction = kI3C_Read;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;
    s_dma_official_rx_size = g_ibiBuff[0];

    arm_smartdma_wake_probe();
    s_dma_official_stage = kDmaOfficialStageSmartDmaWakeArmed;

    result = I3C_MasterTransferDMA(EXAMPLE_MASTER, &g_i3cMasterHandle, &masterXfer);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageSmartDmaWakeArmed, (int32_t)result);
        goto fail;
    }

    result = wait_for_transfer_complete(I3C_DMA_OFFICIAL_TIMEOUT);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageSmartDmaWakeArmed,
                     (result == kStatus_Timeout) ? kDmaOfficialResultTimeoutCompletion : (int32_t)result);
        goto fail;
    }

    for (uint32_t index = 0U; index < g_ibiBuff[0]; index++)
    {
        if (g_master_rxBuff[index] != g_master_txBuff[index + 1U])
        {
            s_dma_official_tail_recovery_count = recover_master_rx_tail(index, g_ibiBuff[0]);
            break;
        }
    }

    s_dma_official_stage = kDmaOfficialStageReadDone;
    s_dma_official_rx_first = (g_ibiBuff[0] != 0U) ? g_master_rxBuff[0] : 0U;
    s_dma_official_rx_last = (g_ibiBuff[0] != 0U) ? g_master_rxBuff[g_ibiBuff[0] - 1U] : 0U;
    memcpy((void *)s_dma_official_rx_snapshot, g_master_rxBuff, sizeof(g_master_rxBuff));

    result = wait_for_smartdma_wake(I3C_DMA_OFFICIAL_TIMEOUT);
    if (result != kStatus_Success)
    {
        mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultTimeoutSmartDmaWake);
        goto fail;
    }

    if ((s_smartdma_wake_param.mailbox != I3C_DMA_WAKE_MAILBOX_COMPLETE) || (s_smartdma_wake_param.wakeCount != 1U))
    {
        mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultUnexpectedSmartDmaWake);
        goto fail;
    }

    s_dma_official_stage = kDmaOfficialStageSmartDmaWakeSeen;

    for (uint32_t index = 0U; index < g_master_txBuff[0]; index++)
    {
        if (g_master_rxBuff[index] != g_master_txBuff[index + 1U])
        {
            s_dma_official_mismatch_index = index;
            mark_failure(kDmaOfficialStageReadDone, kDmaOfficialResultDataMismatch);
            goto fail;
        }
    }

    s_dma_official_stage = kDmaOfficialStageValidated;
    s_dma_official_outcome = kDmaOfficialOutcomeSuccess;
    s_dma_official_result = kDmaOfficialResultSuccess;
    PRINTF("I3C DMA official RX SmartDMA wake probe successful\r\n");
    set_success_led();
    while (1)
    {
        __NOP();
    }

fail:
    snapshot_smartdma_wake_state();
    PRINTF("I3C DMA official RX SmartDMA wake probe failed: stage=%lu result=%ld status=%lu\r\n",
           (unsigned long)s_dma_official_stage,
           (long)s_dma_official_result,
           (unsigned long)s_dma_official_completion_status);
    set_failure_led();
    while (1)
    {
        __NOP();
    }
}