/*
 * Focused RX probe that follows the RT500 RX-DMA errata geometry:
 *  - RXTRIG = 3/4 full
 *  - DMA moves one seed byte per 6-byte FIFO block
 *  - DMA0 IRQ wakes SmartDMA, which drains the remaining 5 bytes
 *  - CM33 does not service RXREADY/TXREADY data IRQs
 */

#define main master_i3c_sdma_seed_tail_len_sweep_unused_main
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_test_standalone.c"
#undef main

#define I3C_RX_SEED6_BLOCK_BYTES 6U
#define I3C_RX_SEED6_SEED_BYTES 1U
#define I3C_RX_SEED6_TAIL_BYTES 5U
#define I3C_RX_SEED6_MAX_BLOCKS (I3C_DMA_SEED_TAIL_SWEEP_MAX_LENGTH / I3C_RX_SEED6_BLOCK_BYTES)
#define I3C_RX_SEED6_CASE_COUNT 6U
#define I3C_RX_SEED6_API_INDEX 2U
#define I3C_RX_SEED6_DMA_CHANNEL 24U
#define I3C_RX_SEED6_DMA_CLOCK kCLOCK_Dmac0
#define I3C_RX_SEED6_DMA_RESET kDMAC0_RST_SHIFT_RSTn
#define I3C_RX_SEED6_DMA_IRQ DMA0_IRQn
#define I3C_RX_SEED6_DMA_INPUTMUX_SIGNAL kINPUTMUX_I3c0RxToDmac0Ch24RequestEna
#define I3C_RX_SEED6_POST_WRITE_SETTLE_US 1000U
#define I3C_RX_SEED6_SAMPLE_BYTES 12U
#define I3C_RX_SEED6_MAILBOX_COMPLETION 1U

uint8_t *g_txBuff = NULL;
uint32_t g_txSize = 0U;
volatile bool g_slaveIbiRequestSent = false;
volatile bool g_slavePostIbiAddressMatched = false;
volatile bool g_slavePostIbiEchoPending = false;
volatile bool g_slavePostIbiEchoArmed = false;

typedef struct _i3c_rx_seed6_tail5_param
{
    volatile uint32_t mailbox;
    uint32_t expectedWakeCount;
    volatile uint32_t wakeCount;
    volatile uint32_t dmaSeedBytes;
    volatile uint32_t smartdmaTailBytes;
    volatile uint32_t dmaIntaCount;
    uint32_t nextRxByteAddress;
    uint32_t remainingCount;
    uint32_t i3cBaseAddress;
    uint32_t dmaIntaAddress;
    uint32_t dmaChannelMask;
} i3c_rx_seed6_tail5_param_t;

typedef enum _rx_seed6_stage
{
    kRxSeed6StageCleared = 0U,
    kRxSeed6StageWriteStarted = 1U,
    kRxSeed6StageReadStarted = 2U,
    kRxSeed6StageMailboxComplete = 3U,
    kRxSeed6StageI3cComplete = 4U,
    kRxSeed6StageValidated = 5U,
    kRxSeed6StageSuccess = 6U,
    kRxSeed6StageErrorWrite = 0x201U,
    kRxSeed6StageErrorGeometry = 0x202U,
    kRxSeed6StageErrorMailbox = 0x203U,
    kRxSeed6StageErrorComplete = 0x204U,
    kRxSeed6StageErrorCompare = 0x205U,
    kRxSeed6StageErrorIrq = 0x206U,
    kRxSeed6StageErrorMetrics = 0x207U,
} rx_seed6_stage_t;

AT_NONCACHEABLE_SECTION_ALIGN(static dma_descriptor_t s_rx_seed6_dma_chain[I3C_RX_SEED6_MAX_BLOCKS - 1U],
                              FSL_FEATURE_DMA_DESCRIPTOR_ALIGN_SIZE);
AT_NONCACHEABLE_SECTION_ALIGN(static i3c_rx_seed6_tail5_param_t s_rx_seed6_param, 4);

static const uint16_t s_rx_seed6_lengths[I3C_RX_SEED6_CASE_COUNT] = {6U, 12U, 18U, 24U, 60U, 252U};

