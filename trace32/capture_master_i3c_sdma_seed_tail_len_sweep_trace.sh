#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

master_axf=$(trace32_master_axf_path)
trace32_require_master_axf

bundle_root=$(trace32_bundle_root)
capture_seconds=${TRACE_CAPTURE_SECONDS:-5}
capture_out_dir=${TRACE_CAPTURE_OUT_DIR:-$bundle_root/.local/trace-captures}
capture_name=${TRACE_CAPTURE_NAME:-master_i3c_sdma_seed_tail_len_sweep_trace_$(date +%Y%m%d_%H%M%S)}
capture_file=$capture_out_dir/$capture_name.ad
trace_list_file=$capture_out_dir/$capture_name.txt

mkdir -p "$capture_out_dir"

trace32_run_generated_script $((capture_seconds + 60)) $(((capture_seconds + 2) * 1000)) <<EOF
$(trace32_loader_preamble "$master_axf")

Go.direct init_transfer_led

IF COMBIPROBE()||UTRACE()||Analyzer()
(
  Data.Set ASD:0x40001500 %Long 0x00000000

  Data.Set ASD:0x40004054 %Long 0x00000106
  Data.Set ASD:0x40004058 %Long 0x00000106
  Data.Set ASD:0x4000405C %Long 0x00000106
  Data.Set ASD:0x40004060 %Long 0x00000106
  Data.Set ASD:0x40004064 %Long 0x00000106

  TPIU.PortSize 4
  TPIU.PortMode Continuous
  ITM.DataTrace CorrelatedData
  ITM.ON
  ETM.Trace ON
  ETM.COND ALL
  ETM.ON
)

IF Analyzer()
(
  Trace.METHOD Analyzer
  Trace.AutoInit ON
  Trace.AutoFocus
  Analyzer.TraceCLOCK 198MHz
)
ELSE IF COMBIPROBE()||UTRACE()
(
  Trace.METHOD CAnalyzer
  Trace.AutoInit ON
  CAnalyzer.AutoFocus
  CAnalyzer.TraceCLOCK 198MHz
)

Trace.List
Go
WAIT ${capture_seconds}.s
Break
WAIT !STATE.RUN() 1.s

Trace.SAVE "$capture_file"
PRinTer.FILE "$trace_list_file" ASCIIE
WinPrint.Trace.List
PRinTer.OFF

PRINT "traceCapture=" \
  " ad=$capture_file" \
  " list=$trace_list_file" \
  " records=" Trace.RECORDS()

ENDDO
EOF

printf 'trace capture saved:\n'
printf '  %s\n' "$capture_file"
printf '  %s\n' "$trace_list_file"