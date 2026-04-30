/*
 * Diagnostic native-RX seed probe:
 *  - read length = 4 bytes
 *  - RXTRIG = on not empty
 *  - DMA0 CH24 moves exactly one seed byte from MRDATAB
 *  - CM33 polls DMA INTA directly instead of routing DMA0 IRQ to SmartDMA
 *  - CM33 still must not service RXREADY/TXREADY data IRQs
 */

#define main master_i3c_sdma_seed_tail_len_sweep_unused_main
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_test_standalone.c"
#undef main

#ifndef I3C_RX_SEED4_PROBE_NAME
#define I3C_RX_SEED4_PROBE_NAME "I3C RX seed4 seed-only DMA INTA poll probe"
#endif

#ifndef I3C_RX_SEED4_DMAFB_MODE
#define I3C_RX_SEED4_DMAFB_MODE 2U
#endif

#ifndef I3C_RX_SEED4_TRACE_LABEL
#define I3C_RX_SEED4_TRACE_LABEL "rx-seed4-dma-inta"
#endif

#ifndef I3C_RX_SEED4_ENABLE_RXREADY_INT
#define I3C_RX_SEED4_ENABLE_RXREADY_INT 0U
#endif

#ifndef I3C_RX_SEED4_KEEP_INPUTMUX_CLOCK_ENABLED
#define I3C_RX_SEED4_KEEP_INPUTMUX_CLOCK_ENABLED 0U
#endif

#define I3C_RX_SEED4_LENGTH 4U
#define I3C_RX_SEED4_DMA_CHANNEL 24U
#define I3C_RX_SEED4_DMA_CLOCK kCLOCK_Dmac0
#define I3C_RX_SEED4_DMA_RESET kDMAC0_RST_SHIFT_RSTn
#define I3C_RX_SEED4_DMA_IRQ DMA0_IRQn
#define I3C_RX_SEED4_DMA_INPUTMUX_SIGNAL kINPUTMUX_I3c0RxToDmac0Ch24RequestEna
#define I3C_RX_SEED4_POST_WRITE_SETTLE_US 1000U

uint8_t *g_txBuff = NULL;
uint32_t g_txSize = 0U;
volatile bool g_slaveIbiRequestSent = false;
volatile bool g_slavePostIbiAddressMatched = false;
volatile bool g_slavePostIbiEchoPending = false;
volatile bool g_slavePostIbiEchoArmed = false;

typedef enum _rx_seed4_stage
{
    kRxSeed4StageCleared = 0U,
    kRxSeed4StageWriteStarted = 1U,
    kRxSeed4StageReadStarted = 2U,
    kRxSeed4StageDmaInta = 3U,
    kRxSeed4StageSuccess = 4U,
    kRxSeed4StageErrorWrite = 0x201U,
    kRxSeed4StageErrorDma = 0x202U,
    kRxSeed4StageErrorIrq = 0x203U,
    kRxSeed4StageErrorMetrics = 0x204U,
} rx_seed4_stage_t;

