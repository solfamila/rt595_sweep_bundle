/*
 * Focused RX request-semantics probe:
 * 1. blocking CPU write + plain polling CPU read of 4 bytes
 * 2. blocking CPU write + plain RX DMA bulk read of 4 bytes on DMA0 CH24
 * 3. blocking CPU write + linked one-byte RX DMA descriptors on DMA0 CH24
 */

#define main master_i3c_sdma_seed_tail_len_sweep_unused_main
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_test_standalone.c"
#undef main

#define I3C_DMA_RX_CHANNEL 24U
#define I3C_DMA_CONTROLLER DMA0
#define I3C_DMA_CLOCK kCLOCK_Dmac0
#define I3C_DMA_RESET kDMAC0_RST_SHIFT_RSTn
#define I3C_DMA_IRQ DMA0_IRQn
#define I3C_DMA_RX_INPUTMUX_SIGNAL kINPUTMUX_I3c0RxToDmac0Ch24RequestEna
#define I3C_DMA_TX_INPUTMUX_SIGNAL kINPUTMUX_I3c0TxToDmac0Ch25RequestEna
#define I3C_RX_REQUEST_PROBE_LENGTH 4U
#define I3C_RX_REQUEST_PROBE_POST_WRITE_SETTLE_US 1000U

uint8_t *g_txBuff = NULL;
uint32_t g_txSize = 0U;
volatile bool g_slaveIbiRequestSent = false;
volatile bool g_slavePostIbiAddressMatched = false;

#define RX_REQUEST_PROBE_MODE_NONE 0U
#define RX_REQUEST_PROBE_MODE_CPU 1U
#define RX_REQUEST_PROBE_MODE_DMA_BULK 2U
#define RX_REQUEST_PROBE_MODE_DMA_CHAIN 3U

#define RX_REQUEST_PROBE_STAGE_CLEARED 0U
#define RX_REQUEST_PROBE_STAGE_CPU_WRITE_STARTED 1U
#define RX_REQUEST_PROBE_STAGE_CPU_READ_STARTED 2U
#define RX_REQUEST_PROBE_STAGE_CPU_OK 3U
#define RX_REQUEST_PROBE_STAGE_DMA_BULK_WRITE_STARTED 4U
#define RX_REQUEST_PROBE_STAGE_DMA_BULK_READ_STARTED 5U
#define RX_REQUEST_PROBE_STAGE_DMA_BULK_OK 6U
#define RX_REQUEST_PROBE_STAGE_DMA_CHAIN_WRITE_STARTED 7U
#define RX_REQUEST_PROBE_STAGE_DMA_CHAIN_READ_STARTED 8U
#define RX_REQUEST_PROBE_STAGE_DMA_CHAIN_OK 9U
#define RX_REQUEST_PROBE_STAGE_SUCCESS 10U
#define RX_REQUEST_PROBE_STAGE_ERROR_WRITE 0x201U
#define RX_REQUEST_PROBE_STAGE_ERROR_READ 0x202U
#define RX_REQUEST_PROBE_STAGE_ERROR_COMPARE 0x203U
#define RX_REQUEST_PROBE_STAGE_ERROR_CM33_IRQ 0x204U

static __NO_INIT volatile uint32_t s_rx_request_probe_mode;
static __NO_INIT volatile uint32_t s_rx_request_probe_stage;
static __NO_INIT volatile int32_t s_rx_request_probe_result;
static __NO_INIT volatile uint32_t s_rx_request_probe_dma_inta_count;
static __NO_INIT volatile uint32_t s_rx_request_probe_data_irq_delta;
static __NO_INIT volatile uint32_t s_rx_request_probe_protocol_irq_delta;
static __NO_INIT volatile uint32_t s_rx_request_probe_ibi_irq_delta;
static __NO_INIT volatile uint32_t s_rx_request_probe_status;
static __NO_INIT volatile uint32_t s_rx_request_probe_err_status;
static __NO_INIT volatile uint32_t s_rx_request_probe_mdatactrl;
static __NO_INIT volatile uint32_t s_rx_request_probe_dma_active;
static __NO_INIT volatile uint32_t s_rx_request_probe_dma_inta;
static __NO_INIT volatile uint32_t s_rx_request_probe_dma_ctlstat;
static __NO_INIT volatile uint32_t s_rx_request_probe_dma_xfercfg;
static __NO_INIT volatile uint32_t s_rx_request_probe_data0;
static __NO_INIT volatile uint32_t s_rx_request_probe_data1;
static __NO_INIT volatile uint32_t s_rx_request_probe_data2;
static __NO_INIT volatile uint32_t s_rx_request_probe_data3;

