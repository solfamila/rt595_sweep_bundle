/*
 * Controlled seed-only RX matrix for the SmartDMA wake path:
 *  - DMA0 CH24 still moves exactly one seed byte from MRDATAB
 *  - DMA0 IRQ still wakes SmartDMA and SmartDMA still does not read any tail bytes
 *  - CM33 must not service RXREADY/TXREADY data IRQs
 *  - only the read length and RXTRIG geometry vary between cases
 */

#define main master_i3c_sdma_seed_tail_len_sweep_unused_main
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_test_standalone.c"
#undef main

#define I3C_RX_SEED_MATRIX_CASE_COUNT 4U
#define I3C_RX_SEED_MATRIX_DMA_CHANNEL 24U
#define I3C_RX_SEED_MATRIX_DMA_CLOCK kCLOCK_Dmac0
#define I3C_RX_SEED_MATRIX_DMA_RESET kDMAC0_RST_SHIFT_RSTn
#define I3C_RX_SEED_MATRIX_DMA_IRQ DMA0_IRQn
#define I3C_RX_SEED_MATRIX_DMA_INPUTMUX_SIGNAL kINPUTMUX_I3c0RxToDmac0Ch24RequestEna
#define I3C_RX_SEED_MATRIX_SMARTDMA_API_INDEX 2U
#define I3C_RX_SEED_MATRIX_POST_WRITE_SETTLE_US 1000U
#define I3C_RX_SEED_MATRIX_MAILBOX_COMPLETION 1U

uint8_t *g_txBuff = NULL;
uint32_t g_txSize = 0U;
volatile bool g_slaveIbiRequestSent = false;
volatile bool g_slavePostIbiAddressMatched = false;
volatile bool g_slavePostIbiEchoPending = false;
volatile bool g_slavePostIbiEchoArmed = false;

typedef struct _i3c_rx_seed_matrix_param
{
    volatile uint32_t mailbox;
    uint32_t expectedWakeCount;
    volatile uint32_t wakeCount;
    volatile uint32_t dmaSeedBytes;
    volatile uint32_t smartdmaBytes;
    volatile uint32_t dmaIntaCount;
    uint32_t nextRxByteAddress;
    uint32_t remainingCount;
    uint32_t i3cBaseAddress;
    uint32_t dmaIntaAddress;
    uint32_t dmaChannelMask;
} i3c_rx_seed_matrix_param_t;

typedef struct _rx_seed_matrix_case
{
    uint16_t length;
    i3c_rx_trigger_level_t rxTrigger;
} rx_seed_matrix_case_t;

typedef enum _rx_seed_matrix_stage
{
    kRxSeedMatrixStageCleared = 0U,
    kRxSeedMatrixStageWriteStarted = 1U,
    kRxSeedMatrixStageReadStarted = 2U,
    kRxSeedMatrixStageMailbox = 3U,
    kRxSeedMatrixStageSuccess = 4U,
    kRxSeedMatrixStageErrorWrite = 0x201U,
    kRxSeedMatrixStageErrorMailbox = 0x202U,
    kRxSeedMatrixStageErrorIrq = 0x203U,
    kRxSeedMatrixStageErrorMetrics = 0x204U,
} rx_seed_matrix_stage_t;

static const rx_seed_matrix_case_t s_rx_seed_matrix_cases[I3C_RX_SEED_MATRIX_CASE_COUNT] = {
    {4U, kI3C_RxTriggerOnNotEmpty},
    {4U, kI3C_RxTriggerUntilOneHalfOrMore},
    {6U, kI3C_RxTriggerUntilOneHalfOrMore},
    {6U, kI3C_RxTriggerUntilThreeQuarterOrMore},
};

AT_NONCACHEABLE_SECTION_ALIGN(static i3c_rx_seed_matrix_param_t s_rx_seed_matrix_param, 4);