static __NO_INIT volatile uint32_t s_rx_seed4_stage;
static __NO_INIT volatile int32_t s_rx_seed4_result;
static __NO_INIT volatile uint32_t s_rx_seed4_dma_inta_count;
static __NO_INIT volatile uint32_t s_rx_seed4_data_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed4_protocol_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed4_ibi_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed4_mstatus;
static __NO_INIT volatile uint32_t s_rx_seed4_merrwarn;
static __NO_INIT volatile uint32_t s_rx_seed4_mdatactrl;
static __NO_INIT volatile uint32_t s_rx_seed4_mdmactrl;
static __NO_INIT volatile uint32_t s_rx_seed4_mintset;
static __NO_INIT volatile uint32_t s_rx_seed4_mintmasked;
static __NO_INIT volatile uint32_t s_rx_seed4_inputmux_dmac_req_ena0;
static __NO_INIT volatile uint32_t s_rx_seed4_dma_active;
static __NO_INIT volatile uint32_t s_rx_seed4_dma_inta;
static __NO_INIT volatile uint32_t s_rx_seed4_dma_ctlstat;
static __NO_INIT volatile uint32_t s_rx_seed4_dma_xfercfg;
static __NO_INIT volatile uint32_t s_rx_seed4_dma_errint;
static __NO_INIT volatile uint32_t s_rx_seed4_rxcount;
static __NO_INIT volatile uint32_t s_rx_seed4_data0;
static __NO_INIT volatile uint32_t s_rx_seed4_data1;
static __NO_INIT volatile uint32_t s_rx_seed4_data2;
static __NO_INIT volatile uint32_t s_rx_seed4_data3;
static volatile uint32_t s_rx_seed4_precleanup_valid;
static volatile uint32_t s_rx_seed4_precleanup_mstatus;
static volatile uint32_t s_rx_seed4_precleanup_merrwarn;
static volatile uint32_t s_rx_seed4_precleanup_mdatactrl;
static volatile uint32_t s_rx_seed4_precleanup_mdmactrl;
static volatile uint32_t s_rx_seed4_precleanup_mintset;
static volatile uint32_t s_rx_seed4_precleanup_mintmasked;
static volatile uint32_t s_rx_seed4_precleanup_inputmux_dmac_req_ena0;
static volatile uint32_t s_rx_seed4_precleanup_dma_active;
static volatile uint32_t s_rx_seed4_precleanup_dma_inta;
static volatile uint32_t s_rx_seed4_precleanup_dma_ctlstat;
static volatile uint32_t s_rx_seed4_precleanup_dma_xfercfg;
static volatile uint32_t s_rx_seed4_precleanup_dma_errint;
static volatile uint32_t s_rx_seed4_precleanup_rxcount;

static void clear_rx_seed4_probe_state(void)
{
    s_rx_seed4_stage = kRxSeed4StageCleared;
    s_rx_seed4_result = (int32_t)kStatus_Success;
    s_rx_seed4_dma_inta_count = 0U;
    s_rx_seed4_data_irq_delta = 0U;
    s_rx_seed4_protocol_irq_delta = 0U;
    s_rx_seed4_ibi_irq_delta = 0U;
    s_rx_seed4_mstatus = 0U;
    s_rx_seed4_merrwarn = 0U;
    s_rx_seed4_mdatactrl = 0U;
    s_rx_seed4_mdmactrl = 0U;
    s_rx_seed4_mintset = 0U;
    s_rx_seed4_mintmasked = 0U;
    s_rx_seed4_inputmux_dmac_req_ena0 = 0U;
    s_rx_seed4_dma_active = 0U;
    s_rx_seed4_dma_inta = 0U;
    s_rx_seed4_dma_ctlstat = 0U;
    s_rx_seed4_dma_xfercfg = 0U;
    s_rx_seed4_dma_errint = 0U;
    s_rx_seed4_rxcount = 0U;
    s_rx_seed4_data0 = 0U;
    s_rx_seed4_data1 = 0U;
    s_rx_seed4_data2 = 0U;
    s_rx_seed4_data3 = 0U;
    s_rx_seed4_precleanup_valid = 0U;
    s_rx_seed4_precleanup_mstatus = 0U;
    s_rx_seed4_precleanup_merrwarn = 0U;
    s_rx_seed4_precleanup_mdatactrl = 0U;
    s_rx_seed4_precleanup_mdmactrl = 0U;
    s_rx_seed4_precleanup_mintset = 0U;
    s_rx_seed4_precleanup_mintmasked = 0U;
    s_rx_seed4_precleanup_inputmux_dmac_req_ena0 = 0U;
    s_rx_seed4_precleanup_dma_active = 0U;
    s_rx_seed4_precleanup_dma_inta = 0U;
    s_rx_seed4_precleanup_dma_ctlstat = 0U;
    s_rx_seed4_precleanup_dma_xfercfg = 0U;
    s_rx_seed4_precleanup_dma_errint = 0U;
    s_rx_seed4_precleanup_rxcount = 0U;
}

