#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}/.."
DB_PATH="${REPO_ROOT}/insurance.db"

cd "${REPO_ROOT}"

if [[ -f "${DB_PATH}" ]]; then
  echo "Removing existing database: ${DB_PATH}"
  rm -f "${DB_PATH}"
fi

mkdir -p build
cmake -S . -B build
cmake --build build --config Release

echo "Running migration mode..."
./build/insurance_system --migrate

echo "Database initialized at ${DB_PATH}."
