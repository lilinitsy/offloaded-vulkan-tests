#!/usr/bin/env bash

set -euxo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

(cd obj-loader && crate2nix generate)