static __NO_INIT volatile uint32_t s_rx_seed_matrix_last_case_index;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_length;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_rx_trigger;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_stage;
static __NO_INIT volatile int32_t s_rx_seed_matrix_result;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_expected_wakes;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_wakes;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_seed_bytes;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_smartdma_bytes;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_inta_count;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_frame_bytes;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_protocol_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_ibi_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_mstatus;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_merrwarn;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_mdatactrl;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_mdmactrl;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_active;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_inta;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_ctlstat;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_xfercfg;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_dma_errint;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_rxcount;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data0;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data1;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data2;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data3;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data4;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_data5;
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_length[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_rx_trigger[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile int32_t s_rx_seed_matrix_case_result[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_expected_wakes[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_wakes[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_dma_seed_bytes[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_dma_inta_count[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_data_irq_delta[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_protocol_irq_delta[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_ibi_irq_delta[I3C_RX_SEED_MATRIX_CASE_COUNT];
static __NO_INIT volatile uint32_t s_rx_seed_matrix_case_rxcount[I3C_RX_SEED_MATRIX_CASE_COUNT];
static volatile uint32_t s_rx_seed_matrix_precleanup_valid;
static volatile uint32_t s_rx_seed_matrix_precleanup_mstatus;
static volatile uint32_t s_rx_seed_matrix_precleanup_merrwarn;
static volatile uint32_t s_rx_seed_matrix_precleanup_mdatactrl;
static volatile uint32_t s_rx_seed_matrix_precleanup_mdmactrl;
static volatile uint32_t s_rx_seed_matrix_precleanup_dma_active;
static volatile uint32_t s_rx_seed_matrix_precleanup_dma_inta;
static volatile uint32_t s_rx_seed_matrix_precleanup_dma_ctlstat;
static volatile uint32_t s_rx_seed_matrix_precleanup_dma_xfercfg;
static volatile uint32_t s_rx_seed_matrix_precleanup_dma_errint;
static volatile uint32_t s_rx_seed_matrix_precleanup_rxcount;

static void clear_rx_seed_matrix_probe_state(void)
{
    s_rx_seed_matrix_last_case_index = 0U;
    s_rx_seed_matrix_length = 0U;
    s_rx_seed_matrix_rx_trigger = 0U;
    s_rx_seed_matrix_stage = kRxSeedMatrixStageCleared;
    s_rx_seed_matrix_result = (int32_t)kStatus_Success;
    s_rx_seed_matrix_expected_wakes = 0U;
    s_rx_seed_matrix_wakes = 0U;
    s_rx_seed_matrix_dma_seed_bytes = 0U;
    s_rx_seed_matrix_smartdma_bytes = 0U;
    s_rx_seed_matrix_dma_inta_count = 0U;
    s_rx_seed_matrix_frame_bytes = 0U;
    s_rx_seed_matrix_data_irq_delta = 0U;
    s_rx_seed_matrix_protocol_irq_delta = 0U;
    s_rx_seed_matrix_ibi_irq_delta = 0U;
    s_rx_seed_matrix_mstatus = 0U;
    s_rx_seed_matrix_merrwarn = 0U;
    s_rx_seed_matrix_mdatactrl = 0U;
    s_rx_seed_matrix_mdmactrl = 0U;
    s_rx_seed_matrix_dma_active = 0U;
    s_rx_seed_matrix_dma_inta = 0U;
    s_rx_seed_matrix_dma_ctlstat = 0U;
    s_rx_seed_matrix_dma_xfercfg = 0U;
    s_rx_seed_matrix_dma_errint = 0U;
    s_rx_seed_matrix_rxcount = 0U;
    s_rx_seed_matrix_data0 = 0U;
    s_rx_seed_matrix_data1 = 0U;
    s_rx_seed_matrix_data2 = 0U;
    s_rx_seed_matrix_data3 = 0U;
    s_rx_seed_matrix_data4 = 0U;
    s_rx_seed_matrix_data5 = 0U;
    memset((void *)s_rx_seed_matrix_case_length, 0, sizeof(s_rx_seed_matrix_case_length));
    memset((void *)s_rx_seed_matrix_case_rx_trigger, 0, sizeof(s_rx_seed_matrix_case_rx_trigger));
    memset((void *)s_rx_seed_matrix_case_result, 0, sizeof(s_rx_seed_matrix_case_result));
    memset((void *)s_rx_seed_matrix_case_expected_wakes, 0, sizeof(s_rx_seed_matrix_case_expected_wakes));
    memset((void *)s_rx_seed_matrix_case_wakes, 0, sizeof(s_rx_seed_matrix_case_wakes));
    memset((void *)s_rx_seed_matrix_case_dma_seed_bytes, 0, sizeof(s_rx_seed_matrix_case_dma_seed_bytes));
    memset((void *)s_rx_seed_matrix_case_dma_inta_count, 0, sizeof(s_rx_seed_matrix_case_dma_inta_count));
    memset((void *)s_rx_seed_matrix_case_data_irq_delta, 0, sizeof(s_rx_seed_matrix_case_data_irq_delta));
    memset((void *)s_rx_seed_matrix_case_protocol_irq_delta, 0, sizeof(s_rx_seed_matrix_case_protocol_irq_delta));
    memset((void *)s_rx_seed_matrix_case_ibi_irq_delta, 0, sizeof(s_rx_seed_matrix_case_ibi_irq_delta));
    memset((void *)s_rx_seed_matrix_case_rxcount, 0, sizeof(s_rx_seed_matrix_case_rxcount));
    memset((void *)&s_rx_seed_matrix_param, 0, sizeof(s_rx_seed_matrix_param));
    s_rx_seed_matrix_precleanup_valid = 0U;
    s_rx_seed_matrix_precleanup_mstatus = 0U;
    s_rx_seed_matrix_precleanup_merrwarn = 0U;
    s_rx_seed_matrix_precleanup_mdatactrl = 0U;
    s_rx_seed_matrix_precleanup_mdmactrl = 0U;
    s_rx_seed_matrix_precleanup_dma_active = 0U;
    s_rx_seed_matrix_precleanup_dma_inta = 0U;
    s_rx_seed_matrix_precleanup_dma_ctlstat = 0U;
    s_rx_seed_matrix_precleanup_dma_xfercfg = 0U;
    s_rx_seed_matrix_precleanup_dma_errint = 0U;
    s_rx_seed_matrix_precleanup_rxcount = 0U;
}

static void clear_dma0_rx_channel_state(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_MATRIX_DMA_CHANNEL);

    DMA0->COMMON[0].INTA = channelMask;
    DMA0->COMMON[0].INTB = channelMask;
    DMA0->COMMON[0].ERRINT = channelMask;
    DMA0->COMMON[0].INTENCLR = channelMask;
    DMA0->COMMON[0].ENABLECLR = channelMask;
}

static uint32_t read_rx_seed_matrix_rxcount(I3C_Type *base)
{
    if (s_rx_seed_matrix_precleanup_valid != 0U)
    {
        return s_rx_seed_matrix_precleanup_rxcount;
    }

    return (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
}

static void capture_rx_seed_matrix_precleanup_state(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_MATRIX_DMA_CHANNEL);

    s_rx_seed_matrix_precleanup_valid = 1U;
    s_rx_seed_matrix_precleanup_mstatus = base->MSTATUS;
    s_rx_seed_matrix_precleanup_merrwarn = base->MERRWARN;
    s_rx_seed_matrix_precleanup_mdatactrl = base->MDATACTRL;
    s_rx_seed_matrix_precleanup_mdmactrl = base->MDMACTRL;
    s_rx_seed_matrix_precleanup_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
    s_rx_seed_matrix_precleanup_dma_inta = DMA0->COMMON[0].INTA & channelMask;
    s_rx_seed_matrix_precleanup_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED_MATRIX_DMA_CHANNEL].CTLSTAT;
    s_rx_seed_matrix_precleanup_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED_MATRIX_DMA_CHANNEL].XFERCFG;
    s_rx_seed_matrix_precleanup_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
    s_rx_seed_matrix_precleanup_rxcount =
        (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
}

static void record_rx_seed_matrix_case_summary(size_t caseIndex,
                                               const rx_seed_matrix_case_t *caseConfig,
                                               int32_t result,
                                               uint32_t dataIrqDelta,
                                               uint32_t protocolIrqDelta,
                                               uint32_t ibiIrqDelta,
                                               uint32_t rxcount)
{
    if (caseIndex >= ARRAY_SIZE(s_rx_seed_matrix_case_length))
    {
        return;
    }

    s_rx_seed_matrix_case_length[caseIndex] = (uint32_t)caseConfig->length;
    s_rx_seed_matrix_case_rx_trigger[caseIndex] = (uint32_t)caseConfig->rxTrigger;
    s_rx_seed_matrix_case_result[caseIndex] = result;
    s_rx_seed_matrix_case_expected_wakes[caseIndex] = s_rx_seed_matrix_param.expectedWakeCount;
    s_rx_seed_matrix_case_wakes[caseIndex] = s_rx_seed_matrix_param.wakeCount;
    s_rx_seed_matrix_case_dma_seed_bytes[caseIndex] = s_rx_seed_matrix_param.dmaSeedBytes;
    s_rx_seed_matrix_case_dma_inta_count[caseIndex] = s_rx_seed_matrix_param.dmaIntaCount;
    s_rx_seed_matrix_case_data_irq_delta[caseIndex] = dataIrqDelta;
    s_rx_seed_matrix_case_protocol_irq_delta[caseIndex] = protocolIrqDelta;
    s_rx_seed_matrix_case_ibi_irq_delta[caseIndex] = ibiIrqDelta;
    s_rx_seed_matrix_case_rxcount[caseIndex] = rxcount;
}

static void capture_rx_seed_matrix_snapshot(I3C_Type *base,
                                            size_t caseIndex,
                                            const rx_seed_matrix_case_t *caseConfig,
                                            uint32_t stage,
                                            int32_t result,
                                            uint32_t dataIrqDelta,
                                            uint32_t protocolIrqDelta,
                                            uint32_t ibiIrqDelta)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_MATRIX_DMA_CHANNEL);

    s_rx_seed_matrix_last_case_index = (uint32_t)caseIndex;
    s_rx_seed_matrix_length = (uint32_t)caseConfig->length;
    s_rx_seed_matrix_rx_trigger = (uint32_t)caseConfig->rxTrigger;
    s_rx_seed_matrix_stage = stage;
    s_rx_seed_matrix_result = result;
    s_rx_seed_matrix_expected_wakes = s_rx_seed_matrix_param.expectedWakeCount;
    s_rx_seed_matrix_wakes = s_rx_seed_matrix_param.wakeCount;
    s_rx_seed_matrix_dma_seed_bytes = s_rx_seed_matrix_param.dmaSeedBytes;
    s_rx_seed_matrix_smartdma_bytes = s_rx_seed_matrix_param.smartdmaBytes;
    s_rx_seed_matrix_dma_inta_count = s_rx_seed_matrix_param.dmaIntaCount;
    s_rx_seed_matrix_frame_bytes = s_rx_seed_matrix_param.dmaSeedBytes + s_rx_seed_matrix_param.smartdmaBytes;
    s_rx_seed_matrix_data_irq_delta = dataIrqDelta;
    s_rx_seed_matrix_protocol_irq_delta = protocolIrqDelta;
    s_rx_seed_matrix_ibi_irq_delta = ibiIrqDelta;
    if (s_rx_seed_matrix_precleanup_valid != 0U)
    {
        s_rx_seed_matrix_mstatus = s_rx_seed_matrix_precleanup_mstatus;
        s_rx_seed_matrix_merrwarn = s_rx_seed_matrix_precleanup_merrwarn;
        s_rx_seed_matrix_mdatactrl = s_rx_seed_matrix_precleanup_mdatactrl;
        s_rx_seed_matrix_mdmactrl = s_rx_seed_matrix_precleanup_mdmactrl;
        s_rx_seed_matrix_dma_active = s_rx_seed_matrix_precleanup_dma_active;
        s_rx_seed_matrix_dma_inta = s_rx_seed_matrix_precleanup_dma_inta;
        s_rx_seed_matrix_dma_ctlstat = s_rx_seed_matrix_precleanup_dma_ctlstat;
        s_rx_seed_matrix_dma_xfercfg = s_rx_seed_matrix_precleanup_dma_xfercfg;
        s_rx_seed_matrix_dma_errint = s_rx_seed_matrix_precleanup_dma_errint;
        s_rx_seed_matrix_rxcount = s_rx_seed_matrix_precleanup_rxcount;
    }
    else
    {
        s_rx_seed_matrix_mstatus = base->MSTATUS;
        s_rx_seed_matrix_merrwarn = base->MERRWARN;
        s_rx_seed_matrix_mdatactrl = base->MDATACTRL;
        s_rx_seed_matrix_mdmactrl = base->MDMACTRL;
        s_rx_seed_matrix_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
        s_rx_seed_matrix_dma_inta = DMA0->COMMON[0].INTA & channelMask;
        s_rx_seed_matrix_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED_MATRIX_DMA_CHANNEL].CTLSTAT;
        s_rx_seed_matrix_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED_MATRIX_DMA_CHANNEL].XFERCFG;
        s_rx_seed_matrix_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
        s_rx_seed_matrix_rxcount =
            (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
    }
    s_rx_seed_matrix_data0 = s_logical_rx_buffer[0];
    s_rx_seed_matrix_data1 = s_logical_rx_buffer[1];
    s_rx_seed_matrix_data2 = s_logical_rx_buffer[2];
    s_rx_seed_matrix_data3 = s_logical_rx_buffer[3];
    s_rx_seed_matrix_data4 = s_logical_rx_buffer[4];
    s_rx_seed_matrix_data5 = s_logical_rx_buffer[5];
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

static status_t wait_for_rx_seed_matrix_mailbox(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_MATRIX_DMA_CHANNEL);
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

        if (s_rx_seed_matrix_param.mailbox == I3C_RX_SEED_MATRIX_MAILBOX_COMPLETION)
        {
            return kStatus_Success;
        }
    }

    return kStatus_Timeout;
}

static void prepare_rx_seed_matrix_controller(I3C_Type *base)
{
    CLOCK_EnableClock(I3C_RX_SEED_MATRIX_DMA_CLOCK);
    RESET_PeripheralReset(I3C_RX_SEED_MATRIX_DMA_RESET);
    DMA0->CTRL = DMA_CTRL_ENABLE(1U);
    DMA0->SRAMBASE = (uint32_t)(uintptr_t)s_dma_descriptor_table;
    memset((void *)s_dma_descriptor_table, 0, sizeof(s_dma_descriptor_table));
    memset((void *)&s_rx_seed_matrix_param, 0, sizeof(s_rx_seed_matrix_param));

    clear_dma0_rx_channel_state();
    I3C_MasterEnableDMA(base, false, false, 1U);
    I3C_MasterDisableInterrupts(base,
                                I3C_PROTOCOL_IRQ_MASK | (uint32_t)kI3C_MasterTxReadyFlag |
                                    (uint32_t)kI3C_MasterRxReadyFlag);

    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, SMART_DMA_TRIGGER_CHANNEL, kINPUTMUX_Dma0IrqToSmartDmaInput);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED_MATRIX_DMA_INPUTMUX_SIGNAL, true);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);

    DMA0->CHANNEL[I3C_RX_SEED_MATRIX_DMA_CHANNEL].CFG = DMA_CHANNEL_CFG_PERIPHREQEN(1U);
    NVIC_ClearPendingIRQ(I3C_RX_SEED_MATRIX_DMA_IRQ);
    NVIC_DisableIRQ(I3C_RX_SEED_MATRIX_DMA_IRQ);
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_DisableIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);
    s_i3c_irq_status_latched = 0U;

    SMARTDMA_Init(
        SMARTDMA_SRAM_ADDR, __smartdma_start__, (uint32_t)((uintptr_t)__smartdma_end__ - (uintptr_t)__smartdma_start__));
    SMARTDMA_Reset();
}

