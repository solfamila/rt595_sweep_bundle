#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_rx_seed4_seed_only_dma_inputmux_live_poll/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma_inputmux_live_poll" >&2
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

PRINT "rxSeed4InputmuxLive=" \
  " pc=" %HEX Register(PC) \
  " stage=" %HEX Var.VALUE(s_rx_seed4_stage) \
  " result=" %HEX Var.VALUE(s_rx_seed4_result) \
  " dmaIntaCount=" %HEX Var.VALUE(s_rx_seed4_dma_inta_count) \
  " dataIrq=" %HEX Var.VALUE(s_rx_seed4_data_irq_delta) \
  " protocolIrq=" %HEX Var.VALUE(s_rx_seed4_protocol_irq_delta) \
  " ibiIrq=" %HEX Var.VALUE(s_rx_seed4_ibi_irq_delta) \
  " mstatus=" %HEX Var.VALUE(s_rx_seed4_mstatus) \
  " merr=" %HEX Var.VALUE(s_rx_seed4_merrwarn) \
  " mdatactrl=" %HEX Var.VALUE(s_rx_seed4_mdatactrl) \
  " mdmactrl=" %HEX Var.VALUE(s_rx_seed4_mdmactrl) \
  " mintset=" %HEX Var.VALUE(s_rx_seed4_mintset) \
  " mintmasked=" %HEX Var.VALUE(s_rx_seed4_mintmasked) \
  " reqena0=" %HEX Var.VALUE(s_rx_seed4_inputmux_dmac_req_ena0) \
  " dmaActive=" %HEX Var.VALUE(s_rx_seed4_dma_active) \
  " dmaInta=" %HEX Var.VALUE(s_rx_seed4_dma_inta) \
  " dmaCtl=" %HEX Var.VALUE(s_rx_seed4_dma_ctlstat) \
  " dmaCfg=" %HEX Var.VALUE(s_rx_seed4_dma_xfercfg) \
  " dmaErr=" %HEX Var.VALUE(s_rx_seed4_dma_errint) \
  " rxcount=" %HEX Var.VALUE(s_rx_seed4_rxcount) \
  " d0=" %HEX Var.VALUE(s_rx_seed4_data0) \
  " d1=" %HEX Var.VALUE(s_rx_seed4_data1) \
  " d2=" %HEX Var.VALUE(s_rx_seed4_data2) \
  " d3=" %HEX Var.VALUE(s_rx_seed4_data3)

ENDDO
EOF