static void clear_rx_request_probe_snapshot(void)
{
    s_rx_request_probe_mode = RX_REQUEST_PROBE_MODE_NONE;
    s_rx_request_probe_stage = RX_REQUEST_PROBE_STAGE_CLEARED;
    s_rx_request_probe_result = (int32_t)kStatus_Success;
    s_rx_request_probe_dma_inta_count = 0U;
    s_rx_request_probe_data_irq_delta = 0U;
    s_rx_request_probe_protocol_irq_delta = 0U;
    s_rx_request_probe_ibi_irq_delta = 0U;
    s_rx_request_probe_status = 0U;
    s_rx_request_probe_err_status = 0U;
    s_rx_request_probe_mdatactrl = 0U;
    s_rx_request_probe_dma_active = 0U;
    s_rx_request_probe_dma_inta = 0U;
    s_rx_request_probe_dma_ctlstat = 0U;
    s_rx_request_probe_dma_xfercfg = 0U;
    s_rx_request_probe_data0 = 0U;
    s_rx_request_probe_data1 = 0U;
    s_rx_request_probe_data2 = 0U;
    s_rx_request_probe_data3 = 0U;
}

static void capture_rx_request_probe_snapshot(I3C_Type *base,
                                              uint32_t mode,
                                              uint32_t stage,
                                              int32_t result,
                                              uint32_t dmaIntaCount,
                                              uint32_t dataIrqDelta,
                                              uint32_t protocolIrqDelta,
                                              uint32_t ibiIrqDelta)
{
    const uint32_t channelMask = (1UL << I3C_DMA_RX_CHANNEL);

    s_rx_request_probe_mode = mode;
    s_rx_request_probe_stage = stage;
    s_rx_request_probe_result = result;
    s_rx_request_probe_dma_inta_count = dmaIntaCount;
    s_rx_request_probe_data_irq_delta = dataIrqDelta;
    s_rx_request_probe_protocol_irq_delta = protocolIrqDelta;
    s_rx_request_probe_ibi_irq_delta = ibiIrqDelta;
    s_rx_request_probe_status = I3C_MasterGetStatusFlags(base);
    s_rx_request_probe_err_status = I3C_MasterGetErrorStatusFlags(base);
    s_rx_request_probe_mdatactrl = base->MDATACTRL;
    s_rx_request_probe_data0 = s_rx_buffer[0];
    s_rx_request_probe_data1 = s_rx_buffer[1];
    s_rx_request_probe_data2 = s_rx_buffer[2];
    s_rx_request_probe_data3 = s_rx_buffer[3];

    if (mode == RX_REQUEST_PROBE_MODE_CPU)
    {
        s_rx_request_probe_dma_active = 0U;
        s_rx_request_probe_dma_inta = 0U;
        s_rx_request_probe_dma_ctlstat = 0U;
        s_rx_request_probe_dma_xfercfg = 0U;
        return;
    }

    s_rx_request_probe_dma_active = I3C_DMA_CONTROLLER->COMMON[0].ACTIVE & channelMask;
    s_rx_request_probe_dma_inta = I3C_DMA_CONTROLLER->COMMON[0].INTA & channelMask;
    s_rx_request_probe_dma_ctlstat = I3C_DMA_CONTROLLER->CHANNEL[I3C_DMA_RX_CHANNEL].CTLSTAT;
    s_rx_request_probe_dma_xfercfg = I3C_DMA_CONTROLLER->CHANNEL[I3C_DMA_RX_CHANNEL].XFERCFG;
}

static void clear_dma0_channel_state_mask(uint32_t channelMask)
{
    I3C_DMA_CONTROLLER->COMMON[0].INTA = channelMask;
    I3C_DMA_CONTROLLER->COMMON[0].INTB = channelMask;
    I3C_DMA_CONTROLLER->COMMON[0].ERRINT = channelMask;
    I3C_DMA_CONTROLLER->COMMON[0].INTENCLR = channelMask;
    I3C_DMA_CONTROLLER->COMMON[0].ENABLECLR = channelMask;
}

static void clear_dma0_rx_channel_state(void)
{
    clear_dma0_channel_state_mask(1UL << I3C_DMA_RX_CHANNEL);
}

