#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

trace32_run_generated_script 10 2000 <<EOF
PRINT "fail=" \
  " valid=" %HEX Var.VALUE(s_failure_snapshot.valid) \
  " mstatus=" %HEX Var.VALUE(s_failure_snapshot.i3cMstatus) \
  " mdata=" %HEX Var.VALUE(s_failure_snapshot.i3cMdataCtrl) \
  " mdma=" %HEX Var.VALUE(s_failure_snapshot.i3cMdmaCtrl) \
  " dmaInt=" %HEX Var.VALUE(s_failure_snapshot.dmaIntStat) \
  " dmaAct=" %HEX Var.VALUE(s_failure_snapshot.dmaActive)

ENDDO
EOF