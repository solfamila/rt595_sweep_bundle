#!/usr/bin/env bash
set -euo pipefail

trace32_common_source_path() {
  if [[ -n "${BASH_SOURCE-}" ]]; then
    printf '%s\n' "${BASH_SOURCE}"
    return 0
  fi

  if [[ -n "${ZSH_VERSION:-}" ]]; then
    eval 'printf "%s\n" "${(%):-%x}"'
    return 0
  fi

  if [[ -n "${0:-}" ]]; then
    printf '%s\n' "$0"
    return 0
  fi

  echo "unable to determine trace32 helper path" >&2
  return 1
}

trace32_bundle_root() {
  local source_path
  local script_dir

  source_path=$(trace32_common_source_path)
  script_dir=$(cd "$(dirname "$source_path")" && pwd)
  cd "$script_dir/.." && pwd
}

trace32_wrapper_path() {
  local bundle_root
  local local_wrapper
  local path_wrapper

  bundle_root=$(trace32_bundle_root)
  if [[ -n "${TRACE32_WRAPPER:-}" ]]; then
    printf '%s\n' "$TRACE32_WRAPPER"
    return 0
  fi

  local_wrapper="$bundle_root/.local/trace32/t32cmd_nostop"
  if [[ -x "$local_wrapper" ]]; then
    printf '%s\n' "$local_wrapper"
    return 0
  fi

  path_wrapper=$(command -v t32cmd_nostop 2>/dev/null || true)
  if [[ -n "$path_wrapper" ]]; then
    printf '%s\n' "$path_wrapper"
    return 0
  fi

  echo "t32cmd_nostop not found. Set TRACE32_WRAPPER or use the bundled .local/trace32/t32cmd_nostop." >&2
  return 1
}

trace32_master_axf_path() {
  local bundle_root

  bundle_root=$(trace32_bundle_root)
  printf '%s/master_i3c_sdma_seed_tail_len_sweep/_build/master/evkmimxrt595_ezhb.axf\n' "$bundle_root"
}

trace32_require_master_axf() {
  local master_axf

  master_axf=$(trace32_master_axf_path)
  if [[ ! -f "$master_axf" ]]; then
    echo "missing master ELF: $master_axf" >&2
    echo "Build it first with: RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_seed_tail_len_sweep" >&2
    return 1
  fi
}

trace32_loader_preamble() {
  local master_axf=$1

  cat <<EOF
RESet
SYStem.RESet
SYStem.CPU IMXRT595-CM33
IF COMBIPROBE()||UTRACE()
(
  SYStem.CONFIG.CONNECTOR MIPI20T
)
SYStem.Option DUALPORT ON
SYStem.MemAccess DAP
SYStem.JtagClock 10MHz
ETM.OFF
ITM.OFF
SYStem.Up

Data.LOAD.Elf "$master_axf"
EOF
}

trace32_run_generated_script() {
  local timeout_seconds=$1
  local wait_ms=$2
  local tmp_root
  local node
  local wrapper
  local script_file

  wrapper=$(trace32_wrapper_path)
  node=${TRACE32_NODE:-127.0.0.1}
  tmp_root=${TMPDIR:-/tmp}
  script_file=$(mktemp "${tmp_root%/}/rt595_sweep_trace32.XXXXXX")
  trap "rm -f '$script_file'" EXIT
  cat >"$script_file"
  "$wrapper" "node=$node" "timeout=$timeout_seconds" "wait=$wait_ms" "DO $script_file"
}