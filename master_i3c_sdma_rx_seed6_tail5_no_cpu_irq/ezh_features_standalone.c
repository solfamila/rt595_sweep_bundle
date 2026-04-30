#define keep_smartdma_api_alive keep_smartdma_api_alive_base
#include "../master_i3c_sdma_seed_tail_len_sweep/ezh_features_standalone.c"
#undef keep_smartdma_api_alive

#define I3C_RX_SEED6_TAIL5_TAIL_BYTES 5U

void SMARTDMA_CODE EZHB_I3cRxSeed6Tail5NoCpuIrq(void);

void SMARTDMA_CODE EZHB_I3cRxSeed6Tail5NoCpuIrq(void)
{
    E_NOP;
    E_NOP;

    E_PER_READ(R6, EZH_ARM2EZH);
    E_LSR(R6, R6, 2);
    E_LSL(R6, R6, 2);

    E_LOAD_IMM(CFS, 0x0);
    E_LOAD_IMM(CFM, 0x101);

    E_ADD_IMM(R7, PC, 0);
    E_ADD_IMM(PC, PC, 5 * 4);

    E_DCD(rx_wait_dma_irq);
    E_DCD(rx_tail_loop);
    E_DCD(rx_after_block);
    E_DCD(rx_complete_mailbox);
    E_DCD(rx_end0);

    E_LDR(R5, R7, 0);
    E_COND_GOTO_REGL(EU, R5);

E_LABEL("rx_wait_dma_irq");
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

    E_LDR(R0, R6, PARAM_NEXT_TX_BYTE_INDEX);
    E_LDR(R1, R6, PARAM_REMAINING_COUNT_INDEX);
    E_LDR(R2, R6, PARAM_I3C_BASE_ADDRESS_INDEX);

    E_SUB_IMMS(R5, R1, 0);
    E_LDR(R5, R7, 3);
    E_COND_GOTO_REGL(ZE, R5);

    E_LOAD_SIMM(R5, I3C_MRDATAB_OFFSET, 0);
    E_ADD(R3, R2, R5);
    E_LOAD_IMM(R4, I3C_RX_SEED6_TAIL5_TAIL_BYTES);

    E_LDR(R5, R7, 1);
    E_COND_GOTO_REGL(EU, R5);

E_LABEL("rx_tail_loop");
    E_SUB_IMMS(R5, R4, 0);
    E_LDR(R5, R7, 2);
    E_COND_GOTO_REGL(ZE, R5);

    E_LDR(R5, R3, 0);
    E_STRB(R0, R5, 0);
    E_ADD_IMM(R0, R0, 1);
    E_SUB_IMMS(R1, R1, 1);
    E_SUB_IMMS(R4, R4, 1);

    E_LDR(R5, R6, PARAM_SMARTDMA_BYTES_INDEX);
    E_ADD_IMM(R5, R5, 1);
    E_STR(R6, R5, PARAM_SMARTDMA_BYTES_INDEX);

    E_SUB_IMMS(R5, R1, 0);
    E_LDR(R5, R7, 3);
    E_COND_GOTO_REGL(ZE, R5);

    E_LDR(R5, R7, 1);
    E_COND_GOTO_REGL(EU, R5);

E_LABEL("rx_after_block");
    E_ADD_IMM(R0, R0, 1);
    E_STR(R6, R0, PARAM_NEXT_TX_BYTE_INDEX);
    E_STR(R6, R1, PARAM_REMAINING_COUNT_INDEX);

    E_LDR(R5, R7, 0);
    E_COND_GOTO_REGL(EU, R5);

E_LABEL("rx_complete_mailbox");
    E_STR(R6, R0, PARAM_NEXT_TX_BYTE_INDEX);
    E_LOAD_IMM(R4, 0x0);
    E_STR(R6, R4, PARAM_REMAINING_COUNT_INDEX);
    E_LOAD_IMM(R4, EZH_SMARTDMA_MAILBOX_COMPLETION);
    E_STR(R6, R4, PARAM_MAILBOX_INDEX);

    E_LDR(R5, R7, 4);
    E_COND_GOTO_REGL(EU, R5);

E_LABEL("rx_end0");
    E_NOP;
    E_GOSUB(rx_end0);
}

void keep_smartdma_api_alive(void)
{
    keep_smartdma_api_alive_base();
    g_SMARTDMA_api[2] = (void (*)(void))(((uint32_t)EZHB_I3cRxSeed6Tail5NoCpuIrq + 4U) & (~3U));
}