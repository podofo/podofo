#!/usr/bin/env sh
# Enable the checkout of the governance/security-scanner submodule (disabled by default) and fetch it
set -e

ROOT="$(dirname "$0")/.."
git -C "$ROOT" config submodule.governance/security-scanner.update checkout
git -C "$ROOT" submodule update --init governance/security-scanner
