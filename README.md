# RT595 Sweep Bundle

This bundle now keeps only the two validated RT595 experiments that still matter in this repo:

- `master_i3c_sdma_seed_tail_len_sweep`
- `master_i3c_dma_official_rx_smartdma_wake_block_stream`

The obsolete RX probe, SmartDMA wake probe, and chunk-loop proof experiments were removed after the retained block-stream path was revalidated and extended beyond the old `255`-byte transport ceiling.

## Included

- `master_i3c_sdma_seed_tail_len_sweep/` with the shared sweep harness.
- `master_i3c_dma_official_rx_smartdma_wake_block_stream/` with the passing larger-block RX DMA-seed plus SmartDMA-tail flow.
- `sdk/` with the vendored RT595 master and slave build payloads.
- `src/master/drivers/fsl_i3c_smartdma.c` and `.h` with the shared SmartDMA driver used by both kept experiments.
- `.local/toolchains/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/` with the validated compiler used during the current runs.
- `.local/linkserver/` with the validated LinkServer payload used to flash the slave.
- `.local/lauterbach-mcp-venv/` with the Lauterbach TRACE32 MCP server runtime.
- `.local/rt595-trace-venv/` with the Python runtime used by the `rt595-trace` MCP server.
- `.local/rt595-trace/rt595_trace.duckdb` with the bundled RT595 trace query database.
- `.local/trace32/` with the local TRACE32 wrapper binaries and sources.
- `trace32/` with the kept sweep, block-stream, and capture helpers.

No source files outside this folder are required by the bundle build.

## Host Requirements

- Two powered EVK-MIMXRT595 boards with accessible probes.
- A Unix-like host with `bash`, `make`, `find`, `grep`, and `mktemp`.
- TRACE32 installed and licensed if you want to run the TRACE32 wrappers. The bundle includes the local wrappers, but not the TRACE32 application itself.

The bundled toolchain and LinkServer copies were validated on macOS arm64. If you move the bundle to a host that cannot run those binaries, point `RT595_TOOLCHAIN_ROOT`, `RT595_CC`, `RT595_OBJCOPY`, `RT595_GDB`, and `LINKSERVER_BIN` at host-compatible equivalents.

## Trace Debug Workflow

For the validated off-chip trace capture, `rt595-trace` export and ingest flow, MCP analysis, and the known TRACE32 / DuckDB pitfalls that were already resolved, see:

- `trace32/RT595_TRACE_DEBUG_WORKFLOW.md`

The kept capture helpers are:

- `trace32/capture_master_i3c_sdma_seed_tail_len_sweep_trace.sh`
- `trace32/capture_master_i3c_dma_official_rx_smartdma_wake_block_stream_trace.sh`

## Sweep Harness

Use the steps below for the validated shared sweep flow.

### Build and arm the slave

```bash
cd /Users/foxy/Downloads/rt595_sweep_bundle
./run_experiment.sh clean master_i3c_sdma_seed_tail_len_sweep
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_seed_tail_len_sweep
```

This builds both sides, flashes the slave if needed, leaves the slave running, and prepares the master ELF for the TRACE32-driven run.

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_sdma_seed_tail_len_sweep_long_settle.sh
```

Validated passing signature:

```text
longRun= pc=20281C1C logical=100 chunk=8 lenSweepStage=5 dmaStage=0B dmaResult=0 roundStage=5 roundResult=0 ibiCount=0 chunkValidateReason=0
```

The shared sweep also still carries the validated RX SmartDMA proof before it enters the length sweep:

```text
rxSmartdma= stage=3 result=0 validate=0 v0=0 v1=0 roundStage=5 roundResult=0 completion=0 configured=7 cb=1 tail=1 pendC=0 pendT=0 fifoBounce=0 protoBounce=1
```

That retained state means the sweep still validates the SmartDMA-driven post-IBI read tail on the shared path.

## Block-Stream Proof

`master_i3c_dma_official_rx_smartdma_wake_block_stream/` keeps the master on the official DMA request-write path, then performs the read with a DMA seed followed by a SmartDMA-drained tail.

For the legacy `<=255` path, the RX seed length remains `6` bytes. For `>255`, the retained repo now uses a five-byte request packet with a 32-bit requested wire length, two leading dummy bytes on the wire, SmartDMA consuming the payload tail, and one sacrificial trailing wire byte to compensate for the one-byte-short read behavior on the plain `I3C_MasterStart()` path.

The current observed passing ceiling validated in this repo is `15952` bytes per logical block. The first neighboring size that consistently stalls in the read phase is `15953` bytes.

### Exact validated 15952-byte repro

Build and arm the slave:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=15952 I3C_STREAM_BLOCK_COUNT=64 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream
```

