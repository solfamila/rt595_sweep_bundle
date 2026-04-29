.DEFAULT_GOAL := help

RUNNER := ./run_experiment.sh
EXPERIMENT := master_i3c_sdma_seed_tail_len_sweep

.PHONY: help all clean $(EXPERIMENT)

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make master_i3c_sdma_seed_tail_len_sweep   Build, flash, run, and validate the passing segmented sweep' \
	  '  make clean                                 Remove only master_i3c_sdma_seed_tail_len_sweep/_build'

all: $(EXPERIMENT)

$(EXPERIMENT):
	$(RUNNER) $@

clean:
	$(RUNNER) clean $(EXPERIMENT)