#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_rx_len6_full_len_dma_inta_poll/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none ./run_experiment.sh master_i3c_rx_len6_full_len_dma_inta_poll" >&2
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

PRINT "rxFull6=" \
  " pc=" %HEX Register(PC) \
  " st=" %HEX Var.VALUE(s_rx_full6_stage) \
  " rs=" %HEX Var.VALUE(s_rx_full6_result) \
  " ic=" %HEX Var.VALUE(s_rx_full6_dma_inta_count) \
  " di=" %HEX Var.VALUE(s_rx_full6_data_irq_delta) \
  " pi=" %HEX Var.VALUE(s_rx_full6_protocol_irq_delta) \
  " ii=" %HEX Var.VALUE(s_rx_full6_ibi_irq_delta) \
  " ms=" %HEX Var.VALUE(s_rx_full6_mstatus) \
  " me=" %HEX Var.VALUE(s_rx_full6_merrwarn) \
  " md=" %HEX Var.VALUE(s_rx_full6_mdatactrl) \
  " mc=" %HEX Var.VALUE(s_rx_full6_mdmactrl) \
  " act=" %HEX Var.VALUE(s_rx_full6_dma_active) \
  " inta=" %HEX Var.VALUE(s_rx_full6_dma_inta) \
  " cfg=" %HEX Var.VALUE(s_rx_full6_dma_cfg) \
  " ctl=" %HEX Var.VALUE(s_rx_full6_dma_ctlstat) \
  " xcfg=" %HEX Var.VALUE(s_rx_full6_dma_xfercfg) \
  " xcnt=" %HEX Var.VALUE(s_rx_full6_dma_xfercount_field) \
  " err=" %HEX Var.VALUE(s_rx_full6_dma_errint) \
  " rx=" %HEX Var.VALUE(s_rx_full6_rxcount) \
  " ds=" %HEX Var.VALUE(s_rx_full6_desc_src_end) \
  " dd=" %HEX Var.VALUE(s_rx_full6_desc_dst_end) \
  " dc=" %HEX Var.VALUE(s_rx_full6_desc_xfercfg) \
  " dn=" %HEX Var.VALUE(s_rx_full6_desc_xfercount) \
  " d0=" %HEX Var.VALUE(s_rx_full6_data0) \
  " d1=" %HEX Var.VALUE(s_rx_full6_data1) \
  " d2=" %HEX Var.VALUE(s_rx_full6_data2) \
  " d3=" %HEX Var.VALUE(s_rx_full6_data3) \
  " d4=" %HEX Var.VALUE(s_rx_full6_data4) \
  " d5=" %HEX Var.VALUE(s_rx_full6_data5)

ENDDO
EOF