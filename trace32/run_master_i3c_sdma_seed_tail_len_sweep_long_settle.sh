#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

master_axf=$(trace32_master_axf_path)
trace32_require_master_axf

trace32_run_generated_script 240 230000 <<EOF
$(trace32_loader_preamble "$master_axf")

Break.Delete
Break.Set set_success_led /Program
Break.Set set_failure_led /Program

Go
WAIT !STATE.RUN() 180.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

PRINT "longRun=" \
  " pc=" %HEX Register(PC) \
  " logical=" %HEX Var.VALUE(s_length_sweep_probe_logical) \
  " chunk=" %HEX Var.VALUE(s_length_sweep_probe_chunk) \
  " lenSweepStage=" %HEX Var.VALUE(s_length_sweep_probe_stage) \
  " dmaStage=" %HEX Var.VALUE(s_dma_probe_stage) \
  " dmaResult=" %HEX Var.VALUE(s_dma_probe_result) \
  " roundStage=" %HEX Var.VALUE(s_roundtrip_read_snapshot.stage) \
  " roundResult=" %HEX Var.VALUE(s_roundtrip_read_snapshot.result) \
  " ibiCount=" %HEX Var.VALUE(s_ibi_payload_count) \
  " chunkValidateReason=" %HEX Var.VALUE(s_chunk_validate_reason)

ENDDO
EOF