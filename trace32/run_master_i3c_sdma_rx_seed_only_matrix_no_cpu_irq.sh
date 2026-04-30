#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_sdma_rx_seed_only_matrix_no_cpu_irq/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none ./run_experiment.sh master_i3c_sdma_rx_seed_only_matrix_no_cpu_irq" >&2
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

PRINT "rxSeedMatrix=" \
  " pc=" %HEX Register(PC) \
  " case=" %HEX Var.VALUE(s_rx_seed_matrix_last_case_index) \
  " len=" %HEX Var.VALUE(s_rx_seed_matrix_length) \
  " trig=" %HEX Var.VALUE(s_rx_seed_matrix_rx_trigger) \
  " stage=" %HEX Var.VALUE(s_rx_seed_matrix_stage) \
  " result=" %HEX Var.VALUE(s_rx_seed_matrix_result) \
  " act=" %HEX Var.VALUE(s_rx_seed_matrix_dma_active) \
  " inta=" %HEX Var.VALUE(s_rx_seed_matrix_dma_inta) \
  " rx=" %HEX Var.VALUE(s_rx_seed_matrix_rxcount) \
  " c0=" %HEX Var.VALUE(s_rx_seed_matrix_case_length[0]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rx_trigger[0]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_result[0]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_wakes[0]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_dma_inta_count[0]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rxcount[0]) \
  " c1=" %HEX Var.VALUE(s_rx_seed_matrix_case_length[1]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rx_trigger[1]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_result[1]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_wakes[1]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_dma_inta_count[1]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rxcount[1]) \
  " c2=" %HEX Var.VALUE(s_rx_seed_matrix_case_length[2]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rx_trigger[2]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_result[2]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_wakes[2]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_dma_inta_count[2]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rxcount[2]) \
  " c3=" %HEX Var.VALUE(s_rx_seed_matrix_case_length[3]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rx_trigger[3]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_result[3]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_wakes[3]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_dma_inta_count[3]) "/" %HEX Var.VALUE(s_rx_seed_matrix_case_rxcount[3])

ENDDO
EOF