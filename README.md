# RT595 Sweep Bundle

This folder is a portable sweep bundle for `master_i3c_sdma_seed_tail_len_sweep`.
It also includes the focused `master_i3c_rx_request_semantics_probe` used to
validate CPU, bulk DMA, and chained RX behavior on the RT595 master, plus the
follow-on `master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq` probe used to test
whether the native 6-byte RX DMA seed request fires at all.
It now also includes the official DMA RX SmartDMA wake proofs, including the
validated repeated chunk-loop path.
It vendors the passing sweep harness, the RT595 SDK payloads, the shared
SmartDMA driver used by the passing replay, the local macOS arm64 Arm GNU
toolchain copy, the local LinkServer copy, and the TRACE32 helper binaries and
wrappers that were used during validation.

## Included

- `master_i3c_sdma_seed_tail_len_sweep/` with the passing harness sources.
- `master_i3c_rx_request_semantics_probe/` with the focused RX request-semantics
  probe sources.
- `master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq/` with the narrower 6-byte
  RX seed-only discriminator sources.
- `master_i3c_dma_official_rx_smartdma_wake_chunk_loop/` with the repeated
  official 6-byte DMA RX plus SmartDMA wake proof sources.
- `sdk/` with the vendored RT595 master and slave build payloads.
- `src/master/drivers/fsl_i3c_smartdma.c` and `.h` with the shared driver fix
  that removed the `COMPLETE|RXPEND` SmartDMA IRQ spin.
- `.local/toolchains/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/`
  with the validated compiler used to rebuild the sweep.
- `.local/linkserver/` with the validated LinkServer payload used to flash the
  slave during the standalone flow.
- `.local/lauterbach-mcp-venv/` with the Lauterbach TRACE32 MCP server runtime.
- `.local/rt595-trace-venv/` with the Python dependencies used by the
  `rt595-trace` MCP server.
- `.local/rt595-trace/rt595_trace.duckdb` with the bundled RT595 trace query
  database used by the `rt595-trace` MCP server.
- `.local/trace32/` with the local TRACE32 wrapper binaries and sources.
- `trace32/` with bundle-local shell entrypoints for the long-settle,
  5-second replay, RX request-semantics probe, RX seed-only probe, and the
  official DMA RX chunk-loop proof flows.

No source files outside this folder are required by the bundle build.

## Host Requirements

- Two powered EVK-MIMXRT595 boards with accessible probes.
- A Unix-like host with `bash`, `make`, `find`, `grep`, and `mktemp`.
- TRACE32 installed and licensed if you want to run the settle or replay
  scripts. The bundle includes the local wrapper binaries, but not the TRACE32
  application itself.

The bundled toolchain and LinkServer copies were validated on macOS arm64. If
you move the bundle to a host that cannot run those binaries, point
`RT595_TOOLCHAIN_ROOT`, `RT595_CC`, `RT595_OBJCOPY`, `RT595_GDB`, and
`LINKSERVER_BIN` at host-compatible equivalents.

## VS Code MCP Servers

The bundle now includes the Lauterbach TRACE32 MCP server under
`.local/lauterbach-mcp-venv/` plus a relocatable launcher at
`.local/bin/lauterbachdebugger-mcp`.

The bundle also includes the `rt595-trace` MCP server runtime under
`.local/rt595-trace-venv/`, the bundled query database at
`.local/rt595-trace/rt595_trace.duckdb`, and a relocatable launcher at
`.local/bin/rt595-trace`.

The `rt595_trace` source code itself is not copied into the bundle. By default,
the launcher and generated `.vscode/mcp.json` point back to the hidden-gibbon
repo source at `/Users/foxy/intent/workspaces/hidden-gibbon/repo/src`.

To install the VS Code MCP config for this bundle, run:

```bash
./setup_vscode_mcp.sh
```

That writes `.vscode/mcp.json` for this folder and points it at both bundled
MCP launchers. Run it again if you move or unzip the bundle to a different
path.

What the Lauterbach launcher does:

1. Uses the bundled Lauterbach MCP Python environment when available.
2. Falls back to `python3` or `python` if needed.
3. Uses `.local/lauterbach-mcp-cache/` as the default MCP cache directory.
4. Uses `T32SYS` from the environment if you already set it.
5. Otherwise tries common TRACE32 install locations such as `~/t32/files`.

What the `rt595-trace` launcher does:

1. Uses the bundled `rt595-trace` Python environment when available.
2. Adds the hidden-gibbon `rt595_trace` source tree to `PYTHONPATH` instead of
  copying that source into the bundle.
3. Uses the bundled `.local/rt595-trace/rt595_trace.duckdb` database by default
  when started with no arguments.
4. Still accepts normal CLI arguments, so you can run `--help` or override the
  subcommand manually.

If the hidden-gibbon repo is in a different location, set
`RT595_TRACE_SOURCE_ROOT` before starting VS Code or edit `.vscode/mcp.json`
after running the setup script.

Quick local check:

```bash
./.local/bin/lauterbachdebugger-mcp --help
./.local/bin/rt595-trace --help
./.local/bin/rt595-trace serve --db ./.local/rt595-trace/rt595_trace.duckdb --help
```

If your TRACE32 installation is not under `~/t32` or `~/t32/files`, export
`T32SYS` before starting VS Code or edit `.vscode/mcp.json` after running the
setup script.

## Exact Build And Run Instructions

Use the steps below exactly for the validated passing sweep flow.

### 1. Open the bundle root

```bash
cd /Users/foxy/Downloads/rt595_sweep_bundle
```

### 2. Optional: clean the previous build

```bash
./run_experiment.sh clean master_i3c_sdma_seed_tail_len_sweep
```

This removes `master_i3c_sdma_seed_tail_len_sweep/_build/` so the next build is
fresh.