static void clear_dma0_rx_channel_state(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED4_DMA_CHANNEL);

    DMA0->COMMON[0].INTA = channelMask;
    DMA0->COMMON[0].INTB = channelMask;
    DMA0->COMMON[0].ERRINT = channelMask;
    DMA0->COMMON[0].INTENCLR = channelMask;
    DMA0->COMMON[0].ENABLECLR = channelMask;
}

static void capture_rx_seed4_precleanup_state(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED4_DMA_CHANNEL);

    s_rx_seed4_precleanup_valid = 1U;
    s_rx_seed4_precleanup_mstatus = base->MSTATUS;
    s_rx_seed4_precleanup_merrwarn = base->MERRWARN;
    s_rx_seed4_precleanup_mdatactrl = base->MDATACTRL;
    s_rx_seed4_precleanup_mdmactrl = base->MDMACTRL;
    s_rx_seed4_precleanup_mintset = base->MINTSET;
    s_rx_seed4_precleanup_mintmasked = base->MINTMASKED;
    s_rx_seed4_precleanup_inputmux_dmac_req_ena0 = INPUTMUX->DMAC0_REQ_ENA0;
    s_rx_seed4_precleanup_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
    s_rx_seed4_precleanup_dma_inta = DMA0->COMMON[0].INTA & channelMask;
    s_rx_seed4_precleanup_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED4_DMA_CHANNEL].CTLSTAT;
    s_rx_seed4_precleanup_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED4_DMA_CHANNEL].XFERCFG;
    s_rx_seed4_precleanup_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
    s_rx_seed4_precleanup_rxcount =
        (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
}

static void capture_rx_seed4_snapshot(I3C_Type *base,
                                      uint32_t stage,
                                      int32_t result,
                                      uint32_t dmaIntaCount,
                                      uint32_t dataIrqDelta,
                                      uint32_t protocolIrqDelta,
                                      uint32_t ibiIrqDelta)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED4_DMA_CHANNEL);

    s_rx_seed4_stage = stage;
    s_rx_seed4_result = result;
    s_rx_seed4_dma_inta_count = dmaIntaCount;
    s_rx_seed4_data_irq_delta = dataIrqDelta;
    s_rx_seed4_protocol_irq_delta = protocolIrqDelta;
    s_rx_seed4_ibi_irq_delta = ibiIrqDelta;
    if (s_rx_seed4_precleanup_valid != 0U)
    {
        s_rx_seed4_mstatus = s_rx_seed4_precleanup_mstatus;
        s_rx_seed4_merrwarn = s_rx_seed4_precleanup_merrwarn;
        s_rx_seed4_mdatactrl = s_rx_seed4_precleanup_mdatactrl;
        s_rx_seed4_mdmactrl = s_rx_seed4_precleanup_mdmactrl;
        s_rx_seed4_mintset = s_rx_seed4_precleanup_mintset;
        s_rx_seed4_mintmasked = s_rx_seed4_precleanup_mintmasked;
        s_rx_seed4_inputmux_dmac_req_ena0 = s_rx_seed4_precleanup_inputmux_dmac_req_ena0;
        s_rx_seed4_dma_active = s_rx_seed4_precleanup_dma_active;
        s_rx_seed4_dma_inta = s_rx_seed4_precleanup_dma_inta;
        s_rx_seed4_dma_ctlstat = s_rx_seed4_precleanup_dma_ctlstat;
        s_rx_seed4_dma_xfercfg = s_rx_seed4_precleanup_dma_xfercfg;
        s_rx_seed4_dma_errint = s_rx_seed4_precleanup_dma_errint;
        s_rx_seed4_rxcount = s_rx_seed4_precleanup_rxcount;
    }
    else
    {
        s_rx_seed4_mstatus = base->MSTATUS;
        s_rx_seed4_merrwarn = base->MERRWARN;
        s_rx_seed4_mdatactrl = base->MDATACTRL;
        s_rx_seed4_mdmactrl = base->MDMACTRL;
        s_rx_seed4_mintset = base->MINTSET;
        s_rx_seed4_mintmasked = base->MINTMASKED;
        s_rx_seed4_inputmux_dmac_req_ena0 = INPUTMUX->DMAC0_REQ_ENA0;
        s_rx_seed4_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
        s_rx_seed4_dma_inta = DMA0->COMMON[0].INTA & channelMask;
        s_rx_seed4_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED4_DMA_CHANNEL].CTLSTAT;
        s_rx_seed4_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED4_DMA_CHANNEL].XFERCFG;
        s_rx_seed4_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
        s_rx_seed4_rxcount = (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
    }
    s_rx_seed4_data0 = s_logical_rx_buffer[0];
    s_rx_seed4_data1 = s_logical_rx_buffer[1];
    s_rx_seed4_data2 = s_logical_rx_buffer[2];
    s_rx_seed4_data3 = s_logical_rx_buffer[3];
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