Run the master with TRACE32:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=15952 I3C_STREAM_BLOCK_COUNT=64 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' ./trace32/run_master_i3c_dma_official_rx_smartdma_wake_block_stream.sh
```

Validated signature:

```text
dmaWakeBlockStreamFinal= st=0B out=1 rs=0 cs=1EE9 sa=31 ec=40 cc=40 ci=3F ip=1 i0=40 i1=0 rx=0F9400 mi=0FFFFFFFF tr=0 rf=0 rl=0FF sm=1 sw=40 si=40 ss=0 smd=12 sms=1000 sdc=1000040 cu=5A0B2A wu=8BF iu=48B3E ru=4BC465 au=1C1E3 su=49A670 xc=42 xd=0 xs=5
```

Interpretation:

1. `st=0B`, `out=1`, and `rs=0` mean the block-stream proof reached validation and reported success.
2. `ec=40`, `cc=40`, and `ci=3F` mean all `64` logical blocks completed and the final chunk index was `63`.
3. `rx=0F9400` means the validated aggregate receive length was `1,020,928` bytes, which matches `15952 x 64`.
4. `sm=1`, `sw=40`, and `si=40` show SmartDMA observed and acknowledged one DMA-completion wake per block.
5. `tr=0` confirms the legacy RX tail-recovery path was not used.
6. `cu=5A0B2A` and `ru=4BC465` correspond to about `169.0 KiB/s` end to end and about `200.8 KiB/s` read-only.

### Exact validated 512-byte repro

Build and arm the slave:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=512 I3C_STREAM_BLOCK_COUNT=4 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream
```

Run the master with TRACE32:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=512 I3C_STREAM_BLOCK_COUNT=4 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' ./trace32/run_master_i3c_dma_official_rx_smartdma_wake_block_stream.sh
```

Validated signature:

```text
dmaWakeBlockStreamFinal= st=0B out=1 rs=0 cs=1EE9 sa=31 ec=4 cc=4 ci=3 ip=1 i0=4 i1=0 rx=800 mi=0FFFFFFFF tr=0 rf=0 rl=0FF sm=1 sw=4 si=4 ss=0 smd=12 sms=1000 sdc=1000040 cu=563D wu=67 iu=2947 ru=27EE au=169 su=25F0 xc=6 xd=0 xs=3 txc=3 di=0 idc=0 i
```

Interpretation:

1. `st=0B`, `out=1`, and `rs=0` mean the block-stream proof reached validation and reported success.
2. `ec=4`, `cc=4`, and `ci=3` mean all four logical blocks completed and the last chunk index was `3`.
3. `rx=800` means the validated aggregate receive length was `2048` bytes, which matches `512 x 4`.
4. `sm=1`, `sw=4`, and `si=4` show SmartDMA observed and acknowledged one DMA-completion wake per block.
5. `tr=0` confirms the legacy RX tail-recovery path was not used.

### Exact validated 255-byte repro

Build and arm the slave:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=255 I3C_STREAM_BLOCK_COUNT=256 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream
```