static __NO_INIT volatile uint32_t s_rx_seed6_last_case_index;
static __NO_INIT volatile uint32_t s_rx_seed6_length;
static __NO_INIT volatile uint32_t s_rx_seed6_stage;
static __NO_INIT volatile int32_t s_rx_seed6_result;
static __NO_INIT volatile uint32_t s_rx_seed6_expected_wake_count;
static __NO_INIT volatile uint32_t s_rx_seed6_wake_count;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_seed_bytes;
static __NO_INIT volatile uint32_t s_rx_seed6_smartdma_tail_bytes;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_inta_count;
static __NO_INIT volatile uint32_t s_rx_seed6_frame_bytes;
static __NO_INIT volatile uint32_t s_rx_seed6_data_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed6_protocol_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed6_ibi_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed6_mstatus;
static __NO_INIT volatile uint32_t s_rx_seed6_merrwarn;
static __NO_INIT volatile uint32_t s_rx_seed6_mdatactrl;
static __NO_INIT volatile uint32_t s_rx_seed6_mdmactrl;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_active;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_inta;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_ctlstat;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_xfercfg;
static __NO_INIT volatile uint32_t s_rx_seed6_dma_errint;
static __NO_INIT volatile uint32_t s_rx_seed6_rxcount;
static __NO_INIT volatile uint32_t s_rx_seed6_buffer_sample[I3C_RX_SEED6_SAMPLE_BYTES];
static __NO_INIT volatile uint32_t s_rx_seed6_case_length[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile int32_t s_rx_seed6_case_result[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_expected_wakes[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_wakes[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_dma_seed_bytes[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_smartdma_tail_bytes[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_frame_bytes[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_data_irq_delta[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_protocol_irq_delta[I3C_RX_SEED6_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed6_case_ibi_irq_delta[I3C_RX_SEED6_CASE_COUNT];
static volatile uint32_t s_rx_seed6_precleanup_valid;
static volatile uint32_t s_rx_seed6_precleanup_mstatus;
static volatile uint32_t s_rx_seed6_precleanup_merrwarn;
static volatile uint32_t s_rx_seed6_precleanup_mdatactrl;
static volatile uint32_t s_rx_seed6_precleanup_mdmactrl;
static volatile uint32_t s_rx_seed6_precleanup_dma_active;
static volatile uint32_t s_rx_seed6_precleanup_dma_inta;
static volatile uint32_t s_rx_seed6_precleanup_dma_ctlstat;
static volatile uint32_t s_rx_seed6_precleanup_dma_xfercfg;
static volatile uint32_t s_rx_seed6_precleanup_dma_errint;
static volatile uint32_t s_rx_seed6_precleanup_rxcount;

static void clear_rx_seed6_probe_state(void)
{
    s_rx_seed6_last_case_index = 0U;
    s_rx_seed6_length = 0U;
    s_rx_seed6_stage = kRxSeed6StageCleared;
    s_rx_seed6_result = (int32_t)kStatus_Success;
    s_rx_seed6_expected_wake_count = 0U;
    s_rx_seed6_wake_count = 0U;
    s_rx_seed6_dma_seed_bytes = 0U;
    s_rx_seed6_smartdma_tail_bytes = 0U;
    s_rx_seed6_dma_inta_count = 0U;
    s_rx_seed6_frame_bytes = 0U;
    s_rx_seed6_data_irq_delta = 0U;
    s_rx_seed6_protocol_irq_delta = 0U;
    s_rx_seed6_ibi_irq_delta = 0U;
    s_rx_seed6_mstatus = 0U;
    s_rx_seed6_merrwarn = 0U;
    s_rx_seed6_mdatactrl = 0U;
    s_rx_seed6_mdmactrl = 0U;
    s_rx_seed6_dma_active = 0U;
    s_rx_seed6_dma_inta = 0U;
    s_rx_seed6_dma_ctlstat = 0U;
    s_rx_seed6_dma_xfercfg = 0U;
    s_rx_seed6_dma_errint = 0U;
    s_rx_seed6_rxcount = 0U;
    memset((void *)s_rx_seed6_buffer_sample, 0, sizeof(s_rx_seed6_buffer_sample));
    memset((void *)s_rx_seed6_case_length, 0, sizeof(s_rx_seed6_case_length));
    memset((void *)s_rx_seed6_case_result, 0, sizeof(s_rx_seed6_case_result));
    memset((void *)s_rx_seed6_case_expected_wakes, 0, sizeof(s_rx_seed6_case_expected_wakes));
    memset((void *)s_rx_seed6_case_wakes, 0, sizeof(s_rx_seed6_case_wakes));
    memset((void *)s_rx_seed6_case_dma_seed_bytes, 0, sizeof(s_rx_seed6_case_dma_seed_bytes));
    memset((void *)s_rx_seed6_case_smartdma_tail_bytes, 0, sizeof(s_rx_seed6_case_smartdma_tail_bytes));
    memset((void *)s_rx_seed6_case_frame_bytes, 0, sizeof(s_rx_seed6_case_frame_bytes));
    memset((void *)s_rx_seed6_case_data_irq_delta, 0, sizeof(s_rx_seed6_case_data_irq_delta));
    memset((void *)s_rx_seed6_case_protocol_irq_delta, 0, sizeof(s_rx_seed6_case_protocol_irq_delta));
    memset((void *)s_rx_seed6_case_ibi_irq_delta, 0, sizeof(s_rx_seed6_case_ibi_irq_delta));
    s_rx_seed6_precleanup_valid = 0U;
    s_rx_seed6_precleanup_mstatus = 0U;
    s_rx_seed6_precleanup_merrwarn = 0U;
    s_rx_seed6_precleanup_mdatactrl = 0U;
    s_rx_seed6_precleanup_mdmactrl = 0U;
    s_rx_seed6_precleanup_dma_active = 0U;
    s_rx_seed6_precleanup_dma_inta = 0U;
    s_rx_seed6_precleanup_dma_ctlstat = 0U;
    s_rx_seed6_precleanup_dma_xfercfg = 0U;
    s_rx_seed6_precleanup_dma_errint = 0U;
    s_rx_seed6_precleanup_rxcount = 0U;
}

static void clear_dma0_rx_channel_state(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED6_DMA_CHANNEL);

    DMA0->COMMON[0].INTA = channelMask;
    DMA0->COMMON[0].INTB = channelMask;
    DMA0->COMMON[0].ERRINT = channelMask;
    DMA0->COMMON[0].INTENCLR = channelMask;
    DMA0->COMMON[0].ENABLECLR = channelMask;
}

static void capture_rx_seed6_buffer_sample(size_t length)
{
    for (size_t index = 0U; index < ARRAY_SIZE(s_rx_seed6_buffer_sample); index++)
    {
        s_rx_seed6_buffer_sample[index] = (index < length) ? s_logical_rx_buffer[index] : 0U;
    }
}

static void capture_rx_seed6_precleanup_state(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED6_DMA_CHANNEL);

    s_rx_seed6_precleanup_valid = 1U;
    s_rx_seed6_precleanup_mstatus = base->MSTATUS;
    s_rx_seed6_precleanup_merrwarn = base->MERRWARN;
    s_rx_seed6_precleanup_mdatactrl = base->MDATACTRL;
    s_rx_seed6_precleanup_mdmactrl = base->MDMACTRL;
    s_rx_seed6_precleanup_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
    s_rx_seed6_precleanup_dma_inta = DMA0->COMMON[0].INTA & channelMask;
    s_rx_seed6_precleanup_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED6_DMA_CHANNEL].CTLSTAT;
    s_rx_seed6_precleanup_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED6_DMA_CHANNEL].XFERCFG;
    s_rx_seed6_precleanup_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
    s_rx_seed6_precleanup_rxcount =
        (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
}

static void record_rx_seed6_case_summary(size_t caseIndex,
                                         size_t length,
                                         int32_t result,
                                         uint32_t dataIrqDelta,
                                         uint32_t protocolIrqDelta,
                                         uint32_t ibiIrqDelta)
{
    if (caseIndex >= ARRAY_SIZE(s_rx_seed6_case_length))
    {
        return;
    }

    s_rx_seed6_case_length[caseIndex] = (uint32_t)length;
    s_rx_seed6_case_result[caseIndex] = result;
    s_rx_seed6_case_expected_wakes[caseIndex] = s_rx_seed6_param.expectedWakeCount;
    s_rx_seed6_case_wakes[caseIndex] = s_rx_seed6_param.wakeCount;
    s_rx_seed6_case_dma_seed_bytes[caseIndex] = s_rx_seed6_param.dmaSeedBytes;
    s_rx_seed6_case_smartdma_tail_bytes[caseIndex] = s_rx_seed6_param.smartdmaTailBytes;
    s_rx_seed6_case_frame_bytes[caseIndex] = s_rx_seed6_param.dmaSeedBytes + s_rx_seed6_param.smartdmaTailBytes;
    s_rx_seed6_case_data_irq_delta[caseIndex] = dataIrqDelta;
    s_rx_seed6_case_protocol_irq_delta[caseIndex] = protocolIrqDelta;
    s_rx_seed6_case_ibi_irq_delta[caseIndex] = ibiIrqDelta;
}

static void capture_rx_seed6_snapshot(I3C_Type *base,
                                      size_t caseIndex,
                                      size_t length,
                                      uint32_t stage,
                                      int32_t result,
                                      uint32_t dataIrqDelta,
                                      uint32_t protocolIrqDelta,
                                      uint32_t ibiIrqDelta)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED6_DMA_CHANNEL);

    s_rx_seed6_last_case_index = (uint32_t)caseIndex;
    s_rx_seed6_length = (uint32_t)length;
    s_rx_seed6_stage = stage;
    s_rx_seed6_result = result;
    s_rx_seed6_expected_wake_count = s_rx_seed6_param.expectedWakeCount;
    s_rx_seed6_wake_count = s_rx_seed6_param.wakeCount;
    s_rx_seed6_dma_seed_bytes = s_rx_seed6_param.dmaSeedBytes;
    s_rx_seed6_smartdma_tail_bytes = s_rx_seed6_param.smartdmaTailBytes;
    s_rx_seed6_dma_inta_count = s_rx_seed6_param.dmaIntaCount;
    s_rx_seed6_frame_bytes = s_rx_seed6_param.dmaSeedBytes + s_rx_seed6_param.smartdmaTailBytes;
    s_rx_seed6_data_irq_delta = dataIrqDelta;
    s_rx_seed6_protocol_irq_delta = protocolIrqDelta;
    s_rx_seed6_ibi_irq_delta = ibiIrqDelta;
    if (s_rx_seed6_precleanup_valid != 0U)
    {
        s_rx_seed6_mstatus = s_rx_seed6_precleanup_mstatus;
        s_rx_seed6_merrwarn = s_rx_seed6_precleanup_merrwarn;
        s_rx_seed6_mdatactrl = s_rx_seed6_precleanup_mdatactrl;
        s_rx_seed6_mdmactrl = s_rx_seed6_precleanup_mdmactrl;
        s_rx_seed6_dma_active = s_rx_seed6_precleanup_dma_active;
        s_rx_seed6_dma_inta = s_rx_seed6_precleanup_dma_inta;
        s_rx_seed6_dma_ctlstat = s_rx_seed6_precleanup_dma_ctlstat;
        s_rx_seed6_dma_xfercfg = s_rx_seed6_precleanup_dma_xfercfg;
        s_rx_seed6_dma_errint = s_rx_seed6_precleanup_dma_errint;
        s_rx_seed6_rxcount = s_rx_seed6_precleanup_rxcount;
    }
    else
    {
        s_rx_seed6_mstatus = base->MSTATUS;
        s_rx_seed6_merrwarn = base->MERRWARN;
        s_rx_seed6_mdatactrl = base->MDATACTRL;
        s_rx_seed6_mdmactrl = base->MDMACTRL;
        s_rx_seed6_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
        s_rx_seed6_dma_inta = DMA0->COMMON[0].INTA & channelMask;
        s_rx_seed6_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED6_DMA_CHANNEL].CTLSTAT;
        s_rx_seed6_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED6_DMA_CHANNEL].XFERCFG;
        s_rx_seed6_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
        s_rx_seed6_rxcount =
            (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
    }
    capture_rx_seed6_buffer_sample(length);
}

static status_t wait_for_ctrl_done_polling(I3C_Type *base)
{
    volatile uint32_t timeout = 0U;

    while (++timeout < I3C_DMA_SEED_CHAIN_TIMEOUT)
    {
        uint32_t status = I3C_MasterGetStatusFlags(base);
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        service_transfer_led();

        if ((status & (uint32_t)kI3C_MasterControlDoneFlag) != 0U)
        {
            I3C_MasterClearStatusFlags(base, (uint32_t)kI3C_MasterControlDoneFlag);
            if (errStatus != 0U)
            {
                I3C_MasterClearErrorStatusFlags(base, errStatus);
            }
            return kStatus_Success;
        }

        if (errStatus != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }
    }

    return kStatus_Timeout;
}

static status_t wait_for_complete_polling(I3C_Type *base)
{
    volatile uint32_t timeout = 0U;

    while (++timeout < I3C_DMA_SEED_CHAIN_TIMEOUT)
    {
        uint32_t status = I3C_MasterGetStatusFlags(base);
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        service_transfer_led();

        if ((status & (uint32_t)kI3C_MasterCompleteFlag) != 0U)
        {
            I3C_MasterClearStatusFlags(base, (uint32_t)kI3C_MasterCompleteFlag);
            if (errStatus != 0U)
            {
                I3C_MasterClearErrorStatusFlags(base, errStatus);
            }
            return kStatus_Success;
        }

        if (errStatus != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }
    }

    return kStatus_Timeout;
}

static status_t wait_for_rx_seed6_mailbox(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED6_DMA_CHANNEL);
    volatile uint32_t timeout = 0U;

    while (++timeout < I3C_DMA_SEED_CHAIN_TIMEOUT)
    {
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);

        service_transfer_led();

        if ((DMA0->COMMON[0].ERRINT & channelMask) != 0U)
        {
            DMA0->COMMON[0].ERRINT = channelMask;
            return kStatus_Fail;
        }

        if (errStatus != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }

        if (s_rx_seed6_param.mailbox == I3C_RX_SEED6_MAILBOX_COMPLETION)
        {
            return kStatus_Success;
        }
    }

    return kStatus_Timeout;
}

static void prepare_rx_seed6_tail5_controller(I3C_Type *base)
{
    CLOCK_EnableClock(I3C_RX_SEED6_DMA_CLOCK);
    RESET_PeripheralReset(I3C_RX_SEED6_DMA_RESET);
    DMA0->CTRL = DMA_CTRL_ENABLE(1U);
    DMA0->SRAMBASE = (uint32_t)(uintptr_t)s_dma_descriptor_table;
    memset((void *)s_dma_descriptor_table, 0, sizeof(s_dma_descriptor_table));
    memset((void *)s_rx_seed6_dma_chain, 0, sizeof(s_rx_seed6_dma_chain));
    memset((void *)&s_rx_seed6_param, 0, sizeof(s_rx_seed6_param));

    clear_dma0_rx_channel_state();
    I3C_MasterEnableDMA(base, false, false, 1U);
    I3C_MasterDisableInterrupts(base,
                                I3C_PROTOCOL_IRQ_MASK | (uint32_t)kI3C_MasterTxReadyFlag |
                                    (uint32_t)kI3C_MasterRxReadyFlag);

    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, SMART_DMA_TRIGGER_CHANNEL, kINPUTMUX_Dma0IrqToSmartDmaInput);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED6_DMA_INPUTMUX_SIGNAL, true);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);

    DMA0->CHANNEL[I3C_RX_SEED6_DMA_CHANNEL].CFG = DMA_CHANNEL_CFG_PERIPHREQEN(1U);
    NVIC_ClearPendingIRQ(I3C_RX_SEED6_DMA_IRQ);
    NVIC_DisableIRQ(I3C_RX_SEED6_DMA_IRQ);
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_DisableIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);
    s_i3c_irq_status_latched = 0U;

    SMARTDMA_Init(
        SMARTDMA_SRAM_ADDR, __smartdma_start__, (uint32_t)((uintptr_t)__smartdma_end__ - (uintptr_t)__smartdma_start__));
    SMARTDMA_Reset();
}

