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
