#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

trace32_run_generated_script 15 8000 <<EOF
Break.Delete
Break.Set capture_chunk_validate_failure /Program
Break.Set capture_failure_snapshot /Program
Break.Set set_failure_led /Program
Break.Set set_success_led /Program

Go
WAIT !STATE.RUN() 5.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

ENDDO
EOF

bash "$SCRIPT_DIR/probe_master_i3c_sdma_seed_tail_len_sweep_replay_state.sh"
bash "$SCRIPT_DIR/probe_master_i3c_sdma_seed_tail_len_sweep_rx_smartdma_state.sh"