static status_t configure_rx_seed6_descriptor_chain(I3C_Type *base, size_t length)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED6_DMA_CHANNEL);
    const uint32_t oneByteXferCfgBase = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_SETINTA(1U) |
                                        DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                                        DMA_CHANNEL_XFERCFG_DSTINC(1U) | DMA_CHANNEL_XFERCFG_XFERCOUNT(0U);
    const size_t blockCount = length / I3C_RX_SEED6_BLOCK_BYTES;
    const void *srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
    dma_descriptor_t *bootstrapDescriptor = &s_dma_descriptor_table[I3C_RX_SEED6_DMA_CHANNEL];

    if ((length == 0U) || ((length % I3C_RX_SEED6_BLOCK_BYTES) != 0U) || (blockCount > I3C_RX_SEED6_MAX_BLOCKS))
    {
        return kStatus_InvalidArgument;
    }

    DMA0->COMMON[0].INTENSET = channelMask;
    DMA0->COMMON[0].ENABLESET = channelMask;

    bootstrapDescriptor->xfercfg = oneByteXferCfgBase | DMA_CHANNEL_XFERCFG_RELOAD((blockCount > 1U) ? 1U : 0U);
    bootstrapDescriptor->srcEndAddr = srcEndAddr;
    bootstrapDescriptor->dstEndAddr = &s_logical_rx_buffer[0];
    bootstrapDescriptor->linkToNextDesc = (blockCount > 1U) ? &s_rx_seed6_dma_chain[0] : NULL;

    for (size_t blockIndex = 1U; blockIndex < blockCount; blockIndex++)
    {
        dma_descriptor_t *descriptor = &s_rx_seed6_dma_chain[blockIndex - 1U];
        const bool isLast = (blockIndex == (blockCount - 1U));

        descriptor->xfercfg = oneByteXferCfgBase | DMA_CHANNEL_XFERCFG_RELOAD(isLast ? 0U : 1U);
        descriptor->srcEndAddr = srcEndAddr;
        descriptor->dstEndAddr = &s_logical_rx_buffer[blockIndex * I3C_RX_SEED6_BLOCK_BYTES];
        descriptor->linkToNextDesc = isLast ? NULL : &s_rx_seed6_dma_chain[blockIndex];
    }

    s_rx_seed6_param.expectedWakeCount = (uint32_t)blockCount;
    s_rx_seed6_param.nextRxByteAddress = (uint32_t)(uintptr_t)&s_logical_rx_buffer[I3C_RX_SEED6_SEED_BYTES];
    s_rx_seed6_param.remainingCount = (uint32_t)(blockCount * I3C_RX_SEED6_TAIL_BYTES);
    s_rx_seed6_param.i3cBaseAddress = (uint32_t)(uintptr_t)base;
    s_rx_seed6_param.dmaIntaAddress = (uint32_t)(uintptr_t)&DMA0->COMMON[0].INTA;
    s_rx_seed6_param.dmaChannelMask = channelMask;
    return kStatus_Success;
}

