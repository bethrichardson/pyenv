#!/bin/bash -eEu

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

source "$SCRIPT_DIR"/setup.inc

setup-common 'prod' "${1:-}"

activate-venv

hcsos_pg_create

deactivate-venv
