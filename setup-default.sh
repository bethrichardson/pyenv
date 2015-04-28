#!/bin/bash -eEu

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

"$SCRIPT_DIR"/setup.sh "${1:-}" "-default"