static void trigger_rx_dma_channel(void)
{
    I3C_DMA_CONTROLLER->COMMON[0].SETTRIG = (1UL << I3C_DMA_RX_CHANNEL);
}

static void prepare_roundtrip_read_dma_controller(I3C_Type *base)
{
    CLOCK_EnableClock(I3C_DMA_CLOCK);
    RESET_PeripheralReset(I3C_DMA_RESET);
    I3C_DMA_CONTROLLER->CTRL = DMA_CTRL_ENABLE(1U);
    I3C_DMA_CONTROLLER->SRAMBASE = (uint32_t)(uintptr_t)s_dma_descriptor_table;
    memset((void *)s_dma_descriptor_table, 0, sizeof(s_dma_descriptor_table));
    memset((void *)s_dma_seed_chain, 0, sizeof(s_dma_seed_chain));

    clear_dma0_rx_channel_state();
    I3C_MasterEnableDMA(base, false, false, 1U);

    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_DMA_RX_INPUTMUX_SIGNAL, true);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_DMA_TX_INPUTMUX_SIGNAL, false);
    INPUTMUX_Deinit(INPUTMUX);

    I3C_DMA_CONTROLLER->CHANNEL[I3C_DMA_RX_CHANNEL].CFG = DMA_CHANNEL_CFG_PERIPHREQEN(1U) |
                                                         DMA_CHANNEL_CFG_TRIGBURST(1U) |
                                                         DMA_CHANNEL_CFG_BURSTPOWER(2U);
    NVIC_ClearPendingIRQ(I3C_DMA_IRQ);
    NVIC_DisableIRQ(I3C_DMA_IRQ);
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_DisableIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);
}

static void configure_rx_dma_bulk(I3C_Type *base)
{
    dma_descriptor_t *bootstrapDescriptor = &s_dma_descriptor_table[I3C_DMA_RX_CHANNEL];

    bootstrapDescriptor->xfercfg = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_SETINTA(1U) |
                                   DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                                   DMA_CHANNEL_XFERCFG_DSTINC(1U) |
                                   DMA_CHANNEL_XFERCFG_XFERCOUNT(I3C_RX_REQUEST_PROBE_LENGTH - 1U);
    bootstrapDescriptor->srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
    bootstrapDescriptor->dstEndAddr = &s_rx_buffer[I3C_RX_REQUEST_PROBE_LENGTH - 1U];
    bootstrapDescriptor->linkToNextDesc = NULL;
}

static void configure_rx_dma_chain(I3C_Type *base)
{
    dma_descriptor_t *bootstrapDescriptor = &s_dma_descriptor_table[I3C_DMA_RX_CHANNEL];

    memset((void *)s_dma_seed_chain, 0, sizeof(s_dma_seed_chain));

    bootstrapDescriptor->xfercfg = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_RELOAD(1U) |
                                   DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                                   DMA_CHANNEL_XFERCFG_DSTINC(0U) | DMA_CHANNEL_XFERCFG_XFERCOUNT(0U);
    bootstrapDescriptor->srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
    bootstrapDescriptor->dstEndAddr = &s_rx_buffer[0];
    bootstrapDescriptor->linkToNextDesc = &s_dma_seed_chain[0];

    for (size_t index = 1U; index < I3C_RX_REQUEST_PROBE_LENGTH; index++)
    {
        dma_descriptor_t *descriptor = &s_dma_seed_chain[index - 1U];
        const bool isLast = (index == (I3C_RX_REQUEST_PROBE_LENGTH - 1U));

        descriptor->xfercfg = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_RELOAD(isLast ? 0U : 1U) |
                              DMA_CHANNEL_XFERCFG_SETINTA(isLast ? 1U : 0U) |
                              DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                              DMA_CHANNEL_XFERCFG_DSTINC(0U) | DMA_CHANNEL_XFERCFG_XFERCOUNT(0U);
        descriptor->srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
        descriptor->dstEndAddr = &s_rx_buffer[index];
        descriptor->linkToNextDesc = isLast ? NULL : &s_dma_seed_chain[index];
    }
}

static void arm_rx_dma_channel(void)
{
    const uint32_t channelMask = (1UL << I3C_DMA_RX_CHANNEL);

    I3C_DMA_CONTROLLER->COMMON[0].INTENSET = channelMask;
    I3C_DMA_CONTROLLER->CHANNEL[I3C_DMA_RX_CHANNEL].XFERCFG = s_dma_descriptor_table[I3C_DMA_RX_CHANNEL].xfercfg;
    I3C_DMA_CONTROLLER->COMMON[0].ENABLESET = channelMask;
    I3C_DMA_CONTROLLER->COMMON[0].SETVALID = channelMask;
}