static void configure_rx_seed_matrix_descriptor(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_MATRIX_DMA_CHANNEL);
    dma_descriptor_t *bootstrapDescriptor = &s_dma_descriptor_table[I3C_RX_SEED_MATRIX_DMA_CHANNEL];

    DMA0->COMMON[0].INTENSET = channelMask;

    bootstrapDescriptor->xfercfg = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_SETINTA(1U) |
                                   DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                                   DMA_CHANNEL_XFERCFG_DSTINC(1U) | DMA_CHANNEL_XFERCFG_XFERCOUNT(0U);
    bootstrapDescriptor->srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
    bootstrapDescriptor->dstEndAddr = &s_logical_rx_buffer[0];
    bootstrapDescriptor->linkToNextDesc = NULL;

    s_rx_seed_matrix_param.expectedWakeCount = 1U;
    s_rx_seed_matrix_param.nextRxByteAddress = (uint32_t)(uintptr_t)&s_logical_rx_buffer[1];
    s_rx_seed_matrix_param.remainingCount = 0U;
    s_rx_seed_matrix_param.i3cBaseAddress = (uint32_t)(uintptr_t)base;
    s_rx_seed_matrix_param.dmaIntaAddress = (uint32_t)(uintptr_t)&DMA0->COMMON[0].INTA;
    s_rx_seed_matrix_param.dmaChannelMask = channelMask;
}

