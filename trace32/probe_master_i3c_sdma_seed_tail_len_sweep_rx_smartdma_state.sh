#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

trace32_run_generated_script 10 2000 <<EOF
PRINT "rxSmartdma=" \
  " stage=" %HEX Var.VALUE(s_rx_smartdma_probe_stage) \
  " result=" %HEX Var.VALUE(s_rx_smartdma_probe_result) \
  " validate=" %HEX Var.VALUE(s_rx_smartdma_validate_reason) \
  " v0=" %HEX Var.VALUE(s_rx_smartdma_validate_value0) \
  " v1=" %HEX Var.VALUE(s_rx_smartdma_validate_value1) \
  " roundStage=" %HEX Var.VALUE(s_roundtrip_read_snapshot.stage) \
  " roundResult=" %HEX Var.VALUE(s_roundtrip_read_snapshot.result) \
  " completion=" %HEX Var.VALUE(s_roundtrip_read_snapshot.completionStatus) \
  " configured=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaConfiguredDataSize) \
  " cb=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaCompletionCallbackCount) \
  " tail=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaReadTailCompleteCount) \
  " pendC=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaCompletionPending) \
  " pendT=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaReadTailPending) \
  " fifoBounce=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaFifoReadyBounceCount) \
  " protoBounce=" %HEX Var.VALUE(s_roundtrip_read_snapshot.smartdmaProtocolBounceCount)

ENDDO
EOF