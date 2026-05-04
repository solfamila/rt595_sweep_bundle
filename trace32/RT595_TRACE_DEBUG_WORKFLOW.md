# RT595 Trace Debug Workflow

This document records the validated workflow used to capture, export, ingest,
and analyze RT595 off-chip trace for the block-stream failure work in this
bundle. It also records the failure modes that wasted time and the exact fixes
that unblocked them.

Use this workflow when retained state and plain TRACE32 stop points are not
enough, especially when the interesting failure happens inside the master read
path after the IBI handshake.

## Scope

This guide is written for the block-stream experiment:

- `master_i3c_dma_official_rx_smartdma_wake_block_stream/`

It also applies to other RT595 experiments in this bundle that use the same:

- TRACE32 off-chip ETM capture flow
- `rt595-trace` DuckDB ingestion flow
- VS Code MCP analysis flow
- IDA correlation flow

## Bundle-Specific Paths

These were the important working paths for the validated flow:

- Bundle root: `/Users/foxy/Downloads/rt595_sweep_bundle`
- TRACE32 MCP config: `.vscode/mcp.json`
- Bundle trace database: `.local/rt595-trace/rt595_trace.duckdb`
- Bundle `rt595-trace` launcher: `.local/bin/rt595-trace`
- Bundle TRACE32 wrapper: `.local/trace32/t32cmd_nostop`
- Block-stream capture helper: `trace32/capture_master_i3c_dma_official_rx_smartdma_wake_block_stream_trace.sh`
- Block-stream master AXF: `master_i3c_dma_official_rx_smartdma_wake_block_stream/_build/master/evkmimxrt595_ezhb.axf`
- Default trace export directory: `.local/rt595-trace/trace32_exports/`

The bundled `rt595-trace` launcher points back to the hidden-gibbon source tree
for the actual `rt595_trace` Python package. In the validated setup that source
root was:

- `/Users/foxy/intent/workspaces/hidden-gibbon/repo/src`

If that source tree moves, update `RT595_TRACE_SOURCE_ROOT` or refresh
`.vscode/mcp.json`.

## First Principles

The rules that mattered most were:

1. Capture raw trace as `.ad` first. Do not make the workflow depend on a
   `Trace.List` text export.
2. Use a fresh capture ID for each serious debug slice so the DB state is
   obvious.
3. Ingest ELF symbols before trying to interpret addresses.
4. Export BranchFlow with signed negative TRACE32 record indices. Positive
   `1..N` ranges were the main source of false "empty trace" conclusions.
5. Use BranchFlow even if CSVFunc is empty. CSVFunc did not unblock the current
   block-stream investigation.
6. Load the exact same AXF into IDA that was used for symbol ingest.

## Recommended End-To-End Flow

### 1. Build the failing master and leave the slave running

Build the exact block-stream repro and keep the master under TRACE32 control:

```bash
cd /Users/foxy/Downloads/rt595_sweep_bundle

RT595_EXTRA_MASTER_DEFINES='I3C_STREAM_BLOCK_BYTES=32 I3C_STREAM_BLOCK_COUNT=4 I3C_DMA_OFFICIAL_INTER_CHUNK_SETTLE_US=0' \
RT595_MASTER_RUN_MODE=none \
RT595_SLAVE_LIVE_RUN=1 \
./run_experiment.sh master_i3c_dma_official_rx_smartdma_wake_block_stream
```

If you need to restart the slave directly under LinkServer, the validated local
command shape was:

```bash
pkill -f 'crt_emu_cm_redlink.*GRA1CQLQ|LinkServer.*run.*GRA1CQLQ|/dev/cu.usbmodemGRA1CQLQ2:115200' || true
"$PWD/.local/linkserver/extracted/flatten_LinkServer_25.12.83.pkg/Payload/dist/LinkServer" run \
  -p GRA1CQLQ \
  --exit-timeout 180 \
  --mode serial:/dev/cu.usbmodemGRA1CQLQ2:115200 \
  MIMXRT595S:EVK-MIMXRT595 \
  "$PWD/master_i3c_dma_official_rx_smartdma_wake_block_stream/_build/slave/slave.axf"
```

Adjust the probe serial and tty device for a different setup.

### 2. Capture raw TRACE32 data without exporting `Trace.List`

Use the block-stream capture helper:

```bash
TRACE_CAPTURE_NAME=blockstream_chunk1_trace \
./trace32/capture_master_i3c_dma_official_rx_smartdma_wake_block_stream_trace.sh
```

The validated helper does all of the following:

1. Loads the block-stream master AXF.
2. Arms ETM off-chip trace.
3. Leaves `Trace.List` in the script only as a TRACE32 setup/UI command.
4. Saves the raw `.ad` trace with `Trace.SAVE`.
5. Prints `traceCapture=` and `dmaWakeBlockStreamFinal=` summaries.

Important distinction:

