/*
 * Narrow RX discriminator for the RT500 errata geometry:
 *  - read length = 6 bytes
 *  - RXTRIG = 3/4 full
 *  - DMA moves exactly one seed byte from MRDATAB
 *  - DMA0 IRQ wakes SmartDMA, but SmartDMA does not read any tail bytes
 *  - CM33 must not service RXREADY/TXREADY data IRQs
 */

#define main master_i3c_sdma_seed_tail_len_sweep_unused_main
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_test_standalone.c"
#undef main

#define I3C_RX_SEED_ONLY_LENGTH 6U
#define I3C_RX_SEED_ONLY_DMA_CHANNEL 24U
#define I3C_RX_SEED_ONLY_DMA_CLOCK kCLOCK_Dmac0
#define I3C_RX_SEED_ONLY_DMA_RESET kDMAC0_RST_SHIFT_RSTn
#define I3C_RX_SEED_ONLY_DMA_IRQ DMA0_IRQn
#define I3C_RX_SEED_ONLY_DMA_INPUTMUX_SIGNAL kINPUTMUX_I3c0RxToDmac0Ch24RequestEna
#define I3C_RX_SEED_ONLY_SMARTDMA_API_INDEX 2U
#define I3C_RX_SEED_ONLY_POST_WRITE_SETTLE_US 1000U
#define I3C_RX_SEED_ONLY_MAILBOX_COMPLETION 1U

uint8_t *g_txBuff = NULL;
uint32_t g_txSize = 0U;
volatile bool g_slaveIbiRequestSent = false;
volatile bool g_slavePostIbiAddressMatched = false;
volatile bool g_slavePostIbiEchoPending = false;
volatile bool g_slavePostIbiEchoArmed = false;

typedef struct _i3c_rx_seed_only_param
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
} i3c_rx_seed_only_param_t;

enum
{
    RX_SEED_ONLY_STAGE_CLEARED = 0U,
    RX_SEED_ONLY_STAGE_WRITE_STARTED = 1U,
    RX_SEED_ONLY_STAGE_READ_STARTED = 2U,
    RX_SEED_ONLY_STAGE_MAILBOX = 3U,
    RX_SEED_ONLY_STAGE_SUCCESS = 4U,
    RX_SEED_ONLY_STAGE_ERROR_WRITE = 0x201U,
    RX_SEED_ONLY_STAGE_ERROR_MAILBOX = 0x202U,
    RX_SEED_ONLY_STAGE_ERROR_IRQ = 0x203U,
    RX_SEED_ONLY_STAGE_ERROR_METRICS = 0x204U,
};

AT_NONCACHEABLE_SECTION_ALIGN(static i3c_rx_seed_only_param_t s_rx_seed_only_param, 4);

static __NO_INIT volatile uint32_t s_rx_seed_only_stage;
static __NO_INIT volatile int32_t s_rx_seed_only_result;
static __NO_INIT volatile uint32_t s_rx_seed_only_expected_wakes;
static __NO_INIT volatile uint32_t s_rx_seed_only_wakes;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_seed_bytes;
static __NO_INIT volatile uint32_t s_rx_seed_only_smartdma_bytes;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_inta_count;
static __NO_INIT volatile uint32_t s_rx_seed_only_frame_bytes;
static __NO_INIT volatile uint32_t s_rx_seed_only_data_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed_only_protocol_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed_only_ibi_irq_delta;
static __NO_INIT volatile uint32_t s_rx_seed_only_mstatus;
static __NO_INIT volatile uint32_t s_rx_seed_only_merrwarn;
static __NO_INIT volatile uint32_t s_rx_seed_only_mdatactrl;
static __NO_INIT volatile uint32_t s_rx_seed_only_mdmactrl;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_active;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_inta;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_ctlstat;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_xfercfg;
static __NO_INIT volatile uint32_t s_rx_seed_only_dma_errint;
static __NO_INIT volatile uint32_t s_rx_seed_only_rxcount;
static __NO_INIT volatile uint32_t s_rx_seed_only_data0;
static __NO_INIT volatile uint32_t s_rx_seed_only_data1;
static __NO_INIT volatile uint32_t s_rx_seed_only_data2;
static __NO_INIT volatile uint32_t s_rx_seed_only_data3;
static __NO_INIT volatile uint32_t s_rx_seed_only_data4;
static __NO_INIT volatile uint32_t s_rx_seed_only_data5;

