#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
go test ./...
go build -o build/labstruct-go .
python3 tests/cli.py build/labstruct-go --go
