.DEFAULT_GOAL := help

RUNNER := ./run_experiment.sh
RUN_ENV := RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1
SUPPORTED_EXPERIMENTS := master_i3c_sdma_seed_tail_len_sweep master_i3c_dma_official_rx_probe master_i3c_dma_official_rx_smartdma_wake_probe
EXPERIMENT ?= master_i3c_sdma_seed_tail_len_sweep

.PHONY: help all clean $(SUPPORTED_EXPERIMENTS)

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make master_i3c_sdma_seed_tail_len_sweep   Build and arm the sweep for the validated TRACE32 master flow' \
	  '  make master_i3c_dma_official_rx_probe      Build and arm the official DMA RX proof for the validated TRACE32 master flow' \
	  '  make master_i3c_dma_official_rx_smartdma_wake_probe Build and arm the official DMA RX SmartDMA wake proof for the validated TRACE32 master flow' \
	  '  make clean                                 Remove _build for both supported experiments'

all: $(EXPERIMENT)

$(SUPPORTED_EXPERIMENTS):
	$(RUN_ENV) $(RUNNER) $@

clean:
	$(RUNNER) clean master_i3c_sdma_seed_tail_len_sweep
	$(RUNNER) clean master_i3c_dma_official_rx_probe
	$(RUNNER) clean master_i3c_dma_official_rx_smartdma_wake_probe