static void clear_rx_seed_only_probe_state(void)
{
    s_rx_seed_only_stage = RX_SEED_ONLY_STAGE_CLEARED;
    s_rx_seed_only_result = (int32_t)kStatus_Success;
    s_rx_seed_only_expected_wakes = 0U;
    s_rx_seed_only_wakes = 0U;
    s_rx_seed_only_dma_seed_bytes = 0U;
    s_rx_seed_only_smartdma_bytes = 0U;
    s_rx_seed_only_dma_inta_count = 0U;
    s_rx_seed_only_frame_bytes = 0U;
    s_rx_seed_only_data_irq_delta = 0U;
    s_rx_seed_only_protocol_irq_delta = 0U;
    s_rx_seed_only_ibi_irq_delta = 0U;
    s_rx_seed_only_mstatus = 0U;
    s_rx_seed_only_merrwarn = 0U;
    s_rx_seed_only_mdatactrl = 0U;
    s_rx_seed_only_mdmactrl = 0U;
    s_rx_seed_only_dma_active = 0U;
    s_rx_seed_only_dma_inta = 0U;
    s_rx_seed_only_dma_ctlstat = 0U;
    s_rx_seed_only_dma_xfercfg = 0U;
    s_rx_seed_only_dma_errint = 0U;
    s_rx_seed_only_rxcount = 0U;
    s_rx_seed_only_data0 = 0U;
    s_rx_seed_only_data1 = 0U;
    s_rx_seed_only_data2 = 0U;
    s_rx_seed_only_data3 = 0U;
    s_rx_seed_only_data4 = 0U;
    s_rx_seed_only_data5 = 0U;
    memset((void *)&s_rx_seed_only_param, 0, sizeof(s_rx_seed_only_param));
}

static void clear_dma0_rx_channel_state(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_ONLY_DMA_CHANNEL);

    DMA0->COMMON[0].INTA = channelMask;
    DMA0->COMMON[0].INTB = channelMask;
    DMA0->COMMON[0].ERRINT = channelMask;
    DMA0->COMMON[0].INTENCLR = channelMask;
    DMA0->COMMON[0].ENABLECLR = channelMask;
}

static void capture_rx_seed_only_snapshot(I3C_Type *base,
                                          uint32_t stage,
                                          int32_t result,
                                          uint32_t dataIrqDelta,
                                          uint32_t protocolIrqDelta,
                                          uint32_t ibiIrqDelta)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_ONLY_DMA_CHANNEL);

    s_rx_seed_only_stage = stage;
    s_rx_seed_only_result = result;
    s_rx_seed_only_expected_wakes = s_rx_seed_only_param.expectedWakeCount;
    s_rx_seed_only_wakes = s_rx_seed_only_param.wakeCount;
    s_rx_seed_only_dma_seed_bytes = s_rx_seed_only_param.dmaSeedBytes;
    s_rx_seed_only_smartdma_bytes = s_rx_seed_only_param.smartdmaBytes;
    s_rx_seed_only_dma_inta_count = s_rx_seed_only_param.dmaIntaCount;
    s_rx_seed_only_frame_bytes = s_rx_seed_only_param.dmaSeedBytes + s_rx_seed_only_param.smartdmaBytes;
    s_rx_seed_only_data_irq_delta = dataIrqDelta;
    s_rx_seed_only_protocol_irq_delta = protocolIrqDelta;
    s_rx_seed_only_ibi_irq_delta = ibiIrqDelta;
    s_rx_seed_only_mstatus = base->MSTATUS;
    s_rx_seed_only_merrwarn = base->MERRWARN;
    s_rx_seed_only_mdatactrl = base->MDATACTRL;
    s_rx_seed_only_mdmactrl = base->MDMACTRL;
    s_rx_seed_only_dma_active = DMA0->COMMON[0].ACTIVE & channelMask;
    s_rx_seed_only_dma_inta = DMA0->COMMON[0].INTA & channelMask;
    s_rx_seed_only_dma_ctlstat = DMA0->CHANNEL[I3C_RX_SEED_ONLY_DMA_CHANNEL].CTLSTAT;
    s_rx_seed_only_dma_xfercfg = DMA0->CHANNEL[I3C_RX_SEED_ONLY_DMA_CHANNEL].XFERCFG;
    s_rx_seed_only_dma_errint = DMA0->COMMON[0].ERRINT & channelMask;
    s_rx_seed_only_rxcount =
        (base->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
    s_rx_seed_only_data0 = s_logical_rx_buffer[0];
    s_rx_seed_only_data1 = s_logical_rx_buffer[1];
    s_rx_seed_only_data2 = s_logical_rx_buffer[2];
    s_rx_seed_only_data3 = s_logical_rx_buffer[3];
    s_rx_seed_only_data4 = s_logical_rx_buffer[4];
    s_rx_seed_only_data5 = s_logical_rx_buffer[5];
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
    masterXfer.dataSize = I3C_RX_SEED_ONLY_LENGTH;

    return I3C_MasterTransferBlocking(base, &masterXfer);
}