static void arm_rx_seed_matrix_descriptor(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_MATRIX_DMA_CHANNEL);

    DMA0->CHANNEL[I3C_RX_SEED_MATRIX_DMA_CHANNEL].XFERCFG = s_dma_descriptor_table[I3C_RX_SEED_MATRIX_DMA_CHANNEL].xfercfg;
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

static status_t finish_rx_seed_matrix_read(I3C_Type *base)
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

static status_t run_rx_seed_matrix_read(I3C_Type *base, const rx_seed_matrix_case_t *caseConfig, uint8_t slaveAddr)
{
    const i3c_tx_trigger_level_t savedTxTriggerLevel =
        (i3c_tx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_TXTRIG_MASK) >> I3C_MDATACTRL_TXTRIG_SHIFT);
    const i3c_rx_trigger_level_t savedRxTriggerLevel =
        (i3c_rx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_RXTRIG_MASK) >> I3C_MDATACTRL_RXTRIG_SHIFT);
    status_t result;

    s_rx_seed_matrix_precleanup_valid = 0U;
    prepare_rx_seed_matrix_controller(base);

    I3C_MasterClearErrorStatusFlags(base, I3C_MasterGetErrorStatusFlags(base));
    I3C_MasterClearStatusFlags(base,
                               (uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag |
                                   (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag |
                                   (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag);
    base->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    base->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, caseConfig->rxTrigger, false, false);

    configure_rx_seed_matrix_descriptor(base);
    SMARTDMA_Boot(I3C_RX_SEED_MATRIX_SMARTDMA_API_INDEX, &s_rx_seed_matrix_param, 0);
    I3C_MasterEnableDMA(base, false, true, 1U);
    arm_rx_seed_matrix_descriptor();

    result = I3C_MasterStartWithRxSize(base, kI3C_TypeI3CSdr, slaveAddr, kI3C_Read, (uint8_t)caseConfig->length);
    if (result == kStatus_Success)
    {
        result = wait_for_rx_seed_matrix_mailbox(base);
    }

    capture_rx_seed_matrix_precleanup_state(base);

    if (result == kStatus_Success)
    {
        result = finish_rx_seed_matrix_read(base);
    }

    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, savedRxTriggerLevel, false, false);
    I3C_MasterEnableDMA(base, false, false, 1U);
    clear_dma0_rx_channel_state();
    SMARTDMA_Reset();
    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED_MATRIX_DMA_INPUTMUX_SIGNAL, false);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);

    return result;
}

