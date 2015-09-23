#!/bin/bash -eEu

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

source "$SCRIPT_DIR"/setup.sh

activate-venv
pip install cschcs-manager
hcsos-pg-create
hcsos-start
deactivate-venv