static status_t wait_for_rx_read_complete(I3C_Type *base)
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

        if ((errStatus & ~((uint32_t)kI3C_MasterErrorNackFlag)) != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }
    }

    return kStatus_Timeout;
}

static status_t wait_for_rx_dma_completion(I3C_Type *base, uint32_t *dmaIntaCount, bool allowRetrigger)
{
    const uint32_t channelMask = (1UL << I3C_DMA_RX_CHANNEL);
    volatile uint32_t timeout = 0U;

    while (++timeout < I3C_DMA_SEED_CHAIN_TIMEOUT)
    {
        uint32_t errStatus = I3C_MasterGetErrorStatusFlags(base);
        uint32_t dmaErr = I3C_DMA_CONTROLLER->COMMON[0].ERRINT & channelMask;

        service_transfer_led();

        if (dmaErr != 0U)
        {
            I3C_DMA_CONTROLLER->COMMON[0].ERRINT = channelMask;
            return kStatus_Fail;
        }

        if (errStatus != 0U)
        {
            return I3C_MasterCheckAndClearError(base, errStatus);
        }

        if ((I3C_DMA_CONTROLLER->COMMON[0].INTA & channelMask) != 0U)
        {
            I3C_DMA_CONTROLLER->COMMON[0].INTA = channelMask;
            *dmaIntaCount += 1U;
            return kStatus_Success;
        }

        if (allowRetrigger && ((I3C_DMA_CONTROLLER->COMMON[0].ACTIVE & channelMask) != 0U) &&
            ((I3C_DMA_CONTROLLER->CHANNEL[I3C_DMA_RX_CHANNEL].CTLSTAT & DMA_CHANNEL_CTLSTAT_TRIG_MASK) == 0U))
        {
            trigger_rx_dma_channel();
        }
    }

    return kStatus_Timeout;
}

