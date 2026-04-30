#define keep_smartdma_api_alive keep_smartdma_api_alive_base
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_features_standalone.c"
#undef keep_smartdma_api_alive

void SMARTDMA_CODE EZHB_I3cRxSeed6SeedOnlyNoCpuIrq(void);

void SMARTDMA_CODE EZHB_I3cRxSeed6SeedOnlyNoCpuIrq(void)
{
    E_NOP;
    E_NOP;

    E_PER_READ(R6, EZH_ARM2EZH);
    E_LSR(R6, R6, 2);
    E_LSL(R6, R6, 2);

    E_LOAD_IMM(CFS, 0x0);
    E_LOAD_IMM(CFM, 0x101);

    E_ADD_IMM(R7, PC, 0);
    E_ADD_IMM(PC, PC, 2 * 4);

    E_DCD(rx_seed_only_wait_dma_irq);
    E_DCD(rx_seed_only_complete_mailbox);
    E_DCD(rx_seed_only_end0);

    E_LDR(R5, R7, 0);
    E_COND_GOTO_REGL(EU, R5);

E_LABEL("rx_seed_only_wait_dma_irq");
    E_HOLD;
    E_BCLR_IMM(CFM, CFM, 0);

    E_LDR(R0, R6, PARAM_WAKE_COUNT_INDEX);
    E_ADD_IMM(R0, R0, 1);
    E_STR(R6, R0, PARAM_WAKE_COUNT_INDEX);

    E_LDR(R1, R6, PARAM_DMA_SEED_BYTES_INDEX);
    E_ADD_IMM(R1, R1, 1);
    E_STR(R6, R1, PARAM_DMA_SEED_BYTES_INDEX);

    E_LDR(R1, R6, PARAM_DMA_INTA_COUNT_INDEX);
    E_ADD_IMM(R1, R1, 1);
    E_STR(R6, R1, PARAM_DMA_INTA_COUNT_INDEX);

    E_LDR(R1, R6, PARAM_DMA_INTA_ADDRESS_INDEX);
    E_LDR(R2, R6, PARAM_DMA_CHANNEL_MASK_INDEX);
    E_STR(R1, R2, 0);

E_LABEL("rx_seed_only_complete_mailbox");
    E_LOAD_IMM(R4, EZH_SMARTDMA_MAILBOX_COMPLETION);
    E_STR(R6, R4, PARAM_MAILBOX_INDEX);

E_LABEL("rx_seed_only_end0");
    E_NOP;
    E_GOSUB(rx_seed_only_end0);
}

void keep_smartdma_api_alive(void)
{
    keep_smartdma_api_alive_base();
    g_SMARTDMA_api[2] = (void (*)(void))(((uint32_t)EZHB_I3cRxSeed6SeedOnlyNoCpuIrq + 4U) & (~3U));
}