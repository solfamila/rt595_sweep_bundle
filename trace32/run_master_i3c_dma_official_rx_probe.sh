#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_dma_official_rx_probe/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_probe" >&2
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

PRINT "dmaOfficial=" \
  " pc=" %HEX Register(PC) \
  " st=" %HEX Var.VALUE(s_dma_official_stage) \
  " out=" %HEX Var.VALUE(s_dma_official_outcome) \
  " rs=" %HEX Var.VALUE(s_dma_official_result) \
  " cs=" %HEX Var.VALUE(s_dma_official_completion_status) \
  " sa=" %HEX Var.VALUE(s_dma_official_slave_addr) \
  " dc=" %HEX Var.VALUE(s_dma_official_dev_count) \
  " ibi=" %HEX Var.VALUE(s_dma_official_ibi_payload_size) \
  " id0=" %HEX Var.VALUE(s_dma_official_ibi_data0) \
  " rx=" %HEX Var.VALUE(s_dma_official_rx_size) \
  " mi=" %HEX Var.VALUE(s_dma_official_mismatch_index) \
  " rf=" %HEX Var.VALUE(s_dma_official_rx_first) \
  " rl=" %HEX Var.VALUE(s_dma_official_rx_last) \
  " xc=" %HEX Var.VALUE(g_i3c_dbg_xfer_count) \
  " xd=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_direction) \
  " xs=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_dataSize) \
  " xsa=" %HEX Var.VALUE(g_i3c_dbg_xfer_last_subaddrSize) \
  " xmd=" %HEX Var.VALUE(g_i3c_dbg_xfer_entry_mdatactrl) \
  " xms=" %HEX Var.VALUE(g_i3c_dbg_xfer_entry_mstatus) \
  " xme=" %HEX Var.VALUE(g_i3c_dbg_xfer_entry_merrwarn) \
  " rc=" %HEX Var.VALUE(g_i3c_dbg_rxloop_count) \
  " rds=" %HEX Var.VALUE(g_i3c_dbg_rxloop_dataSize) \
  " rts=" %HEX Var.VALUE(g_i3c_dbg_rxloop_transDataSize) \
  " rbmd=" %HEX Var.VALUE(g_i3c_dbg_rxloop_before_mdatactrl) \
  " rams=" %HEX Var.VALUE(g_i3c_dbg_rxloop_after_mdatactrl) \
  " rbms=" %HEX Var.VALUE(g_i3c_dbg_rxloop_before_mstatus) \
  " rbme=" %HEX Var.VALUE(g_i3c_dbg_rxloop_before_merrwarn) \
  " rfmd=" %HEX Var.VALUE(g_i3c_dbg_rxloop_first_before_mdatactrl) \
  " rfms=" %HEX Var.VALUE(g_i3c_dbg_rxloop_first_before_mstatus) \
  " rfme=" %HEX Var.VALUE(g_i3c_dbg_rxloop_first_before_merrwarn) \
  " rfd=" %HEX Var.VALUE(g_i3c_dbg_rxloop_first_dataSize)

PRINT "dmaOfficialFinal=" \
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
  " rc=" %HEX Var.VALUE(g_i3c_dbg_rxloop_count) \
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
  " b11=" %HEX Var.VALUE(s_dma_official_rx_snapshot[11]) \
  " b12=" %HEX Var.VALUE(s_dma_official_rx_snapshot[12]) \
  " b13=" %HEX Var.VALUE(s_dma_official_rx_snapshot[13]) \
  " b14=" %HEX Var.VALUE(s_dma_official_rx_snapshot[14]) \
  " b15=" %HEX Var.VALUE(s_dma_official_rx_snapshot[15]) \
  " b28=" %HEX Var.VALUE(s_dma_official_rx_snapshot[28]) \
  " b29=" %HEX Var.VALUE(s_dma_official_rx_snapshot[29]) \
  " b30=" %HEX Var.VALUE(s_dma_official_rx_snapshot[30]) \
  " b31=" %HEX Var.VALUE(s_dma_official_rx_snapshot[31]) \
  " b32=" %HEX Var.VALUE(s_dma_official_rx_snapshot[32])

ENDDO
EOF