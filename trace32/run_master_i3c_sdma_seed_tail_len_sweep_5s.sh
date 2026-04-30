#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

master_axf=$(trace32_master_axf_path)
trace32_require_master_axf

trace32_run_generated_script 15 8000 <<EOF
$(trace32_loader_preamble "$master_axf")

Break.Delete
Go
WAIT 5.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

ENDDO
EOF

bash "$SCRIPT_DIR/probe_master_i3c_sdma_seed_tail_len_sweep_replay_state.sh"
bash "$SCRIPT_DIR/probe_master_i3c_sdma_seed_tail_len_sweep_rx_smartdma_state.sh"