static status_t wait_for_rx_seed4_dma_inta(I3C_Type *base, uint32_t *dmaIntaCount)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED4_DMA_CHANNEL);
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

        if ((DMA0->COMMON[0].INTA & channelMask) != 0U)
        {
            *dmaIntaCount += 1U;
            return kStatus_Success;
        }
    }

    return kStatus_Timeout;
}

static void prepare_rx_seed4_controller(I3C_Type *base)
{
    uint32_t interruptMask = I3C_PROTOCOL_IRQ_MASK | (uint32_t)kI3C_MasterTxReadyFlag |
                             (uint32_t)kI3C_MasterRxReadyFlag;

    CLOCK_EnableClock(I3C_RX_SEED4_DMA_CLOCK);
    RESET_PeripheralReset(I3C_RX_SEED4_DMA_RESET);
    DMA0->CTRL = DMA_CTRL_ENABLE(1U);
    DMA0->SRAMBASE = (uint32_t)(uintptr_t)s_dma_descriptor_table;
    memset((void *)s_dma_descriptor_table, 0, sizeof(s_dma_descriptor_table));

    clear_dma0_rx_channel_state();
    I3C_MasterEnableDMA(base, false, false, 1U);
    I3C_MasterDisableInterrupts(base, interruptMask);
    if (I3C_RX_SEED4_ENABLE_RXREADY_INT != 0U)
    {
        I3C_MasterEnableInterrupts(base, (uint32_t)kI3C_MasterRxReadyFlag);
    }

    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED4_DMA_INPUTMUX_SIGNAL, true);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    if (I3C_RX_SEED4_KEEP_INPUTMUX_CLOCK_ENABLED == 0U)
    {
        INPUTMUX_Deinit(INPUTMUX);
    }

    DMA0->CHANNEL[I3C_RX_SEED4_DMA_CHANNEL].CFG = DMA_CHANNEL_CFG_PERIPHREQEN(1U);
    NVIC_ClearPendingIRQ(I3C_RX_SEED4_DMA_IRQ);
    NVIC_DisableIRQ(I3C_RX_SEED4_DMA_IRQ);
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_DisableIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);
    s_i3c_irq_status_latched = 0U;
}

static void configure_rx_seed4_descriptor(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED4_DMA_CHANNEL);
    dma_descriptor_t *bootstrapDescriptor = &s_dma_descriptor_table[I3C_RX_SEED4_DMA_CHANNEL];

    DMA0->COMMON[0].INTENSET = channelMask;
    bootstrapDescriptor->xfercfg = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_SETINTA(1U) |
                                   DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                                   DMA_CHANNEL_XFERCFG_DSTINC(1U) | DMA_CHANNEL_XFERCFG_XFERCOUNT(0U);
    bootstrapDescriptor->srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
    bootstrapDescriptor->dstEndAddr = &s_logical_rx_buffer[0];
    bootstrapDescriptor->linkToNextDesc = NULL;
}

