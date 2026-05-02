#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
experiment=master_i3c_dma_official_rx_smartdma_wake_block_stream
runner="$SCRIPT_DIR/run_master_i3c_dma_official_rx_smartdma_wake_block_stream.sh"

block_bytes=${RT595_BLOCK_STREAM_MATRIX_BLOCK_BYTES:-32}
counts_text=${RT595_BLOCK_STREAM_MATRIX_COUNTS:-"4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768"}
settles_text=${RT595_BLOCK_STREAM_MATRIX_SETTLES_US:-"0"}
stop_on_fail=${RT595_BLOCK_STREAM_MATRIX_STOP_ON_FAIL:-1}
output_file=${RT595_BLOCK_STREAM_MATRIX_OUTPUT:-"$bundle_root/.local/block_stream_count_sweep_results.tsv"}

extract_field() {
  local key=$1
  local signature=$2

  awk -v key="$key" '{for (i = 1; i <= NF; i++) if ($i ~ ("^" key "=")) {sub("^" key "=", "", $i); print $i; exit}}' <<<"$signature"
}

hex_to_dec() {
  local value=${1:-}

  if [[ -z "$value" ]]; then
    printf ''
    return 0
  fi

  printf '%s' "$((16#$value))"
}

rate_kib_per_s() {
  local bytes=$1
  local usec=$2

  if [[ -z "$usec" || "$usec" == 0 ]]; then
    printf ''
    return 0
  fi

  awk -v bytes="$bytes" -v usec="$usec" 'BEGIN { printf "%.1f", (bytes * 1000000.0 / usec) / 1024.0 }'
}

write_line() {
  local line=$1

  printf '%s\n' "$line"
  printf '%s\n' "$line" >>"$output_file"
}