static void arm_rx_seed6_descriptor_chain(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED6_DMA_CHANNEL);

    DMA0->CHANNEL[I3C_RX_SEED6_DMA_CHANNEL].XFERCFG = s_dma_descriptor_table[I3C_RX_SEED6_DMA_CHANNEL].xfercfg;
    DMA0->COMMON[0].ENABLESET = channelMask;
    DMA0->COMMON[0].SETVALID = channelMask;
}

static status_t write_logical_payload_blocking(I3C_Type *base, uint8_t slaveAddr, size_t length)
{
    i3c_master_transfer_t masterXfer;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;
    masterXfer.data = s_logical_tx_buffer;
    masterXfer.dataSize = (uint16_t)length;

    return I3C_MasterTransferBlocking(base, &masterXfer);
}

static status_t finish_rx_seed6_read(I3C_Type *base)
{
    status_t result;

    I3C_MasterEnableDMA(base, false, false, 1U);
    result = I3C_MasterStop(base);
    if (result == kStatus_Success)
    {
        result = wait_for_ctrl_done_polling(base);
        if (result != kStatus_Success)
        {
            return result;
        }
    }
    else if ((result != kStatus_I3C_InvalidReq) || (I3C_MasterGetState(base) != kI3C_MasterStateIdle))
    {
        return result;
    }

    clear_dma0_rx_channel_state();
    SMARTDMA_Reset();
    return ensure_master_idle(base);
}

