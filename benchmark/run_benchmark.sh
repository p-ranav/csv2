#!/usr/bin/env bash
# Benchmarks ./main against a CSV file with hyperfine, then cross-checks the
# reported rows/cells against an independent Python parse of the same file.
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 <csv_file> [delimiter]" >&2
  echo "  delimiter: ',' ';' '|' or 'tab' (default: ',')" >&2
  exit 2
fi

csv_file="$1"
delimiter="${2:-,}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_dir"

if ! command -v hyperfine >/dev/null 2>&1; then
  echo "error: hyperfine is not installed (brew install hyperfine)" >&2
  exit 2
fi

echo "==> Building benchmark binary"
make

echo
echo "==> Benchmarking '$csv_file' with hyperfine"
hyperfine --warmup 3 --runs 5 "./main $csv_file $delimiter"

echo
echo "==> Verifying rows/cells against Python"
python3 verify.py "$csv_file" "$delimiter"

