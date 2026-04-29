#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
MCP_DIR="$SCRIPT_DIR/.vscode"
MCP_FILE="$MCP_DIR/mcp.json"
LAUTERBACH_WRAPPER="$SCRIPT_DIR/.local/bin/lauterbachdebugger-mcp"
RT595_TRACE_WRAPPER="$SCRIPT_DIR/.local/bin/rt595-trace"
RT595_TRACE_SOURCE_ROOT_DEFAULT="/Users/foxy/intent/workspaces/hidden-gibbon/repo/src"

mkdir -p "$MCP_DIR" "$SCRIPT_DIR/.local/lauterbach-mcp-cache"

cat >"$MCP_FILE" <<EOF
{
  "servers": {
    "lauterbach-trace32": {
      "type": "stdio",
      "command": "$LAUTERBACH_WRAPPER",
      "env": {
        "T32_HOST": "localhost",
        "T32_PORT": "20000",
        "T32_PROTOCOL": "TCP"
      }
    },
    "rt595-trace": {
      "type": "stdio",
      "command": "$RT595_TRACE_WRAPPER",
      "env": {
        "RT595_TRACE_SOURCE_ROOT": "$RT595_TRACE_SOURCE_ROOT_DEFAULT"
      }
    }
  }
}
EOF

echo "Wrote $MCP_FILE"