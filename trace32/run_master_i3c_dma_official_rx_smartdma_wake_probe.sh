#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_dma_official_rx_smartdma_wake_probe/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_probe" >&2
  exit 1
fi

trace32_run_generated_script 240 240000 <<EOF
$(trace32_loader_preamble "$master_axf")

Break.Delete
Break.Set set_success_led /Program
Break.Set set_failure_led /Program

Go
WAIT !STATE.RUN() 90.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

PRINT "dmaWakeFinal=" \
  " st=" %HEX Var.VALUE(s_dma_official_stage) \
  " out=" %HEX Var.VALUE(s_dma_official_outcome) \
  " rs=" %HEX Var.VALUE(s_dma_official_result) \
  " cs=" %HEX Var.VALUE(s_dma_official_completion_status) \
  " sa=" %HEX Var.VALUE(s_dma_official_slave_addr) \
  " rx=" %HEX Var.VALUE(s_dma_official_rx_size) \
  " mi=" %HEX Var.VALUE(s_dma_official_mismatch_index) \
  " tr=" %HEX Var.VALUE(s_dma_official_tail_recovery_count) \
  " rf=" %HEX Var.VALUE(s_dma_official_rx_first) \
  " rl=" %HEX Var.VALUE(s_dma_official_rx_last) \
  " sm=" %HEX Var.VALUE(s_dma_official_smartdma_mailbox) \
  " sw=" %HEX Var.VALUE(s_dma_official_smartdma_wake_count) \
  " si=" %HEX Var.VALUE(s_dma_official_smartdma_dma_inta_count) \
  " ss=" %HEX Var.VALUE(s_dma_official_smartdma_dma_inta_snapshot) \
  " smd=" %HEX Var.VALUE(s_dma_official_smartdma_mdmactrl) \
  " sms=" %HEX Var.VALUE(s_dma_official_smartdma_mstatus) \
  " sdc=" %HEX Var.VALUE(s_dma_official_smartdma_mdatactrl) \
  " xc=" %HEX Var.VALUE(g_i3c_dbg_xfer_count) \
  " xd=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_direction) \
  " xs=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_dataSize) \
  " txc=" %HEX Var.VALUE(g_i3c_dbg_dma_callback_tx_count) \
  " di=" %HEX Var.VALUE(g_dma0_dbg_irq_count) \
  " idc=" %HEX Var.VALUE(g_i3c_dbg_irq_data_count) \
  " ipc=" %HEX Var.VALUE(g_i3c_dbg_irq_protocol_count) \
  " rxc=" %HEX Var.VALUE(g_i3c_dbg_dma_callback_rx_count) \
  " b0=" %HEX Var.VALUE(s_dma_official_rx_snapshot[0]) \
  " b1=" %HEX Var.VALUE(s_dma_official_rx_snapshot[1]) \
  " b2=" %HEX Var.VALUE(s_dma_official_rx_snapshot[2]) \
  " b3=" %HEX Var.VALUE(s_dma_official_rx_snapshot[3]) \
  " b4=" %HEX Var.VALUE(s_dma_official_rx_snapshot[4]) \
  " b5=" %HEX Var.VALUE(s_dma_official_rx_snapshot[5])

ENDDO
EOF