static void arm_rx_seed4_descriptor(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED4_DMA_CHANNEL);

    DMA0->CHANNEL[I3C_RX_SEED4_DMA_CHANNEL].XFERCFG = s_dma_descriptor_table[I3C_RX_SEED4_DMA_CHANNEL].xfercfg;
    DMA0->COMMON[0].ENABLESET = channelMask;
    DMA0->COMMON[0].SETVALID = channelMask;
}

static void set_rx_seed4_dma_mode(I3C_Type *base)
{
    base->MDMACTRL = I3C_MDMACTRL_DMAFB(I3C_RX_SEED4_DMAFB_MODE) | I3C_MDMACTRL_DMATB(0U) |
                     I3C_MDMACTRL_DMAWIDTH(1U);
}

static status_t write_logical_payload_blocking(I3C_Type *base, uint8_t slaveAddr)
{
    i3c_master_transfer_t masterXfer;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;
    masterXfer.data = s_logical_tx_buffer;
    masterXfer.dataSize = I3C_RX_SEED4_LENGTH;

    return I3C_MasterTransferBlocking(base, &masterXfer);
}

static status_t finish_rx_seed4_read(I3C_Type *base)
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
    return ensure_master_idle(base);
}

static status_t run_i3c_rx_seed4_seed_only_dma_inta_poll_probe(I3C_Type *base, uint8_t slaveAddr)
{
    const uint32_t dataIrqBase = s_cm33_i3c_data_irq_count;
    const uint32_t protocolIrqBase = s_cm33_i3c_protocol_irq_count;
    const uint32_t ibiIrqBase = s_cm33_i3c_ibi_irq_count;
    const i3c_tx_trigger_level_t savedTxTriggerLevel =
        (i3c_tx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_TXTRIG_MASK) >> I3C_MDATACTRL_TXTRIG_SHIFT);
    const i3c_rx_trigger_level_t savedRxTriggerLevel =
        (i3c_rx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_RXTRIG_MASK) >> I3C_MDATACTRL_RXTRIG_SHIFT);
    uint32_t dmaIntaCount = 0U;
    uint32_t dataIrqDelta;
    uint32_t protocolIrqDelta;
    uint32_t ibiIrqDelta;
    status_t result;

    clear_rx_seed4_probe_state();
    s_active_transfer_length = I3C_RX_SEED4_LENGTH;
    prepare_logical_payload(I3C_RX_SEED4_LENGTH);
    memset(s_logical_rx_buffer, 0, sizeof(s_logical_rx_buffer));
    clear_ibi_state();
    clear_post_ibi_handoff_snapshot();
    begin_transfer_led();

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed4_snapshot(base, kRxSeed4StageErrorWrite, result, 0U, 0U, 0U, 0U);
        end_transfer_led(false);
        return result;
    }

    capture_rx_seed4_snapshot(base, kRxSeed4StageWriteStarted, kStatus_Success, 0U, 0U, 0U, 0U);
    result = write_logical_payload_blocking(base, slaveAddr);
    if (result != kStatus_Success)
    {
        capture_rx_seed4_snapshot(base, kRxSeed4StageErrorWrite, result, 0U, 0U, 0U, 0U);
        end_transfer_led(false);
        return result;
    }

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed4_snapshot(base, kRxSeed4StageErrorWrite, result, 0U, 0U, 0U, 0U);
        end_transfer_led(false);
        return result;
    }

    SDK_DelayAtLeastUs(I3C_RX_SEED4_POST_WRITE_SETTLE_US, SystemCoreClock);

    prepare_rx_seed4_controller(base);
    I3C_MasterClearErrorStatusFlags(base, I3C_MasterGetErrorStatusFlags(base));
    I3C_MasterClearStatusFlags(base,
                               (uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag |
                                   (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag |
                                   (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag);
    base->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    base->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, kI3C_RxTriggerOnNotEmpty, false, false);
    configure_rx_seed4_descriptor(base);
    I3C_MasterEnableDMA(base, false, true, 1U);
    set_rx_seed4_dma_mode(base);
    arm_rx_seed4_descriptor();

    capture_rx_seed4_snapshot(base, kRxSeed4StageReadStarted, kStatus_Success, 0U, 0U, 0U, 0U);
    result = I3C_MasterStartWithRxSize(base, kI3C_TypeI3CSdr, slaveAddr, kI3C_Read, I3C_RX_SEED4_LENGTH);
    if (result == kStatus_Success)
    {
        result = wait_for_rx_seed4_dma_inta(base, &dmaIntaCount);
    }

    capture_rx_seed4_precleanup_state(base);
    if (result == kStatus_Success)
    {
        result = finish_rx_seed4_read(base);
    }

    dataIrqDelta = s_cm33_i3c_data_irq_count - dataIrqBase;
    protocolIrqDelta = s_cm33_i3c_protocol_irq_count - protocolIrqBase;
    ibiIrqDelta = s_cm33_i3c_ibi_irq_count - ibiIrqBase;

    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, savedRxTriggerLevel, false, false);
    I3C_MasterEnableDMA(base, false, false, 1U);
    clear_dma0_rx_channel_state();
    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED4_DMA_INPUTMUX_SIGNAL, false);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);

    if (result != kStatus_Success)
    {
        capture_rx_seed4_snapshot(base, kRxSeed4StageErrorDma, result, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return result;
    }

    capture_rx_seed4_snapshot(base, kRxSeed4StageDmaInta, kStatus_Success, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);

    if ((dataIrqDelta != 0U) || (protocolIrqDelta != 0U) || (ibiIrqDelta != 0U))
    {
        capture_rx_seed4_snapshot(base, kRxSeed4StageErrorIrq, kStatus_Fail, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return kStatus_Fail;
    }

    if ((dmaIntaCount != 1U) || (s_rx_seed4_data0 != s_logical_tx_buffer[0]))
    {
        capture_rx_seed4_snapshot(base, kRxSeed4StageErrorMetrics, kStatus_Fail, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        end_transfer_led(false);
        return kStatus_Fail;
    }

    capture_rx_seed4_snapshot(base, kRxSeed4StageSuccess, kStatus_Success, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        PRINTF(I3C_RX_SEED4_TRACE_LABEL " dmaInta=%lu rxcount=%lu data=%02x %02x %02x %02x\r\n",
           (unsigned long)dmaIntaCount,
           (unsigned long)s_rx_seed4_rxcount,
           (unsigned int)s_rx_seed4_data0,
           (unsigned int)s_rx_seed4_data1,
           (unsigned int)s_rx_seed4_data2,
           (unsigned int)s_rx_seed4_data3);
        EXP_LOG_INFO(I3C_RX_SEED4_TRACE_LABEL " dmaInta=%lu rxcount=%lu data=%02x %02x %02x %02x",
                 (unsigned long)dmaIntaCount,
                 (unsigned long)s_rx_seed4_rxcount,
                 (unsigned int)s_rx_seed4_data0,
                 (unsigned int)s_rx_seed4_data1,
                 (unsigned int)s_rx_seed4_data2,
                 (unsigned int)s_rx_seed4_data3);

    end_transfer_led(true);
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

    PRINTF("\r\n%s -- master.\r\n", I3C_RX_SEED4_PROBE_NAME);
    EXP_LOG_INFO("%s -- master.", I3C_RX_SEED4_PROBE_NAME);

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

    result = run_i3c_rx_seed4_seed_only_dma_inta_poll_probe(EXAMPLE_MASTER, slaveAddr);
    if (result != kStatus_Success)
    {
        EXP_LOG_ERROR("%s failed: %d", I3C_RX_SEED4_PROBE_NAME, result);
        dump_debug_state(EXAMPLE_MASTER);
        set_failure_led();
        return -1;
    }

    PRINTF("%s successful.\r\n", I3C_RX_SEED4_PROBE_NAME);
    EXP_LOG_INFO("%s successful.", I3C_RX_SEED4_PROBE_NAME);
    set_success_led();
    return 0;
}