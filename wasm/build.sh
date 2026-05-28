#!/bin/bash
# Thin wrapper around the Makefile rule. See `make help` for related targets.
set -euo pipefail
cd "$(dirname "$0")/.."
exec make wasm "$@"
