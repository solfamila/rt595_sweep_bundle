#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

trace32_run_generated_script 10 2000 <<EOF
PRINT "replay=" \
  " pc=" %HEX Register(PC) \
  " logical=" %HEX Var.VALUE(s_length_sweep_probe_logical) \
  " offset=" %HEX Var.VALUE(s_length_sweep_probe_offset) \
  " chunk=" %HEX Var.VALUE(s_length_sweep_probe_chunk) \
  " chunkIndex=" %HEX Var.VALUE(s_length_sweep_probe_chunk_index) \
  " lenSweepStage=" %HEX Var.VALUE(s_length_sweep_probe_stage) \
  " dmaStage=" %HEX Var.VALUE(s_dma_probe_stage) \
  " dmaResult=" %HEX Var.VALUE(s_dma_probe_result) \
  " roundStage=" %HEX Var.VALUE(s_roundtrip_read_snapshot.stage) \
  " roundResult=" %HEX Var.VALUE(s_roundtrip_read_snapshot.result) \
  " ibiCount=" %HEX Var.VALUE(s_ibi_payload_count) \
  " chunkValidateReason=" %HEX Var.VALUE(s_chunk_validate_reason) \
  " failValid=" %HEX Var.VALUE(s_failure_snapshot.valid)

ENDDO
EOF