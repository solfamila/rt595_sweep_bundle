#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_dma_official_rx_smartdma_wake_block_stream/_build/master/evkmimxrt595_ezhb.axf"
capture_out_dir=${TRACE_CAPTURE_OUT_DIR:-$bundle_root/.local/trace-captures}
capture_name=${TRACE_CAPTURE_NAME:-blockstream_chunk1_trace_$(date +%Y%m%d_%H%M%S)}
trace32_timeout_seconds=${TRACE_CAPTURE_TIMEOUT_SECONDS:-240}
trace32_wait_ms=${TRACE_CAPTURE_WAIT_MS:-240000}
trace32_run_wait_seconds=${TRACE_CAPTURE_RUN_WAIT_SECONDS:-180}
save_trace_ad=${TRACE_CAPTURE_SAVE_AD:-1}
capture_file=$capture_out_dir/$capture_name.ad
trace_save_block=

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=32 I3C_STREAM_BLOCK_COUNT=4 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream" >&2
  exit 1
fi

mkdir -p "$capture_out_dir"

if [[ "$save_trace_ad" == 1 ]]; then
  trace_save_block=$(cat <<EOF
Trace.SAVE "$capture_file"
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

PRINT "traceCapture=" \
  " records=" Trace.RECORDS() \
  " ad=$capture_file"
PRINT "dmaWakeBlockStreamFinal=" \
  " st=" %HEX Var.VALUE(s_dma_official_stage) \
  " out=" %HEX Var.VALUE(s_dma_official_outcome) \
  " rs=" %HEX Var.VALUE(s_dma_official_result) \
  " cs=" %HEX Var.VALUE(s_dma_official_completion_status) \
  " sa=" %HEX Var.VALUE(s_dma_official_slave_addr) \
  " ec=" %HEX Var.VALUE(s_dma_official_expected_chunk_count) \
  " cc=" %HEX Var.VALUE(s_dma_official_completed_chunk_count) \
  " ci=" %HEX Var.VALUE(s_dma_official_current_chunk_index) \
  " ip=" %HEX Var.VALUE(s_dma_official_ibi_payload_size) \
  " i0=" %HEX Var.VALUE(s_dma_official_ibi_data0) \
  " i1=" %HEX Var.VALUE(s_dma_official_ibi_data1) \
  " rx=" %HEX Var.VALUE(s_dma_official_rx_size) \
  " mi=" %HEX Var.VALUE(s_dma_official_mismatch_index)
PRINT "dmaWakeBlockStreamProbe=" \
  " p2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_step) \
  " r2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_result) \
  " q2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_state) \
  " f2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_status) \
  " e2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_error) \
  " m2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_mailbox) \
  " w2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_wake_count) \
  " d2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_dma_seed_bytes) \
  " y2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_smartdma_bytes) \
  " n2=" %HEX Var.VALUE(s_dma_official_seed_tail_probe_remaining_count) \
  " wi=" %HEX Var.VALUE(s_rx_seed_read_param.wakeCount) \
  " ii=" %HEX Var.VALUE(s_rx_seed_read_param.dmaIntaCount) \
  " mi2=" %HEX Var.VALUE(s_rx_seed_read_param.mailbox) \
  " ri=" %HEX Var.VALUE(s_rx_seed_read_param.remainingCount)
PRINT "dmaWakeBlockStreamBytes=" \
  " mi=" %HEX Var.VALUE(s_dma_official_mismatch_index) \
  " ri=" %HEX Var.VALUE(s_rx_seed_read_param.remainingCount) \
  " sd=" %HEX Var.VALUE(s_rx_seed_read_param.dmaSeedBytes) \
  " r6=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[6]) \
  " r7=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[7]) \
  " r8=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[8]) \
  " r9=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[9]) \
  " rE=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[14]) \
  " rF=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[15]) \
  " r10=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[16]) \
  " r11=" %HEX Var.VALUE(s_dma_official_seed_tail_raw_snapshot[17]) \
  " b0=" %HEX Var.VALUE(g_chunk_rxBuff[0]) \
  " b1=" %HEX Var.VALUE(g_chunk_rxBuff[1]) \
  " b2=" %HEX Var.VALUE(g_chunk_rxBuff[2]) \
  " b3=" %HEX Var.VALUE(g_chunk_rxBuff[3]) \
  " b8=" %HEX Var.VALUE(g_chunk_rxBuff[8]) \
  " b9=" %HEX Var.VALUE(g_chunk_rxBuff[9]) \
  " bA=" %HEX Var.VALUE(g_chunk_rxBuff[10]) \
  " bB=" %HEX Var.VALUE(g_chunk_rxBuff[11]) \
  " b18=" %HEX Var.VALUE(g_chunk_rxBuff[24]) \
  " b19=" %HEX Var.VALUE(g_chunk_rxBuff[25]) \
  " b1A=" %HEX Var.VALUE(g_chunk_rxBuff[26]) \
  " b1B=" %HEX Var.VALUE(g_chunk_rxBuff[27]) \
  " b1C=" %HEX Var.VALUE(g_chunk_rxBuff[28]) \
  " b1D=" %HEX Var.VALUE(g_chunk_rxBuff[29]) \
  " b1E=" %HEX Var.VALUE(g_chunk_rxBuff[30]) \
  " b1F=" %HEX Var.VALUE(g_chunk_rxBuff[31])

ENDDO
EOF

printf 'trace capture complete\n'
if [[ "$save_trace_ad" == 1 ]]; then
  printf '  %s\n' "$capture_file"
fi