static status_t run_rx_seed6_tail5_read(I3C_Type *base, size_t length)
{
    const i3c_tx_trigger_level_t savedTxTriggerLevel =
        (i3c_tx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_TXTRIG_MASK) >> I3C_MDATACTRL_TXTRIG_SHIFT);
    const i3c_rx_trigger_level_t savedRxTriggerLevel =
        (i3c_rx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_RXTRIG_MASK) >> I3C_MDATACTRL_RXTRIG_SHIFT);
    status_t result;

    s_rx_seed6_precleanup_valid = 0U;
    prepare_rx_seed6_tail5_controller(base);

    I3C_MasterClearErrorStatusFlags(base, I3C_MasterGetErrorStatusFlags(base));
    I3C_MasterClearStatusFlags(base,
                               (uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag |
                                   (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag |
                                   (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag);
    base->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    base->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, kI3C_RxTriggerUntilThreeQuarterOrMore, false, false);

    result = configure_rx_seed6_descriptor_chain(base, length);
    if (result != kStatus_Success)
    {
        I3C_MasterSetWatermarks(base, savedTxTriggerLevel, savedRxTriggerLevel, false, false);
        clear_dma0_rx_channel_state();
        SMARTDMA_Reset();
        return result;
    }

    SMARTDMA_Boot(I3C_RX_SEED6_API_INDEX, &s_rx_seed6_param, 0);
    I3C_MasterEnableDMA(base, false, true, 1U);
    arm_rx_seed6_descriptor_chain();

    result = I3C_MasterStartWithRxSize(base, kI3C_TypeI3CSdr, I3C_TARGET_DYNAMIC_ADDR, kI3C_Read, (uint8_t)length);
    if (result == kStatus_Success)
    {
        result = wait_for_rx_seed6_mailbox(base);
    }
    if (result == kStatus_Success)
    {
        result = wait_for_complete_polling(base);
    }
    if (result == kStatus_Success)
    {
        result = finish_rx_seed6_read(base);
    }

    if (result != kStatus_Success)
    {
        capture_rx_seed6_precleanup_state(base);
    }

    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, savedRxTriggerLevel, false, false);
    I3C_MasterEnableDMA(base, false, false, 1U);
    clear_dma0_rx_channel_state();
    SMARTDMA_Reset();
    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED6_DMA_INPUTMUX_SIGNAL, false);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);

    return result;
}