### 3. Build the sweep and prepare the boards

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_seed_tail_len_sweep
```

This does all of the following in one command:

1. Builds the master ELF at `master_i3c_sdma_seed_tail_len_sweep/_build/master/evkmimxrt595_ezhb.axf`.
2. Builds the slave ELF at `master_i3c_sdma_seed_tail_len_sweep/_build/slave/slave.axf`.
3. Flashes the slave board if the slave image changed.
4. Starts the slave and leaves it waiting for the master.
5. Does not run the master through LinkServer because `RT595_MASTER_RUN_MODE=none`.

For a good setup, the end of the output should look like this:

```text
Master ELF ready at /Users/foxy/Downloads/rt595_sweep_bundle/master_i3c_sdma_seed_tail_len_sweep/_build/master/evkmimxrt595_ezhb.axf
Slave live run started on <your slave probe id>
```

Do not use `make master_i3c_sdma_seed_tail_len_sweep` for the validated replay
flow. `make` calls `run_experiment.sh` with the runner defaults, and the runner
default is `RT595_MASTER_RUN_MODE=linkserver`, not the validated TRACE32-driven
master flow.

### 4. Run the master and reproduce the passing sweep

```bash
./trace32/run_master_i3c_sdma_seed_tail_len_sweep_long_settle.sh
```

This TRACE32 wrapper:

1. Loads `master_i3c_sdma_seed_tail_len_sweep/_build/master/evkmimxrt595_ezhb.axf`.
2. Sets breakpoints on `set_success_led` and `set_failure_led`.
3. Runs the master for up to the long-settle timeout.
4. Prints the retained sweep summary state when execution stops.

The shell wrappers under `trace32/` generate temporary TRACE32 scripts using the
bundle's current absolute path, so the bundle can be moved or unzipped without
editing `.cmm` files.

If the bundled wrapper is not usable on the target host, set
`TRACE32_WRAPPER=/path/to/t32cmd_nostop` before running the trace32 helpers.

## Taking An Off-Chip Trace Capture

The bundle also includes an example wrapper that captures an RT595 off-chip ETM
trace from the passing sweep master:

```bash
./trace32/capture_master_i3c_sdma_seed_tail_len_sweep_trace.sh
```

Use this flow when you want a real trace capture that you can reopen in
TRACE32, rather than just running to the success or failure LEDs.

### What the example does

1. Loads `master_i3c_sdma_seed_tail_len_sweep/_build/master/evkmimxrt595_ezhb.axf`.
2. Runs directly to `init_transfer_led()` so trace is armed only after
  `BOARD_InitHardware()` has finished reprogramming the clock tree.
3. Configures the RT595 EVK off-chip 4-bit trace pins and enables ETM plus ITM.
4. Uses `Trace.AutoFocus`, then forces the analyzer trace data rate to
  `198MHz`.
5. Runs the target for the requested capture window.
6. Halts the core, saves the trace as a TRACE32 `.ad` file, and also exports a
  plain-text `Trace.List` dump for quick grepping.

The explicit `198MHz` setting is intentional. In this bundle's SDK payload, the
core clock is `198000000Hz` and `CLOCK_SetClkDiv(kCLOCK_DivPfc0Clk, 2U)` is set
in `sdk/master/evkmimxrt595_ezhb/board/clock_config.c`. The example therefore
uses the same fixed data-rate setting that was validated for this RT595 setup.

### Hardware prerequisites

1. Build the master ELF first using step 3 from the validated flow above.
2. Connect the TRACE32 AutoFocus preprocessor or equivalent supported analyzer
  to the EVK off-chip trace header.
3. Start TRACE32 so `t32cmd_nostop` can connect to the active node, or point
  `TRACE32_WRAPPER` at a working wrapper binary.
4. If TRACE32 is listening on a non-default node, set `TRACE32_NODE` before
  running the helper.

### Example usage

Capture 5 seconds and write the outputs under the default capture directory:

```bash
./trace32/capture_master_i3c_sdma_seed_tail_len_sweep_trace.sh
```

Capture 12 seconds and write to a custom directory:

```bash
TRACE_CAPTURE_SECONDS=12 \
TRACE_CAPTURE_OUT_DIR="$PWD/.local/trace-captures/custom" \
./trace32/capture_master_i3c_sdma_seed_tail_len_sweep_trace.sh
```

The helper creates:

1. `<capture-name>.ad` with the raw TRACE32 trace capture.
2. `<capture-name>.txt` with a `Trace.List` export for quick search.

The default output directory is `.local/trace-captures/`.

### Reading the result

The quickest sanity checks are:

```bash
ls -lh .local/trace-captures/
rg -n "FLOW ?ERROR|HARDERROR|set_success_led|set_failure_led" .local/trace-captures/*.txt
```

Open the `.ad` file in TRACE32 when you want the full trace UI, timing view, or
trackback analysis.

## Exact Passing Signature

The validated long-settle passing output is:

```text
longRun= pc=20281C1C logical=100 chunk=8 lenSweepStage=5 dmaStage=0B dmaResult=0 roundStage=5 roundResult=0 ibiCount=0 chunkValidateReason=0
```

Interpretation:

1. `logical=100` means the final logical-length case reached `0x100`.
2. `lenSweepStage=5` means `CASE_OK`.
3. `pc=20281C1C` is the success LED function stop point.
4. `chunkValidateReason=0` means no chunk validation failure was latched.

`0x20281C1C` resolves to `set_success_led`.

## 5-Second Replay And Failure Checks

If you want the bounded replay flow that was used during debugging, use these
exact commands after step 3 above.

### Start a bounded 5-second run from reset

```bash
./trace32/run_master_i3c_sdma_seed_tail_len_sweep_5s.sh
```

This wrapper now prints both the retained replay state and the retained RX
SmartDMA probe state automatically after the 5-second stop.

### Continue the current execution for another 5 seconds

```bash
./trace32/continue_master_i3c_sdma_seed_tail_len_sweep_5s.sh
```

This wrapper also prints both retained state lines automatically after the
continue window stops.

### Print the retained replay state without running

```bash
./trace32/probe_master_i3c_sdma_seed_tail_len_sweep_replay_state.sh
```

### Print the retained RX SmartDMA proof state without running

```bash
./trace32/probe_master_i3c_sdma_seed_tail_len_sweep_rx_smartdma_state.sh
```

### Probe the retained failure snapshot

```bash
./trace32/probe_master_i3c_sdma_seed_tail_len_sweep_failure_state.sh
```

## Working RX SmartDMA Signature

The shared sweep already contains a dedicated RX-side SmartDMA proof before it
enters the logical-length sweep. That path does not depend on the native I3C RX
DMAC request line. Instead it routes `I3C0_IRQn` to SmartDMA through
`kINPUTMUX_I3c0IrqToSmartDmaInput` and validates that SmartDMA performed the
post-IBI read tail successfully.

The validated retained-state signature is:

```text
rxSmartdma= stage=3 result=0 validate=0 v0=0 v1=0 roundStage=5 roundResult=0 completion=0 configured=7 cb=1 tail=1 pendC=0 pendT=0 fifoBounce=0 protoBounce=1
```

Interpretation:

1. `stage=3` means the RX SmartDMA probe reached `RX_SMARTDMA_PROBE_STAGE_VALIDATED`.
2. `result=0` and `validate=0` mean the probe and its validator both finished successfully.
3. `configured=7` means SmartDMA was configured to drain the expected 7-byte tail after the 1-byte seed.
4. `cb=1` and `tail=1` mean both the SmartDMA completion callback and the read-tail completion path fired.
5. `pendC=0` and `pendT=0` mean no SmartDMA completion or read-tail work was left pending.
6. `protoBounce=1` is the allowed single `COMPLETE` bounce that the validator explicitly accepts.

This is the current positive RX result in the bundle: RX works through the
existing `I3c0IrqToSmartDmaInput` SmartDMA path in the shared sweep, while the
separate native RX DMAC-request probes remain negative.

## Official Master DMA RX Proof

The bundle also now includes `master_i3c_dma_official_rx_probe/`, which uses
the vendored classic DMA master API from `fsl_i3c_dma.c` and proves the master
side RX path in a focused board-to-board flow.

This proof currently runs with a 6-byte payload. The master side is the
official `I3C_MasterTransferDMA()` path. The companion slave is the shared
interrupt-based standalone slave with an experiment-specific fixed
`0..5` transmit payload after the write/IBI handshake, because the local slave
DMA path in this bundle still corrupts data and the longer 32-byte master RX
path still does not complete correctly.

### Build the official master DMA RX proof

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_probe
```

This command:

1. Builds the master ELF at `master_i3c_dma_official_rx_probe/_build/master/evkmimxrt595_ezhb.axf`.
2. Builds the companion slave ELF at `master_i3c_dma_official_rx_probe/_build/slave/slave.axf`.
3. Flashes the slave if needed.
4. Starts the slave and leaves it waiting for the TRACE32-driven master run.

### Run the official master DMA RX proof

```bash
./trace32/run_master_i3c_dma_official_rx_probe.sh
```

This TRACE32 wrapper loads the master ELF, runs to either `set_success_led` or
`set_failure_led`, and prints the retained `dmaOfficialFinal=` signature.

### Current passing signature

```text
dmaOfficialFinal= st=9 out=1 rs=0 cs=0 sa=31 rx=6 mi=FFFFFFFF tr=0 rf=0 rl=5 rc=1 b0=0 b1=1 b2=2 b3=3 b4=4 b5=5
```

Interpretation:

1. `st=9`, `out=1`, and `rs=0` mean the probe reached `kDmaOfficialStageValidated` and reported success.
2. `rx=6` means the validated payload length is 6 bytes.
3. `mi=FFFFFFFF` means no mismatch was latched.
4. `b0..b5 = 0..5` confirms the master DMA RX buffer matched the expected payload.

## Official Master DMA RX SmartDMA Wake Proof

The bundle also includes `master_i3c_dma_official_rx_smartdma_wake_probe/`,
which keeps the same passing official 6-byte master DMA RX flow but routes the
DMA0 completion event into SmartDMA and verifies that CM33 does not service
`DMA0_IRQn` or `RXREADY`/`TXNOTFULL` data IRQs during the read.

### Build the official DMA RX SmartDMA wake proof

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_probe
```

This leaves the slave running and prepares the master ELF at:

```text
/Users/foxy/Downloads/rt595_sweep_bundle/master_i3c_dma_official_rx_smartdma_wake_probe/_build/master/evkmimxrt595_ezhb.axf
```

### Run the official DMA RX SmartDMA wake proof

```bash
./trace32/run_master_i3c_dma_official_rx_smartdma_wake_probe.sh
```

This TRACE32 wrapper loads the master ELF, runs to either `set_success_led` or
`set_failure_led`, and prints the retained `dmaWakeFinal=` signature.

### Current passing signature

```text
dmaWakeFinal= st=0B out=1 rs=0 cs=0 sa=31 rx=6 mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=1 si=1 ss=0 smd=28 sms=1000 sdc=800000C0 xc=3 xd=1 xs=6 txc=2 di=0 idc=0 ipc=3 rxc=0 b0=0 b1=1 b2=2 b3=3 b4=4 b5=5
```

Interpretation:

1. `st=0B`, `out=1`, and `rs=0` mean the wake probe reached `kDmaOfficialStageValidated` and reported success.
2. `b0..b5 = 0..5` confirms the official 6-byte RX payload still validated.
3. `sm=1`, `sw=1`, and `si=1` mean SmartDMA observed exactly one DMA-completion wake and wrote the mailbox exactly once.
4. `di=0` means CM33 did not service `DMA0_IRQn` during the read.
5. `idc=0` means CM33 did not service `RXREADY` or `TXNOTFULL` data IRQs during the read, while `ipc=3` shows the remaining CM33 I3C activity stayed on protocol IRQs only.
6. `rxc=0` means the CM33 RX DMA callback did not run on the validated path.

## Official Master DMA RX SmartDMA Wake Chunk-Loop Proof

The bundle also includes `master_i3c_dma_official_rx_smartdma_wake_chunk_loop/`,
which repeats the same official 6-byte DMA RX plus SmartDMA wake handoff across
multiple write/IBI/read chunks while keeping CM33 off `DMA0_IRQn`,
`RXREADY`/`TXNOTFULL`, and the RX DMA callback path.

This loop now uses four stabilizers that were required on real hardware:

1. The master scrubs protocol and error flags plus flushes the FIFOs at each chunk boundary.
2. The slave rearms only after a real post-IBI echo completion.
3. The slave clears stale `kI3C_SlaveEventSentFlag` and slave error status before each repeated `I3C_SlaveRequestIBIWithData()` retry.
4. The slave drops stale completion rearm before a newly received generation queues its next IBI, and it only runs completion rearm while no next-generation IBI or post-echo work is pending.

The chunk-loop also uses the mandatory one-byte IBI payload as a generation tag,
so the retained `i0=` field proves that each accepted IBI is fresh rather than a
stale replay.

The same experiment can now also be built against a compile-time
`I3C_LOGICAL_TOTAL_BYTES` target. That keeps the validated official `6`-byte DMA
RX chunk size for full chunks while allowing the final chunk to be shorter. The
slave still returns the fixed `0..n-1` byte pattern, but it now applies a
compile-time final-chunk override derived from the requested total-byte target
instead of changing the proven full-chunk path.

The experiment defaults to a conservative `10 ms` inter-chunk settle because the
shorter gaps that were adequate for `2 x 6` were not stable over longer loops.
After the stale-completion-rearm and post-IBI queued-complete guard fixes
below, `42 x 6` is now validated at `1200 us`. `1000 us`, `100 us`, `10 us`,
and `0 us` still fail on the second chunk with the same `st=6 / rs=-2`
next-IBI timeout boundary.

### Build the chunk-loop proof

The default build validates the `2 x 6` case:

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_chunk_loop
```

### Validated scaled repro command

The camera-scale proof that was validated on hardware uses `42 x 6` chunks:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_LOGICAL_CHUNK_COUNT=42' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_chunk_loop
```

### Validated 1200 us scaled repro command

The shortest settle that is currently validated on hardware for `42 x 6`
rebuilds the same loop with a `1200 us` inter-chunk gap:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_LOGICAL_CHUNK_COUNT=42 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=1200' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_chunk_loop
```

### Validated remainder repro command

The narrow remainder proof that was validated on hardware uses `7` total bytes,
which forces a single-byte final chunk:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_LOGICAL_TOTAL_BYTES=7' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_chunk_loop
```

This leaves the master ELF at:

```text
/Users/foxy/Downloads/rt595_sweep_bundle/master_i3c_dma_official_rx_smartdma_wake_chunk_loop/_build/master/evkmimxrt595_ezhb.axf
```

### Run the chunk-loop proof with TRACE32

```bash
./trace32/run_master_i3c_dma_official_rx_smartdma_wake_chunk_loop.sh
```

This wrapper prints the retained `dmaWakeLoopFinal=` signature after it stops at
either `set_success_led` or `set_failure_led`.

For larger scale points, override the built-in wait with environment variables
instead of editing the script. For example:

```bash
RT595_TRACE32_RUN_WAIT_SECONDS=1800 RT595_TRACE32_TIMEOUT_SECONDS=1860 RT595_TRACE32_WAIT_MS=1860000 ./trace32/run_master_i3c_dma_official_rx_smartdma_wake_chunk_loop.sh
```

### Capture the 1200 us chunk-loop run with TRACE32

```bash
./trace32/capture_master_i3c_dma_official_rx_smartdma_wake_chunk_loop_trace.sh
```

This capture helper now defaults to a `60 s` run wait and a `90 s` wrapper
timeout because the validated `42 x 6 @ 1200 us` run outlives the previous
`5 s` capture default. Override those defaults with
`TRACE_CAPTURE_RUN_WAIT_SECONDS`, `TRACE_CAPTURE_TIMEOUT_SECONDS`, and
`TRACE_CAPTURE_WAIT_MS` if you want a shorter or longer stop window.

### Validated 1200 us capture signature

```text
dmaWakeLoopFinal= st=0B out=1 rs=0 ec=2A cc=2A ci=29 ip=1 i0=2A rx=0FC sw=2A si=2A di=0 idc=0 ipc=7E rxc=0
```

Interpretation:

1. `st=0B`, `out=1`, and `rs=0` mean the repeated wake proof still reached `kDmaOfficialStageValidated` and reported success at the shorter settle.
2. `ec=2A`, `cc=2A`, and `ci=29` mean all 42 logical chunks completed and the final chunk index was 41.
3. `ip=1` and `i0=2A` show the final accepted IBI still carried the chunk-42 generation tag at `1200 us`.
4. `rx=0FC`, `sw=2A`, and `si=2A` mean the aggregate 252-byte RX completed while SmartDMA still observed one wake and one DMA INTA acknowledgement per chunk.
5. `di=0`, `idc=0`, and `rxc=0` mean CM33 still serviced no `DMA0_IRQn`, no data IRQs, and no RX DMA callback across the full sub-`10 ms` loop.

### Run the chunk-count, settle, and remainder ladders

```bash
./trace32/run_master_i3c_dma_official_rx_smartdma_wake_chunk_loop_matrix.sh
```

By default this writes a TSV record to `.local/chunk_loop_matrix_results.tsv`
while running:

1. a settle sweep for the validated `42 x 6` chunk loop at `10000`, `1000`, `100`, `10`, and `0` microseconds.
2. a remainder ladder for `253`, `257`, `511`, and `1025` total bytes at the stable `10000` microsecond settle.

You can override those ladders directly from the shell. For example, the larger
camera-scale chunk ladder discussed in this repo can be run with:

```bash
RT595_CHUNK_LOOP_MATRIX_CHUNK_COUNTS='42 170 682 2731 10923' RT595_CHUNK_LOOP_MATRIX_SETTLES_US='10000 1000 100 10 0' RT595_CHUNK_LOOP_MATRIX_MODE=chunks ./trace32/run_master_i3c_dma_official_rx_smartdma_wake_chunk_loop_matrix.sh
```

### Current validated 42 x 6 signature

```text
dmaWakeLoopFinal= st=0B out=1 rs=0 cs=1EE9 sa=31 ec=2A cc=2A ci=29 ip=1 i0=2A i1=0 rx=0FC mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=2A si=2A ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=55 xd=1 xs=6 txc=2 di=0 idc=0 ipc=7E rxc=0
```

Interpretation:

1. `st=0B`, `out=1`, and `rs=0` mean the repeated wake proof reached `kDmaOfficialStageValidated` and reported success.
2. `ec=2A`, `cc=2A`, and `ci=29` mean all 42 logical chunks completed and the final chunk index was 41.
3. `ip=1` and `i0=2A` show the final accepted IBI carried the one-byte generation tag for chunk 42, proving the loop was still seeing fresh IBIs at the end of the run.
4. `rx=0FC` means the validated aggregate receive length was 252 bytes.
5. `sm=1`, `sw=2A`, and `si=2A` mean SmartDMA observed and acknowledged exactly one DMA-completion wake for each chunk.
6. `di=0`, `idc=0`, and `rxc=0` mean CM33 still serviced no `DMA0_IRQn`, no data IRQs, and no RX DMA callback across the full repeated loop.
7. `mi=0FFFFFFFF`, `rf=0`, and `rl=5` mean no mismatch was latched and the validated payload still began at `0` and ended at `5`.

### Long-loop root cause and new scale points

The earlier intermittent long-loop failures beyond `42` chunks turned out not to
be a hard chunk-count ceiling. The retained slave state showed that generation
`N+1` could queue its IBI while the previous generation's completion-triggered
rearm request was still live. That stale rearm then cleared the new
`g_slaveIbiPending` state before `I3C_SlaveRequestIBIWithData()` issued.

The fix in `sdk/slave/i3c_interrupt_b2b_transfer_slave_base.c` was:

1. clear stale `g_slaveCompletionRearmPending` as soon as a new RX generation queues its post-IBI work.
2. refuse to run `i3c_slave_rearm_after_completion()` while next-generation IBI or post-echo work is already pending.
3. keep a short bounded post-IBI queued-complete guard before the next chunk starts and refuse to reissue while `g_slaveIbiIssued` is still latched.

After that fix, the repeated official DMA RX chunk path scaled substantially at
the same `10 ms` settle:

1. `170 x 6`: `dmaWakeLoopFinal= st=0B out=1 rs=0 cs=0 sa=31 ec=0AA cc=0AA ci=0A9 ip=1 i0=0AA i1=0 rx=3FC mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=0AA si=0AA ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=155 xd=1 xs=6 txc=2 di=0 idc=0 ipc=1FE rxc=0`
2. `341 x 6`: `dmaWakeLoopFinal= st=0B out=1 rs=0 cs=0 sa=31 ec=155 cc=155 ci=154 ip=1 i0=55 i1=0 rx=7FE mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=155 si=155 ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=2AB xd=1 xs=6 txc=2 di=0 idc=0 ipc=3FF rxc=0`

The same slave-side guard also lowered the `42 x 6` settle floor from `10 ms`
to `1200 us` while leaving the `1000 us` and below failures reproducible.
3. `682 x 6`: `dmaWakeLoopFinal= st=0B out=1 rs=0 cs=1EE9 sa=31 ec=2AA cc=2AA ci=2A9 ip=1 i0=0AA i1=0 rx=0FFC mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=2AA si=2AA ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=555 xd=1 xs=6 txc=2 di=0 idc=0 ipc=7FE rxc=0`
4. `2731 x 6`: `dmaWakeLoopFinal= st=0B out=1 rs=0 cs=0 sa=31 ec=0AAB cc=0AAB ci=0AAA ip=1 i0=0AB i1=0 rx=4002 mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=0AAB si=0AAB ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=1557 xd=1 xs=6 txc=2 di=0 idc=0 ipc=2001 rxc=0`
5. `10923 x 6`: `dmaWakeLoopFinal= st=0B out=1 rs=0 cs=0 sa=31 ec=2AAB cc=2AAB ci=2AAA ip=1 i0=0AB i1=0 rx=10002 mi=0FFFFFFFF tr=0 rf=0 rl=5 sm=1 sw=2AAB si=2AAB ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=5557 xd=1 xs=6 txc=2 di=0 idc=0 ipc=8001 rxc=0`

These all preserved the same zero-CM33-payload-IRQ property: `di=0`, `idc=0`,
and `rxc=0` throughout the validated runs.

For the larger scale points, the one-byte `i0=` generation tag wraps modulo 256.
At that point the authoritative completion fields are `ec`, `cc`, `ci`, and
`rx`, not the raw final `i0=` byte by itself.

### Validated `7`-byte remainder signature

```text
dmaWakeLoopFinal= st=0B out=1 rs=0 cs=1EE9 sa=31 ec=2 cc=2 ci=1 ip=1 i0=2 i1=0 rx=7 mi=0FFFFFFFF tr=0 rf=0 rl=0 sm=1 sw=2 si=2 ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=5 xd=1 xs=1 txc=2 di=0 idc=0 ipc=6 rxc=0 b0=0 b1=1 b2=2 b3=3 b4=4 b5=5 b6=0
```

Interpretation:

1. `ec=2`, `cc=2`, and `ci=1` mean the loop completed a full `6`-byte chunk plus a one-byte remainder chunk.
2. `rx=7` proves the aggregate validated receive length followed the requested total-byte target rather than rounding up to `12`.
3. `xs=1` shows the final slave-side data phase was a one-byte transfer.
4. `i0=2`, `sw=2`, and `si=2` show the second IBI generation and SmartDMA wake were still clean on the remainder boundary.
5. `di=0`, `idc=0`, and `rxc=0` mean the remainder path preserved the same zero-CM33-payload-IRQ property as the full-chunk proof.

### Scaled remainder proof at `1025` bytes

The same repaired path also passes a larger non-multiple-of-6 transfer at the
stable `10 ms` settle:

```bash
RT595_EXTRA_MASTER_DEFINES='I3C_LOGICAL_TOTAL_BYTES=1025 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=10000' RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_chunk_loop
```

Validated signature:

```text
dmaWakeLoopFinal= st=0B out=1 rs=0 cs=0 sa=31 ec=0AB cc=0AB ci=0AA ip=1 i0=0AB i1=0 rx=401 mi=0FFFFFFFF tr=1 rf=0 rl=4 sm=1 sw=0AB si=0AB ss=3000000 smd=28 sms=1000 sdc=800000C0 xc=157 xd=1 xs=5 txc=2 di=0 idc=0 ipc=201 rxc=0
```

Interpretation:

1. `ec=0AB`, `cc=0AB`, and `ci=0AA` mean the loop completed `171` chunks total.
2. `rx=401` proves the aggregate validated receive length was `1025` bytes.
3. `xs=5` and `rl=4` show the final chunk was a `5`-byte remainder carrying the expected `0..4` payload.
4. `sw=0AB` and `si=0AB` show SmartDMA still observed one DMA-completion wake per chunk across the full scaled remainder run.
5. `di=0`, `idc=0`, and `rxc=0` mean the scaled remainder path also preserved zero CM33 DMA IRQ, zero CM33 data IRQ, and zero RX DMA callback involvement.

## RX Request-Semantics Probe

Use this flow to reproduce the verified RT595 RX result where all three cases
pass on hardware:

1. CPU write plus plain CPU read of 4 bytes.
2. CPU write plus bulk RX DMA read of 4 bytes.
3. CPU write plus chained one-byte RX DMA descriptors.

### Build the RX probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_request_semantics_probe
```

This leaves the slave running and prepares the master ELF at:

```text
/Users/foxy/Downloads/rt595_sweep_bundle/master_i3c_rx_request_semantics_probe/_build/master/evkmimxrt595_ezhb.axf
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_request_semantics_probe.sh
```

The validated passing signature is:

```text
rxProbe= pc=20281800 mode=3 stage=0A result=0 dmaInta=1 dataIrqDelta=0 protocolIrqDelta=0 ibiIrqDelta=0 status=1000 err=0 mdatactrl=80000030 d0=1 d1=2 d2=3 d3=4
```

Interpretation:

1. `mode=3` means the probe reached the chained RX DMA case.
2. `stage=0A` means the full probe completed successfully.
3. `result=0` means no error was latched.
4. `dataIrqDelta=0`, `protocolIrqDelta=0`, and `ibiIrqDelta=0` show the
  passing path did not depend on CM33 IRQ cleanup.
5. `d0..d3 = 1,2,3,4` confirms the returned payload matches the transmitted
  data.

This probe is still useful, but it does not prove the native RX DMA request
line is viable for SmartDMA wakeup. The DMA phases in this probe wait for I3C
read `COMPLETE` and then software-trigger DMA, so a passing result here is
evidence for the post-`COMPLETE` software-trigger path, not for the native RX
request source.

## RX Seed6 Seed-Only Probe

Use this narrower flow to reproduce the current RT595 RX boundary where the
controller reaches a 6-byte FIFO level with RX DMA enabled, but the first
native DMA seed wake still never arrives.

Geometry under test:

1. Read length fixed at 6 bytes.
2. RX trigger level set to 3/4 full.
3. DMA configured to move exactly one seed byte.
4. SmartDMA booted only to observe the DMA0 IRQ and write the mailbox.
5. CM33 data IRQ handling disabled for the read.

### Build the seed-only probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq
```

This leaves the slave running and prepares the master ELF at:

```text
/Users/foxy/Downloads/rt595_sweep_bundle/master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq/_build/master/evkmimxrt595_ezhb.axf
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq.sh
```

The validated retained-state signature is:

```text
rxSeed6SeedOnly= pc=202883D8 stage=202 result=5 expected=1 wakes=0 seeds=0 tails=0 frame=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mstatus=1E03 merr=0 mdatactrl=60000F0 mdmactrl=12 dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0 rxcount=6 d0=0 d1=0 d2=0 d3=0 d4=0 d5=0
```

Interpretation:

1. `stage=202` is the mailbox wait timeout checkpoint.
2. `result=5` is `kStatus_Timeout`.
3. `rxcount=6` means the RX FIFO reached the expected 6-byte level.
4. `mdmactrl=12` means RX DMA remained enabled.
5. `dmaActive=1000000` with `dmaInta=0` means DMA0 channel 24 stayed armed but never received the native RX trigger.
6. `dataIrq=0` and `protocolIrq=0` mean CM33 did not service the payload path.

This is the narrower follow-up to the earlier `seed1 + tail5` negative probe.
Removing the SmartDMA tail reads does not restore the first RX DMA seed wake,
so the blocker is the native RX DMA request source itself rather than the tail
logic.

## RX Seed-Only Geometry Matrix Probe

Use this follow-on probe to vary only the seed-only geometry while keeping the
same native RX request plus SmartDMA-wake observation path.

Cases under test:

1. Length 4 with `RXTRIG=OnNotEmpty`.
2. Length 4 with `RXTRIG=OneHalfOrMore`.
3. Length 6 with `RXTRIG=OneHalfOrMore`.
4. Length 6 with `RXTRIG=ThreeQuarterOrMore`.

In every case:

1. DMA0 CH24 is configured for a single 1-byte seed transfer from `MRDATAB`.
2. SmartDMA is used only to observe the DMA0 IRQ and complete a mailbox.
3. SmartDMA does not read any RX tail bytes.
4. CM33 data IRQ handling remains disabled.

### Build the matrix probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_rx_seed_only_matrix_no_cpu_irq
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_sdma_rx_seed_only_matrix_no_cpu_irq.sh
```

The validated retained-state signature is:

```text
rxSeedMatrix= pc=20288AB4 case=0 len=4 trig=0 stage=202 result=5 act=1000000 inta=0 rx=4 c0=4/0/5/0/0/4 c1=0/0/0/0/0/0 c2=0/0/0/0/0/0 c3=0/0/0/0/0/0
```

Interpretation:

1. The probe fails immediately on case 0, before reaching the 6-byte cases.
2. `len=4 trig=0` means the failure already occurs at 4 bytes with `RXTRIG=OnNotEmpty`.
3. `stage=202 result=5` is the mailbox wait timeout.
4. `act=1000000 inta=0 rx=4` means DMA0 channel 24 stayed active, never raised INTA, and the RX FIFO still reached 4 bytes.
5. `c0=4/0/5/0/0/4` encodes `len/rxtrig/result/wakes/dmaIntaCount/rxcount` for case 0.

This falsifies the earlier hypothesis that the seed-only failure is specific to
the 6-byte or 3/4-full geometry.

## RX Seed4 Direct DMA INTA Poll Probe

Use this probe to remove SmartDMA from the observation path entirely and test
whether the native 4-byte RX seed request ever raises DMA INTA when CM33 polls
the DMA channel directly.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 uses the same 1-byte seed descriptor.
4. SmartDMA wake routing is not used.
5. CM33 polls DMA INTA directly and still does not service RXREADY/TXREADY data IRQs.

### Build the direct-DMA probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma_inta_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_seed_only_dma_inta_poll.sh
```

The validated retained-state signature is:

```text
rxSeed4DmaInta= pc=2028853C stage=202 result=5 dmaIntaCount=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mstatus=1E03 merr=0 mdatactrl=4000030 mdmactrl=12 dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0 rxcount=4 d0=0 d1=0 d2=0 d3=0
```

Interpretation:

1. `stage=202 result=5` is the DMA-INTA wait timeout.
2. `dmaIntaCount=0` and `dmaInta=0` mean CM33 never observed a DMA completion interrupt from channel 24.
3. `rxcount=4` means the RX FIFO still reached the expected 4-byte level.
4. `dmaActive=1000000` with `mdmactrl=12` means RX DMA remained enabled and the channel stayed armed.
5. `dataIrq=0` and `protocolIrq=0` mean CM33 still did not service the payload path.

This is the stronger discriminator. Even with SmartDMA removed entirely, the
native RX DMA request still does not assert DMA0 CH24 INTA for the 4-byte,
`RXTRIG=OnNotEmpty`, one-byte seed-descriptor path.

## RX Seed4 Full-Length DMA INTA Poll Probe

Use this follow-on probe to keep the same native 4-byte RX request path but
replace the one-byte seed descriptor with a single full-length 4-byte native
DMA descriptor.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 uses one native descriptor with `XFERCOUNT=4`.
4. SmartDMA wake routing is not used.
5. CM33 polls DMA INTA directly and still does not service RXREADY/TXREADY data IRQs.

### Build the full-length direct-DMA probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_full_len_dma_inta_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_full_len_dma_inta_poll.sh
```

The validated retained-state signature is:

```text
rxFull4= pc=2028863C st=202 rs=5 ic=0 di=0 pi=0 ii=0 ms=1E03 me=0 md=4000030 mc=12 act=1000000 inta=0 cfg=1 ctl=1 xcfg=34011 xcnt=3 err=0 rx=4 ds=400360C0 dd=2028CF6F dc=34011 dn=4 d0=0 d1=0 d2=0 d3=0
```

Interpretation:

1. `st=202 rs=5` is the DMA-INTA wait timeout.
2. `ic=0` and `inta=0` mean DMA0 channel 24 never completed the full-length descriptor.
3. `rx=4` means the RX FIFO still reached the expected 4-byte level.
4. `xcfg=34011`, `xcnt=3`, and `dn=4` confirm the armed descriptor was the intended 4-byte transfer.
5. `act=1000000` with `mc=12` means RX DMA remained enabled and the channel stayed armed.

This rules out the earlier idea that the native RX failure might be specific to
the 1-byte seed-descriptor form. The native 4-byte full-length descriptor also
fails to complete.

## RX Len6 Full-Length DMA INTA Poll Probe

Use this follow-on probe to mirror the RX-DMA errata geometry as literally as
possible while still keeping SmartDMA out of the observation path.

Geometry under test:

1. Read length fixed at 6 bytes.
2. `RXTRIG=ThreeQuarterOrMore`.
3. DMA0 CH24 uses one native descriptor with `XFERCOUNT=6`.
4. DMA width stays byte-sized.
5. SmartDMA wake routing is not used.
6. CM33 polls DMA INTA directly and still does not service RXREADY/TXREADY data IRQs.

### Build the len6 full-length probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_len6_full_len_dma_inta_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_len6_full_len_dma_inta_poll.sh
```

The validated retained-state signature is:

```text
rxFull6= pc=20288694 st=202 rs=5 ic=0 di=0 pi=0 ii=0 ms=1E03 me=0 md=60000F0 mc=12 act=1000000 inta=0 cfg=1 ctl=1 xcfg=54011 xcnt=5 err=0 rx=6 ds=400360C0 dd=2028CF71 dc=54011 dn=6 d0=0 d1=0 d2=0 d3=0 d4=0 d5=0
```

Interpretation:

1. `st=202 rs=5` is the DMA-INTA wait timeout.
2. `rx=6` means the RX FIFO still reached the expected 6-byte errata-safe level.
3. `xcfg=54011`, `xcnt=5`, and `dn=6` confirm the armed descriptor was the intended 6-byte full-length transfer.
4. `act=1000000` with `mc=12` means RX DMA remained enabled and the channel stayed armed.
5. `ic=0` and `inta=0` mean DMA0 channel 24 still never completed even in the literal 6-byte errata geometry.

This falsifies the narrower hypothesis that native RX DMA might only fail for
1-byte seed descriptors and recover when moved to the documented 6-byte
workaround geometry. On this board and SDK setup, the native RX DMAC request
still does not produce DMA completion even for a byte-width, full-length,
`RXTRIG=3/4`, 6-byte descriptor.

## RX Seed4 Seed-Only DMA CFG-Compare Probe

Use this sibling probe to keep the failing native 1-byte seed descriptor, but
copy only the DMA channel CFG bits from the older software-trigger-success
probe.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 still uses the same 1-byte seed descriptor.
4. SmartDMA wake routing is not used.
5. DMA channel CFG is changed to `PERIPHREQEN|TRIGBURST|BURSTPOWER(2)`.
6. CM33 polls DMA INTA directly and still does not service RXREADY/TXREADY data IRQs.

### Build the CFG-compare probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma_cfg_compare
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_seed_only_dma_cfg_compare.sh
```

The validated retained-state signature is:

```text
rxCfgCmp= pc=2028862C st=202 rs=5 ic=0 di=0 pi=0 ii=0 ms=1E03 me=0 md=4000030 mc=12 act=1000000 inta=0 cfg=241 ctl=1 xcfg=4011 xcnt=0 err=0 rx=4 ds=400360C0 dd=2028CF6C dc=4011 dn=1 d0=0 d1=0 d2=0 d3=0
```

Interpretation:

1. `st=202 rs=5` is the DMA-INTA wait timeout.
2. `cfg=241` confirms the DMA channel CFG matched the older software-trigger-success path.
3. `ic=0` and `inta=0` mean the native RX request still never completed the 1-byte descriptor.
4. `rx=4` means the RX FIFO still reached the expected 4-byte level.
5. `dc=4011` and `dn=1` confirm the descriptor stayed in the one-byte seed form while only CFG changed.

This rules out the simpler channel-CFG explanation. Copying the earlier
software-trigger path CFG does not restore native RX DMA completion.

## RX Seed4 DMA1 INTA Poll Probe

Use this routing-side probe to keep the same native 4-byte, one-byte seed
descriptor path but switch the request route from DMA0 CH24 to DMA1 CH24.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA1 CH24 uses the same 1-byte seed descriptor from `MRDATAB`.
4. INPUTMUX enables `kINPUTMUX_I3c0RxToDmac1Ch24RequestEna`.
5. SmartDMA wake routing is not used.
6. CM33 polls DMA1 INTA directly and still does not service RXREADY/TXREADY data IRQs.

### Build the DMA1 routing probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma1_inta_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_seed_only_dma1_inta_poll.sh
```

The validated retained-state signature is:

```text
rxSeed4Dma1Inta= pc=2028853C stage=202 result=5 dmaIntaCount=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mstatus=1E03 merr=0 mdatactrl=4000030 mdmactrl=12 dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0 rxcount=4 d0=0 d1=0 d2=0 d3=0
```

Interpretation:

1. `stage=202 result=5` is the DMA-INTA wait timeout.
2. `dmaIntaCount=0` and `dmaInta=0` mean DMA1 CH24 never completed the 1-byte descriptor.
3. `rxcount=4` means the RX FIFO still reached the expected 4-byte level.
4. `mdmactrl=12` shows the I3C-side RX DMA enable mode was unchanged from the DMA0 direct probe.
5. The retained signature matches the DMA0 direct poll result apart from the selected DMA controller.

This falsifies the simple controller-route hypothesis. The native I3C0 RX
request still does not produce a DMA completion when the path is moved from
DMA0 CH24 to DMA1 CH24.

## RX Seed4 DMA One-Frame Poll Probe

Use this I3C-side handshake probe to keep the same DMA0 direct seed-only path
but change only `MDMACTRL.DMAFB` from `ENABLE` to `ENABLE_ONE_FRAME`.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 still uses the same 1-byte seed descriptor.
4. SmartDMA wake routing is not used.
5. CM33 polls DMA0 INTA directly and still does not service RXREADY/TXREADY data IRQs.
6. The probe forces `MDMACTRL = DMAFB(1) | DMAWIDTH(byte)` after enabling RX DMA.

### Build the one-frame probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma_one_frame_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_seed_only_dma_one_frame_poll.sh
```

The validated retained-state signature is:

```text
rxSeed4OneFrame= pc=2028854C stage=202 result=5 dmaIntaCount=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mstatus=1E03 merr=0 mdatactrl=4000030 mdmactrl=10 dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0 rxcount=4 d0=0 d1=0 d2=0 d3=0
```

Interpretation:

1. `stage=202 result=5` is the DMA-INTA wait timeout.
2. `dmaIntaCount=0` and `dmaInta=0` mean the one-frame mode still does not produce a DMA completion.
3. `rxcount=4` means the RX FIFO still reached the expected 4-byte level.
4. `mdmactrl=10` shows the probe no longer retained the earlier `DMAFB(2)` value at timeout, but the transfer still did not complete.
5. `dmaActive=1000000` means DMA0 CH24 stayed armed even though no completion arrived.

This rules out the simpler `MDMACTRL.DMAFB=ENABLE` vs `ENABLE_ONE_FRAME`
explanation. Changing the I3C-side RX DMA mode alone does not restore native
RX DMA completion.

## RX Seed4 DMA RXREADY-Enabled Poll Probe

Use this handshake-side probe to keep the same direct DMA0 seed-only path but
leave `kI3C_MasterRxReadyFlag` enabled in `MINTSET` while NVIC for `I3C0_IRQn`
 remains disabled, so CM33 still does not service the data IRQ.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 still uses the same 1-byte seed descriptor.
4. SmartDMA wake routing is not used.
5. `MINTSET` is left with `kI3C_MasterRxReadyFlag` enabled.
6. NVIC for `I3C0_IRQn` remains disabled, so CM33 still cannot service RXREADY/TXREADY data IRQs.

### Build the RXREADY-enabled probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma_rxready_enabled_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_seed_only_dma_rxready_enabled_poll.sh
```

The validated retained-state signature is:

```text
rxSeed4RxReady= pc=202885F4 stage=202 result=5 dmaIntaCount=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mstatus=1E03 merr=0 mdatactrl=4000030 mdmactrl=12 mintset=800 mintmasked=800 dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0 rxcount=4 d0=0 d1=0 d2=0 d3=0
```

Interpretation:

1. `stage=202 result=5` is still the DMA-INTA wait timeout.
2. `mintset=800` and `mintmasked=800` show that the controller did latch `RXREADY` with the interrupt source enabled.
3. `dataIrq=0` confirms CM33 still did not service the data IRQ path because NVIC remained disabled.
4. `rxcount=4` means the RX FIFO still reached the expected 4-byte level.
5. `dmaIntaCount=0` and `dmaInta=0` mean DMA0 CH24 still never completed the one-byte descriptor.

This rules out the simpler `RXREADY` interrupt-enable gating explanation. Even
with the `RXREADY` source enabled and pending in `MINTMASKED`, the native RX
DMA path still does not complete.

## RX Seed4 DMA INPUTMUX-Live Poll Probe

Use this mux-side probe to keep the same direct DMA0 seed-only path but leave
the INPUTMUX clock enabled for the whole transfer instead of calling
`INPUTMUX_Deinit()` immediately after setting the request-enable bit.

Geometry under test:

1. Read length fixed at 4 bytes.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 still uses the same 1-byte seed descriptor.
4. SmartDMA wake routing is not used.
5. `INPUTMUX->DMAC0_REQ_ENA0` stays live while the transfer runs.
6. The probe snapshots `INPUTMUX->DMAC0_REQ_ENA0` in retained state.

### Build the INPUTMUX-live probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_seed4_seed_only_dma_inputmux_live_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_seed4_seed_only_dma_inputmux_live_poll.sh
```

The validated retained-state signature is:

```text
rxSeed4InputmuxLive= pc=20288604 stage=202 result=5 dmaIntaCount=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mstatus=1E03 merr=0 mdatactrl=4000030 mdmactrl=12 mintset=0 mintmasked=0 reqena0=0FDFFFFFF dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0
```

Interpretation:

1. `stage=202 result=5` is still the DMA-INTA wait timeout.
2. `reqena0=0FDFFFFFF` shows the DMAC0 request-enable register stayed programmed with the `I3C0_RX` enable bit set and the `I3C0_TX` bit cleared during the failure snapshot.
3. `dmaIntaCount=0` and `dmaInta=0` mean DMA0 CH24 still never completed the one-byte descriptor.
4. `mintset=0 mintmasked=0` confirms this variant did not rely on the RXREADY interrupt-enable path.

This rules out the simpler INPUTMUX clock-lifetime explanation. Native RX DMA
still does not complete even when the INPUTMUX request-enable register remains
live for the whole transfer.

## RX Len1 Seed-Only DMA INTA Poll Probe

Use this read-terminate probe to keep the same direct DMA0 seed-only path but
reduce the read length to 1 byte so `MCTRL.RDTERM` is also 1.

Geometry under test:

1. Read length fixed at 1 byte.
2. `RXTRIG=OnNotEmpty`.
3. DMA0 CH24 still uses the same 1-byte seed descriptor.
4. SmartDMA wake routing is not used.
5. `INPUTMUX->DMAC0_REQ_ENA0.I3C0_RX` remains enabled for the transfer.
6. The probe snapshots `MCTRL` so the retained state proves `RDTERM=1` at failure.

### Build the len1 probe and arm the slave

```bash
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_rx_len1_seed_only_dma_inta_poll
```

### Run the master with TRACE32

```bash
./trace32/run_master_i3c_rx_len1_seed_only_dma_inta_poll.sh
```

The validated retained-state signature is:

```text
rxLen1= pc=2028863C stage=202 result=5 dmaIntaCount=0 dataIrq=0 protocolIrq=0 ibiIrq=0 mctrl=16100 mstatus=1E03 merr=0 mdatactrl=1000030 mdmactrl=12 mintset=0 mintmasked=0 reqena0=0FDFFFFFF dmaActive=1000000 dmaInta=0 dmaCtl=1 dmaCfg=4011 dmaErr=0
```

Interpretation:

1. `stage=202 result=5` is still the DMA-INTA wait timeout.
2. `mctrl=16100` means the halted failure snapshot still carried `RDTERM=1`.
3. `mstatus=1E03` shows the read command had already reached `MCTRLDONE|COMPLETE|RXPEND|TXNOTFULL` in `NORMACT` state.
4. `mdatactrl=1000030` means one byte was pending in the RX FIFO.
5. `dmaIntaCount=0` and `dmaInta=0` mean DMA0 CH24 still never completed even for the one-byte read.

This rules out the simpler read-length / `RDTERM`-size explanation. Native RX
DMA still does not complete even when the transfer is reduced to a 1-byte read
that should be the easiest possible seed case.

### Very long settle variant

```bash
./trace32/run_master_i3c_sdma_seed_tail_len_sweep_very_long_settle.sh
```

## Useful Overrides

- `RT595_MASTER_PROBE`
- `RT595_SLAVE_PROBE`
- `RT595_DEVICE`
- `RT595_MASTER_RUN_MODE`
- `RT595_TOOLCHAIN_ROOT`
- `LINKSERVER_BIN`
- `TRACE32_WRAPPER`

## Short Version

If you only want the exact minimal command sequence:

```bash
cd /Users/foxy/Downloads/rt595_sweep_bundle
./run_experiment.sh clean master_i3c_sdma_seed_tail_len_sweep
RT595_MASTER_RUN_MODE=none RT595_SLAVE_LIVE_RUN=1 ./run_experiment.sh master_i3c_sdma_seed_tail_len_sweep
./trace32/run_master_i3c_sdma_seed_tail_len_sweep_long_settle.sh
```