static status_t run_rx_seed_matrix_case(I3C_Type *base, uint8_t slaveAddr, size_t caseIndex)
{
    const rx_seed_matrix_case_t *caseConfig = &s_rx_seed_matrix_cases[caseIndex];
    const uint32_t dataIrqBase = s_cm33_i3c_data_irq_count;
    const uint32_t protocolIrqBase = s_cm33_i3c_protocol_irq_count;
    const uint32_t ibiIrqBase = s_cm33_i3c_ibi_irq_count;
    uint32_t dataIrqDelta = 0U;
    uint32_t protocolIrqDelta = 0U;
    uint32_t ibiIrqDelta = 0U;
    status_t result;

    s_active_transfer_length = caseConfig->length;
    prepare_logical_payload(caseConfig->length);
    memset(s_logical_rx_buffer, 0, sizeof(s_logical_rx_buffer));
    clear_ibi_state();
    clear_post_ibi_handoff_snapshot();
    begin_transfer_led();

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageErrorWrite, result, 0U, 0U, 0U);
        record_rx_seed_matrix_case_summary(caseIndex, caseConfig, result, 0U, 0U, 0U, read_rx_seed_matrix_rxcount(base));
        end_transfer_led(false);
        return result;
    }

    capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageWriteStarted, kStatus_Success, 0U, 0U, 0U);
    result = write_logical_payload_blocking(base, slaveAddr, caseConfig->length);
    if (result != kStatus_Success)
    {
        capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageErrorWrite, result, 0U, 0U, 0U);
        record_rx_seed_matrix_case_summary(caseIndex, caseConfig, result, 0U, 0U, 0U, read_rx_seed_matrix_rxcount(base));
        end_transfer_led(false);
        return result;
    }

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageErrorWrite, result, 0U, 0U, 0U);
        record_rx_seed_matrix_case_summary(caseIndex, caseConfig, result, 0U, 0U, 0U, read_rx_seed_matrix_rxcount(base));
        end_transfer_led(false);
        return result;
    }

    SDK_DelayAtLeastUs(I3C_RX_SEED_MATRIX_POST_WRITE_SETTLE_US, SystemCoreClock);

    capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageReadStarted, kStatus_Success, 0U, 0U, 0U);
    result = run_rx_seed_matrix_read(base, caseConfig, slaveAddr);

    dataIrqDelta = s_cm33_i3c_data_irq_count - dataIrqBase;
    protocolIrqDelta = s_cm33_i3c_protocol_irq_count - protocolIrqBase;
    ibiIrqDelta = s_cm33_i3c_ibi_irq_count - ibiIrqBase;

    if (result != kStatus_Success)
    {
        capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageErrorMailbox, result, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed_matrix_case_summary(caseIndex, caseConfig, result, dataIrqDelta, protocolIrqDelta, ibiIrqDelta,
                                           read_rx_seed_matrix_rxcount(base));
        end_transfer_led(false);
        return result;
    }

    capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageMailbox, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);

    if ((dataIrqDelta != 0U) || (protocolIrqDelta != 0U) || (ibiIrqDelta != 0U))
    {
        capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageErrorIrq, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed_matrix_case_summary(caseIndex, caseConfig, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta,
                                           read_rx_seed_matrix_rxcount(base));
        end_transfer_led(false);
        return kStatus_Fail;
    }

    if ((s_rx_seed_matrix_param.mailbox != I3C_RX_SEED_MATRIX_MAILBOX_COMPLETION) ||
        (s_rx_seed_matrix_param.expectedWakeCount != 1U) || (s_rx_seed_matrix_param.wakeCount != 1U) ||
        (s_rx_seed_matrix_param.dmaSeedBytes != 1U) || (s_rx_seed_matrix_param.smartdmaBytes != 0U) ||
        (s_rx_seed_matrix_param.dmaIntaCount != 1U))
    {
        capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageErrorMetrics, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        record_rx_seed_matrix_case_summary(caseIndex, caseConfig, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta,
                                           read_rx_seed_matrix_rxcount(base));
        end_transfer_led(false);
        return kStatus_Fail;
    }

    capture_rx_seed_matrix_snapshot(base, caseIndex, caseConfig, kRxSeedMatrixStageSuccess, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
    record_rx_seed_matrix_case_summary(caseIndex, caseConfig, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta,
                                       read_rx_seed_matrix_rxcount(base));
    PRINTF("rx-seed-matrix case=%lu len=%lu trig=%lu wakes=%lu inta=%lu rxcount=%lu\r\n",
           (unsigned long)caseIndex,
           (unsigned long)caseConfig->length,
           (unsigned long)caseConfig->rxTrigger,
           (unsigned long)s_rx_seed_matrix_param.wakeCount,
           (unsigned long)s_rx_seed_matrix_param.dmaIntaCount,
           (unsigned long)read_rx_seed_matrix_rxcount(base));
    EXP_LOG_INFO("rx-seed-matrix case=%lu len=%lu trig=%lu wakes=%lu inta=%lu rxcount=%lu",
                 (unsigned long)caseIndex,
                 (unsigned long)caseConfig->length,
                 (unsigned long)caseConfig->rxTrigger,
                 (unsigned long)s_rx_seed_matrix_param.wakeCount,
                 (unsigned long)s_rx_seed_matrix_param.dmaIntaCount,
                 (unsigned long)read_rx_seed_matrix_rxcount(base));

    end_transfer_led(true);
    return kStatus_Success;
}

