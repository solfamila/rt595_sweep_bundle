#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_dma_official_rx_smartdma_wake_block_stream/_build/master/evkmimxrt595_ezhb.axf"
trace32_timeout_seconds=${RT595_TRACE32_TIMEOUT_SECONDS:-240}
trace32_wait_ms=${RT595_TRACE32_WAIT_MS:-240000}
trace32_run_wait_seconds=${RT595_TRACE32_RUN_WAIT_SECONDS:-180}

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream" >&2
  exit 1
fi

trace32_run_generated_script "$trace32_timeout_seconds" "$trace32_wait_ms" <<EOF
$(trace32_loader_preamble "$master_axf")

Break.Delete
Break.Set set_success_led /Program
Break.Set set_failure_led /Program

Go
WAIT !STATE.RUN() ${trace32_run_wait_seconds}.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

PRINT "dmaWakeBlockStreamFinal=" \
  " st=" %HEX Var.VALUE(s_dma_official_stage) \
  " out=" %HEX Var.VALUE(s_dma_official_outcome) \
  " rs=" %HEX Var.VALUE(s_dma_official_result) \
  " cs=" %HEX Var.VALUE(s_dma_official_completion_status) \
  " sa=" %HEX Var.VALUE(s_dma_official_slave_addr) \
  " ec=" %HEX Var.VALUE(s_dma_official_expected_chunk_count) \
  " cc=" %HEX Var.VALUE(s_dma_official_completed_chunk_count) \
  " ci=" %HEX Var.VALUE(s_dma_official_current_chunk_index) \
  " ip=" %HEX Var.VALUE(s_dma_official_ibi_payload_size) \
  " i0=" %HEX Var.VALUE(s_dma_official_ibi_data0) \
  " i1=" %HEX Var.VALUE(s_dma_official_ibi_data1) \
  " rx=" %HEX Var.VALUE(s_dma_official_rx_size) \
  " mi=" %HEX Var.VALUE(s_dma_official_mismatch_index) \
  " tr=" %HEX Var.VALUE(s_dma_official_tail_recovery_count) \
  " rf=" %HEX Var.VALUE(s_dma_official_rx_first) \
  " rl=" %HEX Var.VALUE(s_dma_official_rx_last) \
  " sm=" %HEX Var.VALUE(s_dma_official_smartdma_mailbox) \
  " sw=" %HEX Var.VALUE(s_dma_official_total_smartdma_wake_count) \
  " si=" %HEX Var.VALUE(s_dma_official_total_smartdma_dma_inta_count) \
  " ss=" %HEX Var.VALUE(s_dma_official_smartdma_dma_inta_snapshot) \
  " smd=" %HEX Var.VALUE(s_dma_official_smartdma_mdmactrl) \
  " sms=" %HEX Var.VALUE(s_dma_official_smartdma_mstatus) \
  " sdc=" %HEX Var.VALUE(s_dma_official_smartdma_mdatactrl) \
  " cu=" %HEX Var.VALUE(s_dma_official_total_chunk_time_us) \
  " wu=" %HEX Var.VALUE(s_dma_official_total_write_wait_us) \
  " iu=" %HEX Var.VALUE(s_dma_official_total_ibi_wait_us) \
  " ru=" %HEX Var.VALUE(s_dma_official_total_read_wait_us) \
  " au=" %HEX Var.VALUE(s_dma_official_total_smartdma_arm_us) \
  " su=" %HEX Var.VALUE(s_dma_official_total_smartdma_wait_us) \
  " xc=" %HEX Var.VALUE(g_i3c_dbg_xfer_count) \
  " xd=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_direction) \
  " xs=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_dataSize) \
  " txc=" %HEX Var.VALUE(g_i3c_dbg_dma_callback_tx_count) \
  " di=" %HEX Var.VALUE(s_dma_official_total_dma0_irq_count) \
  " idc=" %HEX Var.VALUE(s_dma_official_total_data_irq_count) \
  " ipc=" %HEX Var.VALUE(s_dma_official_total_protocol_irq_count) \
  " rxc=" %HEX Var.VALUE(s_dma_official_total_rx_dma_callback_count) \
  " b0=" %HEX Var.VALUE(s_dma_official_rx_snapshot[0]) \
  " b1=" %HEX Var.VALUE(s_dma_official_rx_snapshot[1]) \
  " b2=" %HEX Var.VALUE(s_dma_official_rx_snapshot[2]) \
  " b3=" %HEX Var.VALUE(s_dma_official_rx_snapshot[3]) \
  " b4=" %HEX Var.VALUE(s_dma_official_rx_snapshot[4]) \
  " b5=" %HEX Var.VALUE(s_dma_official_rx_snapshot[5]) \
  " b6=" %HEX Var.VALUE(s_dma_official_rx_snapshot[6]) \
  " b7=" %HEX Var.VALUE(s_dma_official_rx_snapshot[7]) \
  " b8=" %HEX Var.VALUE(s_dma_official_rx_snapshot[8]) \
  " b9=" %HEX Var.VALUE(s_dma_official_rx_snapshot[9]) \
  " b10=" %HEX Var.VALUE(s_dma_official_rx_snapshot[10]) \
  " b11=" %HEX Var.VALUE(s_dma_official_rx_snapshot[11])

ENDDO
EOF