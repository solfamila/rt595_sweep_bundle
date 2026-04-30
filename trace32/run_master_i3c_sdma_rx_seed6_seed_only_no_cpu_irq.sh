#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
source "$SCRIPT_DIR/common.sh"

bundle_root=$(trace32_bundle_root)
master_axf="$bundle_root/master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq/_build/master/evkmimxrt595_ezhb.axf"

if [[ ! -f "$master_axf" ]]; then
  echo "missing master ELF: $master_axf" >&2
  echo "Build it first with: RT595_MASTER_RUN_MODE=none ./run_experiment.sh master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq" >&2
  exit 1
fi

trace32_run_generated_script 240 240000 <<EOF
$(trace32_loader_preamble "$master_axf")

Break.Delete
Break.Set set_success_led /Program
Break.Set set_failure_led /Program

Go
WAIT !STATE.RUN() 90.s
IF STATE.RUN()
(
  Break
  WAIT !STATE.RUN() 1.s
)

PRINT "rxSeed6SeedOnly=" \
  " pc=" %HEX Register(PC) \
  " stage=" %HEX Var.VALUE(s_rx_seed_only_stage) \
  " result=" %HEX Var.VALUE(s_rx_seed_only_result) \
  " expected=" %HEX Var.VALUE(s_rx_seed_only_expected_wakes) \
  " wakes=" %HEX Var.VALUE(s_rx_seed_only_wakes) \
  " seeds=" %HEX Var.VALUE(s_rx_seed_only_dma_seed_bytes) \
  " tails=" %HEX Var.VALUE(s_rx_seed_only_smartdma_bytes) \
  " frame=" %HEX Var.VALUE(s_rx_seed_only_frame_bytes) \
  " dataIrq=" %HEX Var.VALUE(s_rx_seed_only_data_irq_delta) \
  " protocolIrq=" %HEX Var.VALUE(s_rx_seed_only_protocol_irq_delta) \
  " ibiIrq=" %HEX Var.VALUE(s_rx_seed_only_ibi_irq_delta) \
  " mstatus=" %HEX Var.VALUE(s_rx_seed_only_mstatus) \
  " merr=" %HEX Var.VALUE(s_rx_seed_only_merrwarn) \
  " mdatactrl=" %HEX Var.VALUE(s_rx_seed_only_mdatactrl) \
  " mdmactrl=" %HEX Var.VALUE(s_rx_seed_only_mdmactrl) \
  " dmaActive=" %HEX Var.VALUE(s_rx_seed_only_dma_active) \
  " dmaInta=" %HEX Var.VALUE(s_rx_seed_only_dma_inta) \
  " dmaCtl=" %HEX Var.VALUE(s_rx_seed_only_dma_ctlstat) \
  " dmaCfg=" %HEX Var.VALUE(s_rx_seed_only_dma_xfercfg) \
  " dmaErr=" %HEX Var.VALUE(s_rx_seed_only_dma_errint) \
  " rxcount=" %HEX Var.VALUE(s_rx_seed_only_rxcount) \
  " d0=" %HEX Var.VALUE(s_rx_seed_only_data0) \
  " d1=" %HEX Var.VALUE(s_rx_seed_only_data1) \
  " d2=" %HEX Var.VALUE(s_rx_seed_only_data2) \
  " d3=" %HEX Var.VALUE(s_rx_seed_only_data3) \
  " d4=" %HEX Var.VALUE(s_rx_seed_only_data4) \
  " d5=" %HEX Var.VALUE(s_rx_seed_only_data5)

ENDDO
EOF