static status_t run_rx_seed6_case(I3C_Type *base, uint8_t slaveAddr, size_t caseIndex, size_t length)
{
    const uint32_t dataIrqBase = s_cm33_i3c_data_irq_count;
    const uint32_t protocolIrqBase = s_cm33_i3c_protocol_irq_count;
    const uint32_t ibiIrqBase = s_cm33_i3c_ibi_irq_count;
    const uint32_t expectedWakeCount = (uint32_t)(length / I3C_RX_SEED6_BLOCK_BYTES);
    uint32_t dataIrqDelta = 0U;
    uint32_t protocolIrqDelta = 0U;
    uint32_t ibiIrqDelta = 0U;
    status_t result;

    s_active_transfer_length = length;
    prepare_logical_payload(length);
    memset(s_logical_rx_buffer, 0, sizeof(s_logical_rx_buffer));
    clear_ibi_state();
    clear_post_ibi_handoff_snapshot();
    begin_transfer_led();

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorWrite, result, 0U, 0U, 0U);
        record_rx_seed6_case_summary(caseIndex, length, result, 0U, 0U, 0U);
        end_transfer_led(false);
        return result;
    }

    capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageWriteStarted, kStatus_Success, 0U, 0U, 0U);
    result = write_logical_payload_blocking(base, slaveAddr, length);
    if (result != kStatus_Success)
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorWrite, result, 0U, 0U, 0U);
        record_rx_seed6_case_summary(caseIndex, length, result, 0U, 0U, 0U);
        end_transfer_led(false);
        return result;
    }

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorWrite, result, 0U, 0U, 0U);
        record_rx_seed6_case_summary(caseIndex, length, result, 0U, 0U, 0U);
        end_transfer_led(false);
        return result;
    }

    SDK_DelayAtLeastUs(I3C_RX_SEED6_POST_WRITE_SETTLE_US, SystemCoreClock);

    capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageReadStarted, kStatus_Success, 0U, 0U, 0U);
    result = run_rx_seed6_tail5_read(base, length);

    dataIrqDelta = s_cm33_i3c_data_irq_count - dataIrqBase;
    protocolIrqDelta = s_cm33_i3c_protocol_irq_count - protocolIrqBase;
    ibiIrqDelta = s_cm33_i3c_ibi_irq_count - ibiIrqBase;

    if (result != kStatus_Success)
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorMailbox, result, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed6_case_summary(caseIndex, length, result, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return result;
    }

    capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageI3cComplete, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);

    if (!buffers_match(s_logical_tx_buffer, s_logical_rx_buffer, length))
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorCompare, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed6_case_summary(caseIndex, length, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return kStatus_Fail;
    }

    if ((dataIrqDelta != 0U) || (protocolIrqDelta != 0U) || (ibiIrqDelta != 0U))
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorIrq, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed6_case_summary(caseIndex, length, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return kStatus_Fail;
    }

    if ((s_rx_seed6_param.mailbox != I3C_RX_SEED6_MAILBOX_COMPLETION) ||
        (s_rx_seed6_param.expectedWakeCount != expectedWakeCount) ||
        (s_rx_seed6_param.wakeCount != expectedWakeCount) ||
        (s_rx_seed6_param.dmaSeedBytes != expectedWakeCount) ||
        (s_rx_seed6_param.smartdmaTailBytes != (expectedWakeCount * I3C_RX_SEED6_TAIL_BYTES)) ||
        (base->MERRWARN != 0U))
    {
        capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageErrorMetrics, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed6_case_summary(caseIndex, length, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return kStatus_Fail;
    }

    capture_rx_seed6_snapshot(base, caseIndex, length, kRxSeed6StageValidated, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
    record_rx_seed6_case_summary(caseIndex, length, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);

    PRINTF("rx-seed6-tail5 len=%lu wakes=%lu seeds=%lu tails=%lu frame=%lu dataIrq=%lu protocolIrq=%lu\r\n",
           (unsigned long)length,
           (unsigned long)s_rx_seed6_param.wakeCount,
           (unsigned long)s_rx_seed6_param.dmaSeedBytes,
           (unsigned long)s_rx_seed6_param.smartdmaTailBytes,
           (unsigned long)(s_rx_seed6_param.dmaSeedBytes + s_rx_seed6_param.smartdmaTailBytes),
           (unsigned long)dataIrqDelta,
           (unsigned long)protocolIrqDelta);
    EXP_LOG_INFO("rx-seed6-tail5 len=%lu wakes=%lu seeds=%lu tails=%lu frame=%lu dataIrq=%lu protocolIrq=%lu",
                 (unsigned long)length,
                 (unsigned long)s_rx_seed6_param.wakeCount,
                 (unsigned long)s_rx_seed6_param.dmaSeedBytes,
                 (unsigned long)s_rx_seed6_param.smartdmaTailBytes,
                 (unsigned long)(s_rx_seed6_param.dmaSeedBytes + s_rx_seed6_param.smartdmaTailBytes),
                 (unsigned long)dataIrqDelta,
                 (unsigned long)protocolIrqDelta);

    end_transfer_led(true);
    return kStatus_Success;
}

static status_t run_i3c_rx_seed6_tail5_no_cpu_irq_probe(I3C_Type *base, uint8_t slaveAddr)
{
    status_t result = kStatus_Success;

    clear_rx_seed6_probe_state();

    for (size_t caseIndex = 0U; caseIndex < ARRAY_SIZE(s_rx_seed6_lengths); caseIndex++)
    {
        result = run_rx_seed6_case(base, slaveAddr, caseIndex, s_rx_seed6_lengths[caseIndex]);
        if (result != kStatus_Success)
        {
            return result;
        }
    }

    capture_rx_seed6_snapshot(base,
                              ARRAY_SIZE(s_rx_seed6_lengths) - 1U,
                              s_rx_seed6_lengths[ARRAY_SIZE(s_rx_seed6_lengths) - 1U],
                              kRxSeed6StageSuccess,
                              kStatus_Success,
                              0U,
                              0U,
                              0U);
    return kStatus_Success;
}

int main(void)
{
    i3c_master_config_t masterConfig;
    status_t result;
    uint8_t slaveAddr = 0U;

    BOARD_InitHardware();
    init_transfer_led();

    for (volatile uint32_t startupDelay = 0U; startupDelay < I3C_DMA_SEED_CHAIN_STARTUP_WAIT; startupDelay++)
    {
        __NOP();
    }

    PRINTF("\r\nI3C RX seed6-tail5 no-CPU-IRQ probe -- master.\r\n");
    EXP_LOG_INFO("I3C RX seed6-tail5 no-CPU-IRQ probe -- master.");

    I3C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Hz.i2cBaud = I3C_DMA_SEED_CHAIN_I2C_BAUDRATE;
    masterConfig.baudRate_Hz.i3cPushPullBaud = I3C_DMA_SEED_CHAIN_I3C_PP_BAUDRATE;
    masterConfig.baudRate_Hz.i3cOpenDrainBaud = I3C_DMA_SEED_CHAIN_I3C_OD_BAUDRATE;
    masterConfig.enableOpenDrainStop = false;
    I3C_MasterInit(EXAMPLE_MASTER, &masterConfig, I3C_MASTER_CLOCK_FREQUENCY);

    result = run_cpu_rstdaa_and_daa(EXAMPLE_MASTER, &slaveAddr);
    if (result != kStatus_Success)
    {
        EXP_LOG_ERROR("DAA setup failed: %d", result);
        set_failure_led();
        return -1;
    }

    result = run_i3c_rx_seed6_tail5_no_cpu_irq_probe(EXAMPLE_MASTER, slaveAddr);
    if (result != kStatus_Success)
    {
        EXP_LOG_ERROR("I3C RX seed6-tail5 no-CPU-IRQ probe failed: %d", result);
        dump_debug_state(EXAMPLE_MASTER);
        set_failure_led();
        return -1;
    }

    PRINTF("I3C RX seed6-tail5 no-CPU-IRQ probe successful.\r\n");
    EXP_LOG_INFO("I3C RX seed6-tail5 no-CPU-IRQ probe successful.");

    set_success_led();
    return 0;
}