static void prepare_rx_seed_only_controller(I3C_Type *base)
{
    CLOCK_EnableClock(I3C_RX_SEED_ONLY_DMA_CLOCK);
    RESET_PeripheralReset(I3C_RX_SEED_ONLY_DMA_RESET);
    DMA0->CTRL = DMA_CTRL_ENABLE(1U);
    DMA0->SRAMBASE = (uint32_t)(uintptr_t)s_dma_descriptor_table;
    memset((void *)s_dma_descriptor_table, 0, sizeof(s_dma_descriptor_table));

    clear_dma0_rx_channel_state();
    I3C_MasterEnableDMA(base, false, false, 1U);
    I3C_MasterDisableInterrupts(base,
                                I3C_PROTOCOL_IRQ_MASK | (uint32_t)kI3C_MasterTxReadyFlag |
                                    (uint32_t)kI3C_MasterRxReadyFlag);

    RESET_PeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, SMART_DMA_TRIGGER_CHANNEL, kINPUTMUX_Dma0IrqToSmartDmaInput);
    INPUTMUX_EnableSignal(INPUTMUX, I3C_RX_SEED_ONLY_DMA_INPUTMUX_SIGNAL, true);
    INPUTMUX_EnableSignal(INPUTMUX, kINPUTMUX_I3c0TxToDmac0Ch25RequestEna, false);
    INPUTMUX_Deinit(INPUTMUX);

    DMA0->CHANNEL[I3C_RX_SEED_ONLY_DMA_CHANNEL].CFG = DMA_CHANNEL_CFG_PERIPHREQEN(1U);
    NVIC_ClearPendingIRQ(I3C_RX_SEED_ONLY_DMA_IRQ);
    NVIC_DisableIRQ(I3C_RX_SEED_ONLY_DMA_IRQ);
    NVIC_ClearPendingIRQ(I3C0_IRQn);
    NVIC_DisableIRQ(I3C0_IRQn);
    NVIC_ClearPendingIRQ(SDMA_IRQn);
    NVIC_DisableIRQ(SDMA_IRQn);
    s_i3c_irq_status_latched = 0U;

    SMARTDMA_Init(
        SMARTDMA_SRAM_ADDR, __smartdma_start__, (uint32_t)((uintptr_t)__smartdma_end__ - (uintptr_t)__smartdma_start__));
    SMARTDMA_Reset();
}

static void configure_rx_seed_only_descriptor(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_ONLY_DMA_CHANNEL);
    dma_descriptor_t *bootstrapDescriptor = &s_dma_descriptor_table[I3C_RX_SEED_ONLY_DMA_CHANNEL];

    DMA0->COMMON[0].INTENSET = channelMask;

    bootstrapDescriptor->xfercfg = DMA_CHANNEL_XFERCFG_CFGVALID(1U) | DMA_CHANNEL_XFERCFG_SETINTA(1U) |
                                   DMA_CHANNEL_XFERCFG_WIDTH(0U) | DMA_CHANNEL_XFERCFG_SRCINC(0U) |
                                   DMA_CHANNEL_XFERCFG_DSTINC(1U) | DMA_CHANNEL_XFERCFG_XFERCOUNT(0U);
    bootstrapDescriptor->srcEndAddr = (const void *)(uintptr_t)I3C_MasterGetRxFifoAddress(base, 1U);
    bootstrapDescriptor->dstEndAddr = &s_logical_rx_buffer[0];
    bootstrapDescriptor->linkToNextDesc = NULL;

    s_rx_seed_only_param.expectedWakeCount = 1U;
    s_rx_seed_only_param.nextRxByteAddress = (uint32_t)(uintptr_t)&s_logical_rx_buffer[1];
    s_rx_seed_only_param.remainingCount = 0U;
    s_rx_seed_only_param.i3cBaseAddress = (uint32_t)(uintptr_t)base;
    s_rx_seed_only_param.dmaIntaAddress = (uint32_t)(uintptr_t)&DMA0->COMMON[0].INTA;
    s_rx_seed_only_param.dmaChannelMask = channelMask;
}

