# RT595 Two-Experiment Bundle

This repo is intentionally trimmed to the two supported, hardware-validated
master experiments only:

- `master_i3c_sdma_seed_tail_len_sweep`
- `master_i3c_dma_official_rx_probe`

The earlier failed RX probe matrix and its helper scripts were removed so the
repo only contains the files needed to reproduce these two flows.

Build outputs live under each experiment's `_build/` directory and are
generated locally. They are intentionally not committed.

## Kept In The Repo

- `master_i3c_sdma_seed_tail_len_sweep/` with the passing SmartDMA sweep
  harness.
- `master_i3c_dma_official_rx_probe/` with the passing official master DMA RX
  proof harness.
- `sdk/` with the vendored RT595 master and slave build payloads used by the
  two supported flows.
- `src/master/drivers/fsl_i3c_smartdma.c` and `.h` with the shared SmartDMA
  path used by the sweep.
- `trace32/common.sh`
- `trace32/run_master_i3c_sdma_seed_tail_len_sweep_long_settle.sh`
- `trace32/run_master_i3c_dma_official_rx_probe.sh`
- `run_experiment.sh` and `Makefile`
- The bundled local toolchain / LinkServer / TRACE32 wrapper payloads under
  `.local/` when present in this workspace.

## Host Requirements

- Two powered EVK-MIMXRT595 boards with accessible probes.
- A Unix-like host with `bash`, `make`, `find`, `grep`, and `mktemp`.
- TRACE32 installed and licensed for the master-side run.
- LinkServer available either in `.local/` or on `PATH` for slave flash/run.

The validated flow uses Lauterbach TRACE32 on the master and LinkServer on the
slave.

## Supported Commands

The `make` targets prepare the validated TRACE32-driven master flows by
building the master, flashing the slave if needed, and starting the slave live
run:

```bash
make master_i3c_sdma_seed_tail_len_sweep
make master_i3c_dma_official_rx_probe
```

Those targets are equivalent to these direct commands:

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_seed_tail_len_sweep
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_probe
```

To remove generated build outputs for the two supported experiments:

```bash
make clean
```

## Sweep Flow

Prepare the sweep:

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_seed_tail_len_sweep
```

Run the master through TRACE32:

```bash
./trace32/run_master_i3c_sdma_seed_tail_len_sweep_long_settle.sh
```

Expected passing retained-state signature:

```text
longRun= pc=20281C1C logical=100 chunk=8 lenSweepStage=5 dmaStage=0B dmaResult=0 roundStage=5 roundResult=0 ibiCount=0 chunkValidateReason=0
```

The sweep also contains the positive shared RX SmartDMA proof that runs before
the logical-length sweep. Its retained-state signature is:

```text
rxSmartdma= stage=3 result=0 validate=0 v0=0 v1=0 roundStage=5 roundResult=0 completion=0 configured=7 cb=1 tail=1 pendC=0 pendT=0 fifoBounce=0 protoBounce=1
```

## Official DMA RX Flow

Prepare the official DMA RX proof:

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_probe
```

Run the master through TRACE32:

```bash
./trace32/run_master_i3c_dma_official_rx_probe.sh
```

Expected passing retained-state signature:

```text
dmaOfficialFinal= st=9 out=1 rs=0 cs=0 sa=31 rx=6 mi=0FFFFFFFF tr=0 rf=0 rl=5 rc=1 b0=0 b1=1 b2=2 b3=3 b4=4 b5=5
```

This proves the official RT595 master classic-DMA RX path works on hardware for
the validated 6-byte case.

## Known Limit

The repo does not claim a passing long-read official DMA RX result. The current
hardware-backed limit remains:

- The official master DMA RX proof passes for the 6-byte read above.
- Longer official-driver reads remain limited by the current ERR052123 RX-loop
  path in `fsl_i3c_dma.c`.

That limitation is documented, but the failed intermediate probe matrix that was
used to isolate it has been removed from this repo.