static status_t run_i3c_rx_seed_only_matrix_no_cpu_irq_probe(I3C_Type *base, uint8_t slaveAddr)
{
    status_t result = kStatus_Success;

    clear_rx_seed_matrix_probe_state();

    for (size_t caseIndex = 0U; caseIndex < ARRAY_SIZE(s_rx_seed_matrix_cases); caseIndex++)
    {
        result = run_rx_seed_matrix_case(base, slaveAddr, caseIndex);
        if (result != kStatus_Success)
        {
            return result;
        }
    }

    return result;
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

    PRINTF("\r\nI3C RX seed-only matrix no-CPU-IRQ probe -- master.\r\n");
    EXP_LOG_INFO("I3C RX seed-only matrix no-CPU-IRQ probe -- master.");

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

    result = run_i3c_rx_seed_only_matrix_no_cpu_irq_probe(EXAMPLE_MASTER, slaveAddr);
    if (result != kStatus_Success)
    {
        EXP_LOG_ERROR("I3C RX seed-only matrix no-CPU-IRQ probe failed: %d", result);
        dump_debug_state(EXAMPLE_MASTER);
        set_failure_led();
        return -1;
    }

    PRINTF("I3C RX seed-only matrix no-CPU-IRQ probe successful.\r\n");
    EXP_LOG_INFO("I3C RX seed-only matrix no-CPU-IRQ probe successful.");
    set_success_led();
    return 0;
}