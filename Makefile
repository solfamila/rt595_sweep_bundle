.DEFAULT_GOAL := help

RUNNER := ./run_experiment.sh
RUN_ENV := RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1
SUPPORTED_EXPERIMENTS := master_i3c_sdma_seed_tail_len_sweep master_i3c_dma_official_rx_smartdma_wake_block_stream
EXPERIMENT ?= master_i3c_sdma_seed_tail_len_sweep

.PHONY: help all clean $(SUPPORTED_EXPERIMENTS)

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make master_i3c_sdma_seed_tail_len_sweep   Build and arm the sweep for the validated TRACE32 master flow' \
	  '  make master_i3c_dma_official_rx_smartdma_wake_block_stream Build and arm the passing 255-byte RX SmartDMA wake block-stream flow' \
	  '  make clean                                 Remove _build for both supported experiments'

all: $(EXPERIMENT)

$(SUPPORTED_EXPERIMENTS):
	$(RUN_ENV) $(RUNNER) $@

clean:
	$(RUNNER) clean master_i3c_sdma_seed_tail_len_sweep
	$(RUNNER) clean master_i3c_dma_official_rx_smartdma_wake_block_stream