static status_t finish_dma_read(I3C_Type *base)
{
    status_t result;

    I3C_MasterEnableDMA(base, false, false, 1U);
    result = I3C_MasterStop(base);
    if (result == kStatus_Success)
    {
        result = wait_for_post_ibi_read_ctrl_done(base);
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

static status_t write_transfer_window_blocking(I3C_Type *base, uint8_t slaveAddr)
{
    i3c_master_transfer_t masterXfer;

    prepare_roundtrip_read_cpu_controller(base);

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.direction = kI3C_Write;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;
    masterXfer.data = s_tx_buffer;
    masterXfer.dataSize = s_active_transfer_length;

    return I3C_MasterTransferBlocking(base, &masterXfer);
}

static status_t run_roundtrip_read_dma(I3C_Type *base, uint8_t slaveAddr, bool linkedDescriptors, uint32_t *dmaIntaCount)
{
    static const uint32_t clearFlags = (uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag |
                                       (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag |
                                       (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag;
    status_t result;

    memset(s_rx_buffer, 0, sizeof(s_rx_buffer));
    prepare_roundtrip_read_dma_controller(base);

    I3C_MasterClearErrorStatusFlags(base, I3C_MasterGetErrorStatusFlags(base));
    I3C_MasterClearStatusFlags(base, clearFlags);
    base->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    base->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;

    if (linkedDescriptors)
    {
        configure_rx_dma_chain(base);
    }
    else
    {
        configure_rx_dma_bulk(base);
    }

    I3C_MasterEnableDMA(base, false, true, 1U);
    arm_rx_dma_channel();

    result = I3C_MasterStartWithRxSize(base, kI3C_TypeI3CSdr, slaveAddr, kI3C_Read, I3C_RX_REQUEST_PROBE_LENGTH);
    if (result != kStatus_Success)
    {
        return result;
    }

    result = wait_for_rx_read_complete(base);
    if (result != kStatus_Success)
    {
        return result;
    }

    trigger_rx_dma_channel();

    result = wait_for_rx_dma_completion(base, dmaIntaCount, linkedDescriptors);
    if (result != kStatus_Success)
    {
        return result;
    }

    return finish_dma_read(base);
}

static status_t run_blocking_read_case(I3C_Type *base, uint8_t slaveAddr)
{
    static const uint32_t clearFlags = (uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag |
                                       (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag |
                                       (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag;
    i3c_master_transfer_t masterXfer;
    status_t result;

    clear_roundtrip_read_snapshot();
    memset(s_rx_buffer, 0, sizeof(s_rx_buffer));
    I3C_MasterClearErrorStatusFlags(base, I3C_MasterGetErrorStatusFlags(base));
    I3C_MasterClearStatusFlags(base, clearFlags);
    base->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    base->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    prepare_roundtrip_read_cpu_controller(base);

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = slaveAddr;
    masterXfer.direction = kI3C_Read;
    masterXfer.busType = kI3C_TypeI3CSdr;
    masterXfer.flags = kI3C_TransferDefaultFlag;
    masterXfer.ibiResponse = kI3C_IbiRespAckMandatory;
    masterXfer.data = s_rx_buffer;
    masterXfer.dataSize = s_active_transfer_length;

    result = I3C_MasterTransferBlocking(base, &masterXfer);
    s_roundtrip_read_status = result;
    s_roundtrip_read_snapshot.stage =
        (result == kStatus_Success) ? ROUNDTRIP_STAGE_BLOCKING_COMPLETED : ROUNDTRIP_STAGE_ERROR_BLOCKING;
    s_roundtrip_read_snapshot.remaining = 0U;
    capture_roundtrip_read_snapshot(base, result);
    if ((result != kStatus_Success) && (result != kStatus_I3C_Nak) && (result != kStatus_I3C_Term))
    {
        return result;
    }

    if (!buffers_match(s_tx_buffer, s_rx_buffer, s_active_transfer_length))
    {
        return kStatus_Fail;
    }

    return ensure_master_idle(base);
}

static status_t run_rx_request_probe_case(I3C_Type *base, uint8_t slaveAddr, uint32_t mode, uint32_t writeStage, uint32_t readStage, uint32_t okStage)
{
    uint32_t dataIrqBase = s_cm33_i3c_data_irq_count;
    uint32_t protocolIrqBase = s_cm33_i3c_protocol_irq_count;
    uint32_t ibiIrqBase = s_cm33_i3c_ibi_irq_count;
    uint32_t dmaIntaCount = 0U;
    uint32_t dataIrqDelta;
    uint32_t protocolIrqDelta;
    uint32_t ibiIrqDelta;
    status_t result;

    clear_ibi_state();
    clear_post_ibi_handoff_snapshot();
    prepare_logical_payload(I3C_RX_REQUEST_PROBE_LENGTH);
    load_transfer_window(s_logical_tx_buffer, I3C_RX_REQUEST_PROBE_LENGTH);
    memset(s_rx_buffer, 0, sizeof(s_rx_buffer));

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_request_probe_snapshot(base, mode, RX_REQUEST_PROBE_STAGE_ERROR_WRITE, result, 0U, 0U, 0U, 0U);
        return result;
    }

    capture_rx_request_probe_snapshot(base, mode, writeStage, kStatus_Success, 0U, 0U, 0U, 0U);
    result = write_transfer_window_blocking(base, slaveAddr);
    if (result != kStatus_Success)
    {
        capture_rx_request_probe_snapshot(base, mode, RX_REQUEST_PROBE_STAGE_ERROR_WRITE, result, 0U, 0U, 0U, 0U);
        return result;
    }

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_request_probe_snapshot(base, mode, RX_REQUEST_PROBE_STAGE_ERROR_WRITE, result, 0U, 0U, 0U, 0U);
        return result;
    }

    SDK_DelayAtLeastUs(I3C_RX_REQUEST_PROBE_POST_WRITE_SETTLE_US, SystemCoreClock);

    capture_rx_request_probe_snapshot(base, mode, readStage, kStatus_Success, 0U, 0U, 0U, 0U);
    if (mode == RX_REQUEST_PROBE_MODE_CPU)
    {
        result = run_roundtrip_read(base, slaveAddr);
    }
    else
    {
        result = run_roundtrip_read_dma(base, slaveAddr, mode == RX_REQUEST_PROBE_MODE_DMA_CHAIN, &dmaIntaCount);
    }

    dataIrqDelta = s_cm33_i3c_data_irq_count - dataIrqBase;
    protocolIrqDelta = s_cm33_i3c_protocol_irq_count - protocolIrqBase;
    ibiIrqDelta = s_cm33_i3c_ibi_irq_count - ibiIrqBase;

    if (result != kStatus_Success)
    {
        capture_rx_request_probe_snapshot(
            base, mode, RX_REQUEST_PROBE_STAGE_ERROR_READ, result, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        return result;
    }

    if (!buffers_match(s_tx_buffer, s_rx_buffer, s_active_transfer_length))
    {
        capture_rx_request_probe_snapshot(
            base, mode, RX_REQUEST_PROBE_STAGE_ERROR_COMPARE, kStatus_Fail, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        return kStatus_Fail;
    }

    if ((dataIrqDelta != 0U) || (protocolIrqDelta != 0U) || (ibiIrqDelta != 0U))
    {
        capture_rx_request_probe_snapshot(
            base, mode, RX_REQUEST_PROBE_STAGE_ERROR_CM33_IRQ, kStatus_Fail, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        return kStatus_Fail;
    }

    capture_rx_request_probe_snapshot(base, mode, okStage, kStatus_Success, dmaIntaCount, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
    PRINTF("rx-request-probe mode=%lu dma_inta=%lu data=%02x %02x %02x %02x\r\n",
           (unsigned long)mode,
           (unsigned long)dmaIntaCount,
           (unsigned int)s_rx_buffer[0],
           (unsigned int)s_rx_buffer[1],
           (unsigned int)s_rx_buffer[2],
           (unsigned int)s_rx_buffer[3]);
    EXP_LOG_INFO("rx-request-probe mode=%lu dma_inta=%lu data=%02x %02x %02x %02x",
                 (unsigned long)mode,
                 (unsigned long)dmaIntaCount,
                 (unsigned int)s_rx_buffer[0],
                 (unsigned int)s_rx_buffer[1],
                 (unsigned int)s_rx_buffer[2],
                 (unsigned int)s_rx_buffer[3]);
    return kStatus_Success;
}

static status_t run_i3c_rx_request_semantics_probe(I3C_Type *base, uint8_t slaveAddr)
{
    status_t result;

    clear_rx_request_probe_snapshot();

    result = run_rx_request_probe_case(base,
                                       slaveAddr,
                                       RX_REQUEST_PROBE_MODE_CPU,
                                       RX_REQUEST_PROBE_STAGE_CPU_WRITE_STARTED,
                                       RX_REQUEST_PROBE_STAGE_CPU_READ_STARTED,
                                       RX_REQUEST_PROBE_STAGE_CPU_OK);
    if (result != kStatus_Success)
    {
        return result;
    }

    result = run_rx_request_probe_case(base,
                                       slaveAddr,
                                       RX_REQUEST_PROBE_MODE_DMA_BULK,
                                       RX_REQUEST_PROBE_STAGE_DMA_BULK_WRITE_STARTED,
                                       RX_REQUEST_PROBE_STAGE_DMA_BULK_READ_STARTED,
                                       RX_REQUEST_PROBE_STAGE_DMA_BULK_OK);
    if (result != kStatus_Success)
    {
        return result;
    }

    result = run_rx_request_probe_case(base,
                                       slaveAddr,
                                       RX_REQUEST_PROBE_MODE_DMA_CHAIN,
                                       RX_REQUEST_PROBE_STAGE_DMA_CHAIN_WRITE_STARTED,
                                       RX_REQUEST_PROBE_STAGE_DMA_CHAIN_READ_STARTED,
                                       RX_REQUEST_PROBE_STAGE_DMA_CHAIN_OK);
    if (result != kStatus_Success)
    {
        return result;
    }

    capture_rx_request_probe_snapshot(base,
                                      RX_REQUEST_PROBE_MODE_DMA_CHAIN,
                                      RX_REQUEST_PROBE_STAGE_SUCCESS,
                                      kStatus_Success,
                                      s_rx_request_probe_dma_inta_count,
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

    PRINTF("\r\nI3C RX request semantics probe -- master.\r\n");
    EXP_LOG_INFO("I3C RX request semantics probe -- master.");

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

    result = run_i3c_rx_request_semantics_probe(EXAMPLE_MASTER, slaveAddr);
    if (result != kStatus_Success)
    {
        EXP_LOG_ERROR("I3C RX request semantics probe failed: %d", result);
        dump_debug_state(EXAMPLE_MASTER);
        set_failure_led();
        return -1;
    }

    PRINTF("I3C RX request semantics probe successful.\r\n");
    EXP_LOG_INFO("I3C RX request semantics probe successful.");
    set_success_led();
    return 0;
}