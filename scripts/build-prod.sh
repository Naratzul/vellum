#!/usr/bin/env bash
# Production build script for Vellum.
# Sensitive config (DSN) lives in local/, which is gitignored.
#
# Usage:
#   1. Create local/sentry-dsn with your DSN (one line, no quotes)
#   2. Run: ./scripts/build-prod.sh
#
# Or set VELLUM_SENTRY_DSN env var to override the config file.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
CONFIG_FILE="${PROJECT_ROOT}/local/sentry-dsn"

# DSN: env var overrides, then config file
if [[ -z "${VELLUM_SENTRY_DSN}" ]]; then
  if [[ -f "${CONFIG_FILE}" ]]; then
    VELLUM_SENTRY_DSN="$(sed -n '1p' "${CONFIG_FILE}" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
  fi
fi

if [[ -z "${VELLUM_SENTRY_DSN}" ]]; then
  echo "Error: Sentry DSN not found."
  echo "Copy and edit: cp local-sentry-dsn.example local/sentry-dsn"
  echo "Or create ${CONFIG_FILE} with your DSN (one line)."
  echo "Or set VELLUM_SENTRY_DSN env var."
  exit 1
fi

cmake -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DVELLUM_SENTRY_DSN="${VELLUM_SENTRY_DSN}"

cmake --build "${BUILD_DIR}" --config RelWithDebInfo --parallel

echo "Production build complete: ${BUILD_DIR}/src/vellum"