- The working rule is "do not export or depend on `Trace.List` text".
- It is still acceptable to call `Trace.List` inside the TRACE32 script if that
  helps TRACE32 materialize the view correctly.

The default output file is:

- `.local/trace-captures/<capture-name>.ad`

Sanity check after capture:

1. The helper should print a non-zero `Trace.RECORDS()` count.
2. TRACE32 `Trace.Chart.Symbol` activity is a valid sign that the capture is
   populated even if an early export attempt looks empty.

### 3. Prepare a fresh `rt595-trace` database state

Use stable variables for the rest of the workflow:

```bash
cd /Users/foxy/Downloads/rt595_sweep_bundle

CAPTURE_ID=blockstream_chunk1_mismatch
DB=.local/rt595-trace/rt595_trace.duckdb
AXF=$PWD/master_i3c_dma_official_rx_smartdma_wake_block_stream/_build/master/evkmimxrt595_ezhb.axf
EXPORT_DIR=.local/rt595-trace/trace32_exports/$CAPTURE_ID
TRACE_SRC=${RT595_TRACE_SOURCE_ROOT:-/Users/foxy/intent/workspaces/hidden-gibbon/repo/src}
```

If the database file already exists and you only want a clean capture state,
reset the capture:

```bash
./.local/bin/rt595-trace capture reset --db "$DB" --capture-id "$CAPTURE_ID" --yes
```

If you deliberately removed the database file and need to create it again,
do not rely on the launcher for `init-db`. The validated workaround was:

```bash
rm -f "$DB"
PYTHONPATH="$TRACE_SRC" \
./.local/rt595-trace-venv/bin/python -m rt595_trace.cli init-db \
  --db "$DB" \
  --capture-id "$CAPTURE_ID" \
  --name "$CAPTURE_ID"
```

This workaround matters because the bundle launcher currently expects the DB
file to exist before it can run `init-db`.

### 4. Ingest symbols before any trace analysis

```bash
./.local/bin/rt595-trace ingest symbols-elf \
  --db "$DB" \
  --capture-id "$CAPTURE_ID" \
  --elf "$AXF"
```

This step is mandatory if you want `resolve_address`, `find_symbol`,
`calls_from`, `calls_to`, and IDA correlation to make sense.

### 5. Export and ingest a focused BranchFlow tail

This was the key fix that made the workflow work.

Use signed negative record indices, not positive `1..Trace.RECORDS()` ranges:

```bash
mkdir -p "$EXPORT_DIR"

./.local/bin/rt595-trace t32 export branchflow-range \
  --output "$EXPORT_DIR/neg_tail_branchflow.txt" \
  --start-record -300000 \
  --end-record -1
```

Quick verification:

```bash
wc -l "$EXPORT_DIR/neg_tail_branchflow.txt"
sed -n '1,12p' "$EXPORT_DIR/neg_tail_branchflow.txt"
```

If the export worked, the file should have real rows, not just a short header.

Ingest the same signed tail slice:

```bash
./.local/bin/rt595-trace t32 ingest branchflow-range \
  --db "$DB" \
  --capture-id "$CAPTURE_ID" \
  --start-record -300000 \
  --end-record -1 \
  --export-dir "$EXPORT_DIR"
```

The validated block-stream ingest produced:

- populated `branch_events`
- populated `branch_edge_summary`
- zero parser errors

### 6. Treat CSVFunc as optional

CSVFunc export was still header-only in the validated block-stream session even
after BranchFlow was working. Do not block the workflow on this.

BranchFlow plus symbol ingest was sufficient to:

1. identify the real hot path
2. prove when the trace was centered too late on the debug-print tail
3. drive the final IDA correlation

### 7. Analyze the trace through the RT595 MCP server

Once the bundle workspace is open in VS Code with `.vscode/mcp.json`, the most
useful RT595 MCP tools for this workflow were:

```text
trace_overview(capture_id="blockstream_chunk1_mismatch")
find_symbol(query="run_window_plain_start_read")
calls_from(caller_symbol="run_window_plain_start_read", capture_id="blockstream_chunk1_mismatch")
calls_to(target_symbol="wait_for_post_ibi_read_ctrl_done", capture_id="blockstream_chunk1_mismatch")
branch_window(evidence_handle="...", before=20, after=20)
resolve_address(address="0x202822d0", capture_id="blockstream_chunk1_mismatch")
```

Recommended analysis order:

1. `trace_overview` to confirm the DB really contains branch data.
2. `find_symbol` or `calls_from` for the owning master helper.
3. `branch_window` around evidence handles from the interesting helper, not the
   very last branch event.
4. `resolve_address` for unresolved hot addresses before switching to IDA.

Important lesson:

- The last branch events in a failing run were dominated by `DbgConsole_*`
  printing after failure.
- The useful branch windows were earlier, centered on evidence handles inside
  `run_window_plain_start_read` and `wait_for_post_ibi_read_ctrl_done`.

### 8. Correlate the trace in IDA

Load the exact master AXF used for symbol ingest:

