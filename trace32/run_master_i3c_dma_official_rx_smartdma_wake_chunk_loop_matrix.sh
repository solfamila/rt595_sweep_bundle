#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
experiment=master_i3c_dma_official_rx_smartdma_wake_chunk_loop
runner="$SCRIPT_DIR/run_master_i3c_dma_official_rx_smartdma_wake_chunk_loop.sh"

mode=${RT595_CHUNK_LOOP_MATRIX_MODE:-both}
chunk_counts_text=${RT595_CHUNK_LOOP_MATRIX_CHUNK_COUNTS:-"42"}
settles_text=${RT595_CHUNK_LOOP_MATRIX_SETTLES_US:-"10000 1000 100 10 0"}
remainder_bytes_text=${RT595_CHUNK_LOOP_MATRIX_TOTAL_BYTES:-"253 257 511 1025"}
remainder_settle_us=${RT595_CHUNK_LOOP_MATRIX_REMAINDER_SETTLE_US:-10000}
output_file=${RT595_CHUNK_LOOP_MATRIX_OUTPUT:-"$bundle_root/.local/chunk_loop_matrix_results.tsv"}

extract_field() {
  local key=$1
  local signature=$2

  awk -v key="$key" '{for (i = 1; i <= NF; i++) if ($i ~ ("^" key "=")) {sub("^" key "=", "", $i); print $i; exit}}' <<<"$signature"
}

write_line() {
  local line=$1

  printf '%s\n' "$line"
  printf '%s\n' "$line" >>"$output_file"
}

run_case() {
  local case_name=$1
  local case_kind=$2
  local requested_bytes=$3
  local requested_chunks=$4
  local settle_us=$5
  local defines=$6
  local build_output
  local build_status
  local run_output=""
  local run_status=0
  local elapsed_s
  local signature
  local status=unknown
  local stage=""
  local result=""
  local expected_chunks=""
  local completed_chunks=""
  local chunk_index=""
  local ibi_tag=""
  local rx_bytes=""
  local smartdma_wakes=""
  local dma_inta_count=""
  local cm33_dma_irq=""
  local cm33_data_irq=""
  local protocol_irq=""
  local rx_dma_callback=""
  local mismatch_index=""
  local out=""
  local line
  local case_start=$SECONDS

  printf 'Running %s (%s)\n' "$case_name" "$defines" >&2

  set +e
  build_output=$(cd "$bundle_root" && RT595_EXTRA_MASTER_DEFINES="$defines" RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh "$experiment" 2>&1)
  build_status=$?
  set -e

  if [[ $build_status -ne 0 ]]; then
    status=build-fail
    printf '%s\n' "$build_output" | tail -n 20 >&2
  else
    set +e
    run_output=$("$runner" 2>&1)
    run_status=$?
    set -e
    if [[ $run_status -ne 0 ]]; then
      status=runner-fail
      printf '%s\n' "$run_output" | tail -n 20 >&2
    fi
  fi

  elapsed_s=$((SECONDS - case_start))
  signature=$(printf '%s\n' "$run_output" | grep 'dmaWakeLoopFinal=' | tail -n 1 | sed 's/^.*msg=//' || true)

  if [[ -n "$signature" ]]; then
    out=$(extract_field out "$signature")
    stage=$(extract_field st "$signature")
    result=$(extract_field rs "$signature")
    expected_chunks=$(extract_field ec "$signature")
    completed_chunks=$(extract_field cc "$signature")
    chunk_index=$(extract_field ci "$signature")
    ibi_tag=$(extract_field i0 "$signature")
    rx_bytes=$(extract_field rx "$signature")
    smartdma_wakes=$(extract_field sw "$signature")
    dma_inta_count=$(extract_field si "$signature")
    cm33_dma_irq=$(extract_field di "$signature")
    cm33_data_irq=$(extract_field idc "$signature")
    protocol_irq=$(extract_field ipc "$signature")
    rx_dma_callback=$(extract_field rxc "$signature")
    mismatch_index=$(extract_field mi "$signature")
  fi

  if [[ $build_status -eq 0 && $run_status -eq 0 ]]; then
    if [[ "$out" == "1" && "$result" == "0" ]]; then
      status=pass
    elif [[ -z "$status" || "$status" == unknown ]]; then
      status=fail
    fi
  fi

  printf -v line '%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s' \
    "$case_name" "$case_kind" "$requested_bytes" "$requested_chunks" "$settle_us" "$elapsed_s" "$status" \
    "$build_status" "$run_status" "$stage" "$result" "$expected_chunks" "$completed_chunks" "$chunk_index" \
    "$ibi_tag" "$rx_bytes" "$smartdma_wakes" "$dma_inta_count" "$cm33_dma_irq" "$cm33_data_irq" \
    "$protocol_irq" "$rx_dma_callback" "$mismatch_index" "$signature"
  write_line "$line"
}

case "$mode" in
  chunk|chunks|remainder|remainders|both)
    ;;
  *)
    echo "unsupported RT595_CHUNK_LOOP_MATRIX_MODE: $mode" >&2
    exit 2
    ;;
esac

mkdir -p "$(dirname "$output_file")"
: >"$output_file"

write_line 'case_name	kind	requested_bytes	requested_chunks	settle_us	elapsed_s	status	build_status	run_status	stage	result	expected_chunks	completed_chunks	final_chunk_index	final_ibi_tag	validated_rx_bytes	smartdma_wakes	dma_inta_count	cm33_dma_irq	cm33_data_irq	protocol_irq	rx_dma_callback	mismatch_index	signature'

if [[ "$mode" == chunk || "$mode" == chunks || "$mode" == both ]]; then
  read -r -a chunk_counts <<<"$chunk_counts_text"
  read -r -a settles <<<"$settles_text"
  for chunk_count in "${chunk_counts[@]}"; do
    for settle_us in "${settles[@]}"; do
      run_case \
        "chunks_${chunk_count}_settle_${settle_us}" \
        chunks \
        $((chunk_count * 6)) \
        "$chunk_count" \
        "$settle_us" \
        "I3C_LOGICAL_CHUNK_COUNT=${chunk_count} I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=${settle_us}"
    done
  done
fi

if [[ "$mode" == remainder || "$mode" == remainders || "$mode" == both ]]; then
  read -r -a remainder_bytes <<<"$remainder_bytes_text"
  for total_bytes in "${remainder_bytes[@]}"; do
    run_case \
      "bytes_${total_bytes}_settle_${remainder_settle_us}" \
      remainders \
      "$total_bytes" \
      $(((total_bytes + 5) / 6)) \
      "$remainder_settle_us" \
      "I3C_LOGICAL_TOTAL_BYTES=${total_bytes} I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=${remainder_settle_us}"
  done
fi

printf 'Wrote results to %s\n' "$output_file" >&2