#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_dma_official_rx_smartdma_wake_chunk_loop/_build/master/evkmimxrt595_ezhb.axf"
capture_out_dir=${TRACE_CAPTURE_OUT_DIR:-$bundle_root/.local/trace-captures}
capture_name=${TRACE_CAPTURE_NAME:-chunkloop_sub10ms_trace_$(date +%Y%m%d_%H%M%S)}
trace32_timeout_seconds=${TRACE_CAPTURE_TIMEOUT_SECONDS:-90}
trace32_wait_ms=${TRACE_CAPTURE_WAIT_MS:-90000}
trace32_run_wait_seconds=${TRACE_CAPTURE_RUN_WAIT_SECONDS:-60}
save_trace_ad=${TRACE_CAPTURE_SAVE_AD:-0}
save_trace_list=${TRACE_CAPTURE_SAVE_LIST:-0}
capture_file=$capture_out_dir/$capture_name.ad
trace_list_file=$capture_out_dir/$capture_name.txt
trace_save_block=
trace_list_block=

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_EXTRA_MASTER_DEFINES='I3C_LOGICAL_CHUNK_COUNT=42 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=1200' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_chunk_loop" >&2
  exit 1
fi

mkdir -p "$capture_out_dir"

if [[ "$save_trace_ad" == 1 ]]; then
  trace_save_block=$(cat <<EOF
Trace.SAVE "$capture_file"
EOF
)
fi

if [[ "$save_trace_list" == 1 ]]; then
  trace_list_block=$(cat <<EOF
PRinTer.FILE "$trace_list_file" ASCIIE
WinPrint.Trace.List
PRinTer.OFF
EOF
)
fi

trace32_run_generated_script "$trace32_timeout_seconds" "$trace32_wait_ms" <<EOF
$(trace32_loader_preamble "$master_axf")

Break.Delete
Break.Set set_success_led /Program
Break.Set set_failure_led /Program

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
WAIT !STATE.RUN() ${trace32_run_wait_seconds}.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

$trace_save_block
$trace_list_block

PRINT "traceCapture=" \
  " records=" Trace.RECORDS()
PRINT "dmaWakeLoopFinal=" \
  " st=" %HEX Var.VALUE(s_dma_official_stage) \
  " out=" %HEX Var.VALUE(s_dma_official_outcome) \
  " rs=" %HEX Var.VALUE(s_dma_official_result) \
  " ec=" %HEX Var.VALUE(s_dma_official_expected_chunk_count) \
  " cc=" %HEX Var.VALUE(s_dma_official_completed_chunk_count) \
  " ci=" %HEX Var.VALUE(s_dma_official_current_chunk_index) \
  " ip=" %HEX Var.VALUE(s_dma_official_ibi_payload_size) \
  " i0=" %HEX Var.VALUE(s_dma_official_ibi_data0) \
  " rx=" %HEX Var.VALUE(s_dma_official_rx_size) \
  " sw=" %HEX Var.VALUE(s_dma_official_total_smartdma_wake_count) \
  " si=" %HEX Var.VALUE(s_dma_official_total_smartdma_dma_inta_count) \
  " di=" %HEX Var.VALUE(s_dma_official_total_dma0_irq_count) \
  " idc=" %HEX Var.VALUE(s_dma_official_total_data_irq_count) \
  " ipc=" %HEX Var.VALUE(s_dma_official_total_protocol_irq_count) \
  " rxc=" %HEX Var.VALUE(s_dma_official_total_rx_dma_callback_count)

ENDDO
EOF

printf 'trace capture complete\n'
if [[ "$save_trace_ad" == 1 ]]; then
  printf '  %s\n' "$capture_file"
fi
if [[ "$save_trace_list" == 1 ]]; then
  printf '  %s\n' "$trace_list_file"
fi