#!/bin/bash -eEu

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

source "$SCRIPT_DIR/setup.inc"

activate-venv
pyb --start-project
pyb install_dependencies
deactivate-venv