Run the master with TRACE32:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=255 I3C_STREAM_BLOCK_COUNT=256 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' ./trace32/run_master_i3c_dma_official_rx_smartdma_wake_block_stream.sh
```

Validated signature from the cleaned repo:

```text
dmaWakeBlockStreamFinal= st=0B out=1 rs=0 cs=1EE9 sa=31 ec=100 cc=100 ci=0FF ip=1 i0=0 i1=0 rx=0FF00 mi=0FFFFFFFF tr=0 rf=0 rl=0FF sm=1 sw=100 si=100 ss=1000000 smd=12 sms=1000 sdc=800000C0 cu=8A175 wu=14FF iu=2EB15 ru=4FE39 au=2F03 su=49800 xc=102
```

Interpretation:

1. `st=0B`, `out=1`, and `rs=0` mean the block-stream proof reached validation and reported success.
2. `ec=100`, `cc=100`, and `ci=0FF` mean all `256` logical blocks completed and the final block index was `255`.
3. `rx=0FF00` means the validated aggregate receive length was `65280` bytes.
4. `mi=0FFFFFFFF` means no mismatch was latched.
5. `sm=1`, `sw=100`, and `si=100` show SmartDMA observed and acknowledged one DMA-completion wake per block.
6. `tr=0` confirms the legacy RX tail-recovery path was not used.

### Widths revalidated on hardware

The current cleaned repo was rechecked on hardware with `I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0`.

| Block bytes | Result | Aggregate RX bytes |
| ---: | --- | ---: |
| 32 | pass | `0x02000` |
| 64 | pass | `0x04000` |
| 128 | pass | `0x08000` |
| 255 | pass | `0x0FF00` |
| 511 | pass | `0x07FC` |
| 512 | pass | `0x0800` |
| 15952 | pass | `0x03E50` |

The `511` and `512` validations use `I3C_STREAM_BLOCK_COUNT=4` on the retained wider transport path. The current larger-block validation uses `15952 x 64 @ 0 us`. The legacy `255` regression path with `I3C_STREAM_BLOCK_COUNT=256` is still kept as the long-run stress repro.

### Benchmark and capture helpers

The kept matrix runner is:

```bash
./trace32/run_master_i3c_dma_official_rx_smartdma_wake_block_stream_matrix.sh
```

Useful overrides:

- `RT595_BLOCK_STREAM_MATRIX_BLOCK_BYTES`
- `RT595_BLOCK_STREAM_MATRIX_COUNTS`
- `RT595_BLOCK_STREAM_MATRIX_SETTLES_US`

The current post-fix stability conclusion is:

1. The zero-settle `255`-byte path still sustains about `114.4 KiB/s` end to end and about `195.0 KiB/s` read-only from `4096` through `32768` blocks.
2. The old early `st=5 rs=1EDC` failure was a slave-side post-IBI rearm visibility race, not a monotonic payload ceiling.
3. The retained block-stream implementation now also passes `511 x 4 @ 0 us`, `512 x 4 @ 0 us`, and `15952 x 64 @ 0 us` using the 32-bit request length plus dummy-seed SmartDMA-tail path.
4. The practical sustained `15952` path settles around `169.0 KiB/s` end to end and `200.8 KiB/s` read-only.
5. The first neighboring size that consistently stalls in the read phase is `15953` bytes.
6. The regression check at `255 x 256 @ 0 us` still passes after the shared slave changes and after extending the transport beyond `255`.

## Useful Overrides

- `RT595_MASTER_PROBE`
- `RT595_SLAVE_PROBE`
- `RT595_DEVICE`
- `RT595_MASTER_RUN_MODE`
- `RT595_SLAVE_LIVE_RUN`
- `RT595_EXTRA_MASTER_DEFINES`
- `RT595_EXTRA_SLAVE_DEFINES`
- `RT595_TOOLCHAIN_ROOT`
- `LINKSERVER_BIN`
- `TRACE32_WRAPPER`

## Short Version

If you only want the current short passing block-stream repro:

```bash
cd /Users/foxy/Downloads/rt595_sweep_bundle
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=15952 I3C_STREAM_BLOCK_COUNT=64 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream
RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=15952 I3C_STREAM_BLOCK_COUNT=64 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' ./trace32/run_master_i3c_dma_official_rx_smartdma_wake_block_stream.sh
```