run_case() {
  local block_count=$1
  local settle_us=$2
  local requested_bytes=$((block_count * block_bytes))
  local defines="I3C_STREAM_BLOCK_BYTES=${block_bytes} I3C_STREAM_BLOCK_COUNT=${block_count} I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=${settle_us}"
  local case_name="blocks_${block_count}_bytes_${block_bytes}_settle_${settle_us}"
  local build_output
  local build_status
  local run_output=""
  local run_status=0
  local elapsed_s
  local signature=""
  local case_status=unknown
  local stage=""
  local result=""
  local expected_chunks=""
  local completed_chunks=""
  local chunk_index=""
  local rx_bytes=""
  local mismatch_index=""
  local smartdma_wakes=""
  local dma_inta_count=""
  local chunk_time_hex=""
  local write_wait_hex=""
  local ibi_wait_hex=""
  local read_wait_hex=""
  local smartdma_arm_hex=""
  local expected_chunks_dec=""
  local completed_chunks_dec=""
  local chunk_index_dec=""
  local rx_bytes_dec=""
  local smartdma_wakes_dec=""
  local dma_inta_count_dec=""
  local chunk_time_us=""
  local write_wait_us=""
  local ibi_wait_us=""
  local read_wait_us=""
  local smartdma_arm_us=""
  local full_kib_per_s=""
  local read_kib_per_s=""
  local out=""
  local line
  local case_start=$SECONDS

  printf 'Running %s (%s)\n' "$case_name" "$defines" >&2

  set +e
  build_output=$(cd "$bundle_root" && RT595_EXTRA_MASTER_DEFINES="$defines" RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh "$experiment" 2>&1)
  build_status=$?
  set -e

  if [[ $build_status -ne 0 ]]; then
    case_status=build-fail
    printf '%s\n' "$build_output" | tail -n 20 >&2
  else
    set +e
    run_output=$(RT595_EXTRA_MASTER_DEFINES="$defines" "$runner" 2>&1)
    run_status=$?
    set -e
    if [[ $run_status -ne 0 ]]; then
      case_status=runner-fail
      printf '%s\n' "$run_output" | tail -n 20 >&2
    fi
  fi

  elapsed_s=$((SECONDS - case_start))
  signature=$(printf '%s\n' "$run_output" | grep 'dmaWakeBlockStreamFinal=' | tail -n 1 | sed 's/^.*msg=//' || true)

  if [[ -n "$signature" ]]; then
    out=$(extract_field out "$signature")
    stage=$(extract_field st "$signature")
    result=$(extract_field rs "$signature")
    expected_chunks=$(extract_field ec "$signature")
    completed_chunks=$(extract_field cc "$signature")
    chunk_index=$(extract_field ci "$signature")
    rx_bytes=$(extract_field rx "$signature")
    mismatch_index=$(extract_field mi "$signature")
    smartdma_wakes=$(extract_field sw "$signature")
    dma_inta_count=$(extract_field si "$signature")
    chunk_time_hex=$(extract_field cu "$signature")
    write_wait_hex=$(extract_field wu "$signature")
    ibi_wait_hex=$(extract_field iu "$signature")
    read_wait_hex=$(extract_field ru "$signature")
    smartdma_arm_hex=$(extract_field au "$signature")

    expected_chunks_dec=$(hex_to_dec "$expected_chunks")
    completed_chunks_dec=$(hex_to_dec "$completed_chunks")
    chunk_index_dec=$(hex_to_dec "$chunk_index")
    rx_bytes_dec=$(hex_to_dec "$rx_bytes")
    smartdma_wakes_dec=$(hex_to_dec "$smartdma_wakes")
    dma_inta_count_dec=$(hex_to_dec "$dma_inta_count")
    chunk_time_us=$(hex_to_dec "$chunk_time_hex")
    write_wait_us=$(hex_to_dec "$write_wait_hex")
    ibi_wait_us=$(hex_to_dec "$ibi_wait_hex")
    read_wait_us=$(hex_to_dec "$read_wait_hex")
    smartdma_arm_us=$(hex_to_dec "$smartdma_arm_hex")
    full_kib_per_s=$(rate_kib_per_s "$requested_bytes" "$chunk_time_us")
    read_kib_per_s=$(rate_kib_per_s "$requested_bytes" "$read_wait_us")

    if [[ "$out" == "1" && "$result" == "0" && "$mismatch_index" == "0FFFFFFFF" ]]; then
      case_status=pass
    else
      case_status=fail
    fi
  fi

  printf -v line '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s' \
    "$case_name" "$block_bytes" "$block_count" "$settle_us" "$requested_bytes" "$elapsed_s" "$case_status" \
    "$build_status" "$run_status" "$stage" "$result" "$expected_chunks_dec" "$completed_chunks_dec" \
    "$chunk_index_dec" "$rx_bytes_dec" "$mismatch_index" "$smartdma_wakes_dec" "$dma_inta_count_dec" \
    "$chunk_time_us" "$write_wait_us" "$ibi_wait_us" "$read_wait_us" "$smartdma_arm_us" \
    "$full_kib_per_s" "$read_kib_per_s" "$signature"
  write_line "$line"

  printf '  -> %s: status=%s chunk_time_us=%s full_kib_per_s=%s read_kib_per_s=%s\n' \
    "$case_name" "$case_status" "$chunk_time_us" "$full_kib_per_s" "$read_kib_per_s" >&2

  if [[ "$stop_on_fail" == "1" && "$case_status" != "pass" ]]; then
    return 1
  fi

  return 0
}

mkdir -p "$(dirname "$output_file")"
: >"$output_file"

printf -v header '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s' \
  'case_name' 'block_bytes' 'block_count' 'settle_us' 'requested_bytes' 'elapsed_s' 'case_status' \
  'build_status' 'run_status' 'stage' 'result' 'expected_chunks' 'completed_chunks' 'final_chunk_index' \
  'validated_rx_bytes' 'mismatch_index' 'smartdma_wakes' 'dma_inta_count' 'chunk_time_us' 'write_wait_us' \
  'ibi_wait_us' 'read_wait_us' 'smartdma_arm_us' 'full_kib_per_s' 'read_kib_per_s' 'signature'
write_line "$header"

read -r -a counts <<<"$counts_text"
read -r -a settles <<<"$settles_text"

for settle_us in "${settles[@]}"; do
  for block_count in "${counts[@]}"; do
    run_case "$block_count" "$settle_us" || {
      printf 'Wrote partial results to %s\n' "$output_file" >&2
      exit 1
    }
  done
done

printf 'Wrote results to %s\n' "$output_file" >&2