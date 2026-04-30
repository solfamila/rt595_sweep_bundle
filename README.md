# RT595 Sweep Bundle

This folder is a portable sweep bundle for `master_i3c_sdma_seed_tail_len_sweep`.
It also includes the focused `master_i3c_rx_request_semantics_probe` used to
validate CPU, bulk DMA, and chained RX behavior on the RT595 master, plus the
follow-on `master_i3c_sdma_rx_seed6_seed_only_no_cpu_irq` probe used to test
whether the native 6-byte RX DMA seed request fires at all.
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
  5-second replay, RX request-semantics probe, and RX seed-only probe flows.

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

This wrapper now prints the retained replay state automatically after the
5-second stop.

### Continue the current execution for another 5 seconds

```bash
./trace32/continue_master_i3c_sdma_seed_tail_len_sweep_5s.sh
```

This wrapper also prints the retained replay state automatically after the
continue window stops.

### Print the retained replay state without running

```bash
./trace32/probe_master_i3c_sdma_seed_tail_len_sweep_replay_state.sh
```

### Probe the retained failure snapshot

```bash
./trace32/probe_master_i3c_sdma_seed_tail_len_sweep_failure_state.sh
```

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
4. `dataIrqDelta=0`, `protocolIrqDelta=0`, and `ibiIrqDelta=0` show the
  passing path did not depend on CM33 IRQ cleanup.
5. `d0..d3 = 1,2,3,4` confirms the returned payload matches the transmitted
  data.

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
