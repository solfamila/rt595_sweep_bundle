#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
MCP_DIR="$SCRIPT_DIR/.vscode"
MCP_FILE="$MCP_DIR/mcp.json"
LAUTERBACH_WRAPPER="$SCRIPT_DIR/.local/bin/lauterbachdebugger-mcp"

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
    }
  }
}
EOF

echo "Wrote $MCP_FILE"