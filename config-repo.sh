#!/bin/bash -eEu

if [ ! -d ".git" ]; then
    echo "Cannot find Git metadata in the current directory '$(pwd -P)'. Running in a wrong dir perhaps?" >&2
    exit 1
fi

SCRIPT_DIR="$(dirname "$0")"
SCRIPT_DIR="$(cd "$SCRIPT_DIR" && pwd -P)"

source "$SCRIPT_DIR/setup.inc"

activate-venv
flake8 --install-hook
git config flake8.strict true
deactivate-venv
