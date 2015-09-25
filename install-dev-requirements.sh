#!/bin/bash -eEu

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

source "$SCRIPT_DIR/setup.inc"

assert-not-root

activate-venv

configure-dev-requirements

deactivate-venv
