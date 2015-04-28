#!/bin/bash -eEu

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

source "$SCRIPT_DIR/setup.inc"

install-pip-virtualenv
install-pyhome "${1:-}"
configure-bash-profile
activate-venv
configure"${2:-}"-venv
deactivate-venv