static void arm_rx_seed_only_descriptor(void)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_ONLY_DMA_CHANNEL);

    DMA0->CHANNEL[I3C_RX_SEED_ONLY_DMA_CHANNEL].XFERCFG = s_dma_descriptor_table[I3C_RX_SEED_ONLY_DMA_CHANNEL].xfercfg;
    DMA0->COMMON[0].ENABLESET = channelMask;
    DMA0->COMMON[0].SETVALID = channelMask;
}

static status_t wait_for_rx_seed_only_mailbox(I3C_Type *base)
{
    const uint32_t channelMask = (1UL << I3C_RX_SEED_ONLY_DMA_CHANNEL);
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

        if (s_rx_seed_only_param.mailbox == I3C_RX_SEED_ONLY_MAILBOX_COMPLETION)
        {
            return kStatus_Success;
        }
    }

    return kStatus_Timeout;
}

static status_t run_i3c_rx_seed6_seed_only_no_cpu_irq_probe(I3C_Type *base, uint8_t slaveAddr)
{
    const uint32_t dataIrqBase = s_cm33_i3c_data_irq_count;
    const uint32_t protocolIrqBase = s_cm33_i3c_protocol_irq_count;
    const uint32_t ibiIrqBase = s_cm33_i3c_ibi_irq_count;
    const i3c_tx_trigger_level_t savedTxTriggerLevel =
        (i3c_tx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_TXTRIG_MASK) >> I3C_MDATACTRL_TXTRIG_SHIFT);
    const i3c_rx_trigger_level_t savedRxTriggerLevel =
        (i3c_rx_trigger_level_t)((base->MDATACTRL & I3C_MDATACTRL_RXTRIG_MASK) >> I3C_MDATACTRL_RXTRIG_SHIFT);
    uint32_t dataIrqDelta;
    uint32_t protocolIrqDelta;
    uint32_t ibiIrqDelta;
    status_t result;

    clear_rx_seed_only_probe_state();
    s_active_transfer_length = I3C_RX_SEED_ONLY_LENGTH;
    prepare_logical_payload(I3C_RX_SEED_ONLY_LENGTH);
    memset(s_logical_rx_buffer, 0, sizeof(s_logical_rx_buffer));
    clear_ibi_state();
    clear_post_ibi_handoff_snapshot();

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_ERROR_WRITE, result, 0U, 0U, 0U);
        return result;
    }

    capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_WRITE_STARTED, kStatus_Success, 0U, 0U, 0U);
    result = write_logical_payload_blocking(base, slaveAddr);
    if (result != kStatus_Success)
    {
        capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_ERROR_WRITE, result, 0U, 0U, 0U);
        return result;
    }

    result = ensure_master_idle(base);
    if (result != kStatus_Success)
    {
        capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_ERROR_WRITE, result, 0U, 0U, 0U);
        return result;
    }

    SDK_DelayAtLeastUs(I3C_RX_SEED_ONLY_POST_WRITE_SETTLE_US, SystemCoreClock);

    prepare_rx_seed_only_controller(base);
    I3C_MasterClearErrorStatusFlags(base, I3C_MasterGetErrorStatusFlags(base));
    I3C_MasterClearStatusFlags(base,
                               (uint32_t)kI3C_MasterSlaveStartFlag | (uint32_t)kI3C_MasterControlDoneFlag |
                                   (uint32_t)kI3C_MasterCompleteFlag | (uint32_t)kI3C_MasterArbitrationWonFlag |
                                   (uint32_t)kI3C_MasterSlave2MasterFlag | (uint32_t)kI3C_MasterErrorFlag);
    base->MSTATUS = I3C_MSTATUS_NACKED_MASK;
    base->MDATACTRL |= I3C_MDATACTRL_FLUSHTB_MASK | I3C_MDATACTRL_FLUSHFB_MASK;
    I3C_MasterSetWatermarks(base, savedTxTriggerLevel, kI3C_RxTriggerUntilThreeQuarterOrMore, false, false);

    configure_rx_seed_only_descriptor(base);
    SMARTDMA_Boot(I3C_RX_SEED_ONLY_SMARTDMA_API_INDEX, &s_rx_seed_only_param, 0);
    I3C_MasterEnableDMA(base, false, true, 1U);
    arm_rx_seed_only_descriptor();

    capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_READ_STARTED, kStatus_Success, 0U, 0U, 0U);
    result = I3C_MasterStartWithRxSize(base, kI3C_TypeI3CSdr, slaveAddr, kI3C_Read, I3C_RX_SEED_ONLY_LENGTH);
    if (result == kStatus_Success)
    {
        result = wait_for_rx_seed_only_mailbox(base);
    }

    dataIrqDelta = s_cm33_i3c_data_irq_count - dataIrqBase;
    protocolIrqDelta = s_cm33_i3c_protocol_irq_count - protocolIrqBase;
    ibiIrqDelta = s_cm33_i3c_ibi_irq_count - ibiIrqBase;

    if (result != kStatus_Success)
    {
        capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_ERROR_MAILBOX, result, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        return result;
    }

    capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_MAILBOX, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);

    if ((dataIrqDelta != 0U) || (protocolIrqDelta != 0U) || (ibiIrqDelta != 0U))
    {
        capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_ERROR_IRQ, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        return kStatus_Fail;
    }

    if ((s_rx_seed_only_param.mailbox != I3C_RX_SEED_ONLY_MAILBOX_COMPLETION) ||
        (s_rx_seed_only_param.expectedWakeCount != 1U) || (s_rx_seed_only_param.wakeCount != 1U) ||
        (s_rx_seed_only_param.dmaSeedBytes != 1U) || (s_rx_seed_only_param.smartdmaBytes != 0U) ||
        (s_rx_seed_only_param.dmaIntaCount != 1U))
    {
        capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_ERROR_METRICS, kStatus_Fail, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
        return kStatus_Fail;
    }

    capture_rx_seed_only_snapshot(base, RX_SEED_ONLY_STAGE_SUCCESS, kStatus_Success, dataIrqDelta, protocolIrqDelta, ibiIrqDelta);
    PRINTF("rx-seed6-seed-only wakes=%lu seeds=%lu tails=%lu frame=%lu rxcount=%lu\r\n",
           (unsigned long)s_rx_seed_only_param.wakeCount,
           (unsigned long)s_rx_seed_only_param.dmaSeedBytes,
           (unsigned long)s_rx_seed_only_param.smartdmaBytes,
           (unsigned long)(s_rx_seed_only_param.dmaSeedBytes + s_rx_seed_only_param.smartdmaBytes),
           (unsigned long)s_rx_seed_only_rxcount);
    EXP_LOG_INFO("rx-seed6-seed-only wakes=%lu seeds=%lu tails=%lu frame=%lu rxcount=%lu",
                 (unsigned long)s_rx_seed_only_param.wakeCount,
                 (unsigned long)s_rx_seed_only_param.dmaSeedBytes,
                 (unsigned long)s_rx_seed_only_param.smartdmaBytes,
                 (unsigned long)(s_rx_seed_only_param.dmaSeedBytes + s_rx_seed_only_param.smartdmaBytes),
                 (unsigned long)s_rx_seed_only_rxcount);
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

    PRINTF("\r\nI3C RX seed6 seed-only no-CPU-IRQ probe -- master.\r\n");
    EXP_LOG_INFO("I3C RX seed6 seed-only no-CPU-IRQ probe -- master.");

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

    result = run_i3c_rx_seed6_seed_only_no_cpu_irq_probe(EXAMPLE_MASTER, slaveAddr);
    if (result != kStatus_Success)
    {
        EXP_LOG_ERROR("I3C RX seed6 seed-only no-CPU-IRQ probe failed: %d", result);
        dump_debug_state(EXAMPLE_MASTER);
        set_failure_led();
        return -1;
    }

    PRINTF("I3C RX seed6 seed-only no-CPU-IRQ probe successful.\r\n");
    EXP_LOG_INFO("I3C RX seed6 seed-only no-CPU-IRQ probe successful.");
    set_success_led();
    return 0;
}