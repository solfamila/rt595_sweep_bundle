#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_rx_request_semantics_probe/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none ./run_experiment.sh master_i3c_rx_request_semantics_probe" >&2
  exit 1
fi

trace32_run_generated_script 240 230000 <<EOF
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

PRINT "rxProbe=" \
  " pc=" %HEX Register(PC) \
  " mode=" %HEX Var.VALUE(s_rx_request_probe_mode) \
  " stage=" %HEX Var.VALUE(s_rx_request_probe_stage) \
  " result=" %HEX Var.VALUE(s_rx_request_probe_result) \
  " dmaInta=" %HEX Var.VALUE(s_rx_request_probe_dma_inta_count) \
  " dataIrqDelta=" %HEX Var.VALUE(s_rx_request_probe_data_irq_delta) \
  " protocolIrqDelta=" %HEX Var.VALUE(s_rx_request_probe_protocol_irq_delta) \
  " ibiIrqDelta=" %HEX Var.VALUE(s_rx_request_probe_ibi_irq_delta) \
  " status=" %HEX Var.VALUE(s_rx_request_probe_status) \
  " err=" %HEX Var.VALUE(s_rx_request_probe_err_status) \
  " mdatactrl=" %HEX Var.VALUE(s_rx_request_probe_mdatactrl) \
  " d0=" %HEX Var.VALUE(s_rx_request_probe_data0) \
  " d1=" %HEX Var.VALUE(s_rx_request_probe_data1) \
  " d2=" %HEX Var.VALUE(s_rx_request_probe_data2) \
  " d3=" %HEX Var.VALUE(s_rx_request_probe_data3)

ENDDO
EOF