```text
/Users/foxy/Downloads/rt595_sweep_bundle/master_i3c_dma_official_rx_smartdma_wake_block_stream/_build/master/evkmimxrt595_ezhb.axf
```

For the current block-stream failure, the most useful IDA functions were:

- `wait_for_post_ibi_read_ctrl_done` at `0x2028156c`
- `run_window_plain_start_read` at `0x20281fd8`

The most useful IDA MCP operations were:

```text
list_instances
select_instance
analyze_function(addr="0x2028156c", include_asm=true)
analyze_function(addr="0x20281fd8", include_asm=true)
basic_blocks(addrs="0x20281fd8")
set_comments(items=[...])
```

Always make sure the AXF loaded in IDA matches the AXF used for symbol ingest.
If they differ, address-to-source correlation will drift.

## Current Example Result From This Workflow

The validated block-stream session established all of the following:

1. The trace was real and populated.
2. The empty-export conclusion was false.
3. The root cause was not in the late `DbgConsole_*` tail.
4. The failing chunk reached the first `wait_for_post_ibi_read_ctrl_done`
   call site after `I3C_MasterStartWithRxSize`.
5. The failing chunk did not reach the second `wait_for_post_ibi_read_ctrl_done`
   site after `I3C_MasterStop`.
6. The caller-side loop in `run_window_plain_start_read` locally returns
   `kStatus_I3C_Nak` when NACK is present and `rxCount == 0`, which matched the
   observed `rs=0x1EDE` failure.

The relevant source is in:

- `master_i3c_dma_official_rx_smartdma_wake_block_stream/ezh_test_standalone.c`

The relevant hot addresses from the validated session were:

- `0x202820dc`: first `wait_for_post_ibi_read_ctrl_done` after start
- `0x202821e2`: second `wait_for_post_ibi_read_ctrl_done` after stop
- `0x202822d0`: local `kStatus_I3C_Nak` return in the caller loop
- `0x202822ec`: second local `kStatus_I3C_Nak` return in the caller loop
- `0x20282320`: loop-side `I3C_MasterCheckAndClearError` path

## Troubleshooting And Known Failure Modes

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| TRACE32 UI shows lots of activity, but the export is empty or header-only | Positive record numbering was used for export | Use signed negative records such as `--start-record -300000 --end-record -1` |
| `Trace.RECORDS()` is large, but `branchflow-range` still looks empty | Same record-range mistake | Re-export from the tail with signed negative indices |
| `./.local/bin/rt595-trace init-db ...` fails after deleting the DB | Bundle launcher currently expects the DB file to exist already | Run `PYTHONPATH="$TRACE_SRC" ./.local/rt595-trace-venv/bin/python -m rt595_trace.cli init-db ...` |
| RT595 MCP queries show the wrong captures or stale data | More than one `rt595-trace` server is running, or VS Code is attached to the wrong one | Kill stale `rt595-trace` servers, reopen the bundle workspace, and verify `.vscode/mcp.json` points at the bundle DB |
| Ingest fails with a DuckDB lock error | Another server process still holds the DB open for write | Stop the old `rt595-trace` server process and retry the ingest |
| CSVFunc export is only a short header | CSVFunc is not currently reliable for this block-stream capture | Ignore CSVFunc and proceed with BranchFlow plus symbol ingest |
| The latest branch window only shows `DbgConsole_*` | The window is centered on the post-failure print tail | Use earlier evidence handles from `calls_from(run_window_plain_start_read)` or the helper call sites |
| RT595 trace addresses do not resolve to symbols | ELF symbols were not ingested, or the wrong AXF was used | Re-run `ingest symbols-elf` with the master AXF and load the same AXF in IDA |
| The capture helper stops too early | TRACE32 timeout or run-wait is too short | Increase `TRACE_CAPTURE_TIMEOUT_SECONDS`, `TRACE_CAPTURE_WAIT_MS`, and `TRACE_CAPTURE_RUN_WAIT_SECONDS` |
| A reviewer asks not to export `Trace.List` text | Confusion between calling `Trace.List` and exporting it | Keep `Trace.List` as a TRACE32 setup command if needed, but do not printer-export `WinPrint.Trace.List` |
| MCP tool schemas are unclear | Local agent/tooling context does not show the HTTP schema | Query the local IDA MCP endpoint with `tools/list` and inspect the returned `inputSchema` |

## Fast Checklist

Use this exact checklist next time:

1. Build the master and start the slave live.
2. Capture raw `.ad` trace with the block-stream helper.
3. Reset the capture or recreate the DB with the `init-db` workaround.
4. Ingest symbols from the exact master AXF.
5. Export and ingest BranchFlow with signed negative record indices.
6. Confirm `trace_overview` shows populated branch tables.
7. Center `branch_window` on earlier helper evidence handles, not the final
   debug-print tail.
8. Load the same AXF in IDA and correlate the hot addresses there.

If any step appears empty, assume the workflow is wrong before assuming the
trace is empty.