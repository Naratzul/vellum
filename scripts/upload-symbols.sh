#!/usr/bin/env bash
# Upload debug symbols (PDB, dSYM, etc.) to Sentry.
# Sensitive config (org/project) lives in local/, which is gitignored.
#
# Prerequisites:
#   - sentry-cli installed (https://docs.sentry.io/cli/)
#   - SENTRY_AUTH_TOKEN env var (create at sentry.io > Settings > Auth Tokens)
#
# Usage:
#   1. Copy local-sentry-upload.example to local/sentry-upload, edit org/project
#   2. Run: ./scripts/upload-symbols.sh [path]
#
# Path defaults to build/src (contains vellum exe + PDB on Windows, or vellum + dSYM on macOS).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_SRC="${PROJECT_ROOT}/build/src"
CONFIG_FILE="${PROJECT_ROOT}/local/sentry-upload"
UPLOAD_PATH="${1:-${BUILD_SRC}}"

# Check sentry-cli
if ! command -v sentry-cli &>/dev/null; then
  echo "Error: sentry-cli not found. Install from https://docs.sentry.io/cli/"
  exit 1
fi

# Auth token required
if [[ -z "${SENTRY_AUTH_TOKEN}" ]]; then
  echo "Error: SENTRY_AUTH_TOKEN is not set."
  echo "Create a token at sentry.io > Settings > Auth Tokens"
  exit 1
fi

# Read org/project from config
if [[ -f "${CONFIG_FILE}" ]]; then
  while IFS= read -r line; do
    [[ "${line%%#*}" =~ ^[[:space:]]*$ ]] && continue
    line="${line%%#*}"
    line="${line#"${line%%[![:space:]]*}}"
    line="${line%"${line##*[![:space:]]}}"
    if [[ "$line" =~ ^([A-Za-z_][A-Za-z0-9_]*)=(.*)$ ]]; then
      export "${BASH_REMATCH[1]}=${BASH_REMATCH[2]}"
    fi
  done < "${CONFIG_FILE}"
fi

if [[ -z "${SENTRY_ORG}" ]] || [[ -z "${SENTRY_PROJECT}" ]]; then
  echo "Error: SENTRY_ORG and SENTRY_PROJECT not set."
  echo "Copy and edit: cp local-sentry-upload.example local/sentry-upload"
  exit 1
fi

if [[ ! -d "${UPLOAD_PATH}" ]]; then
  echo "Error: Upload path not found: ${UPLOAD_PATH}"
  echo "Build the project first (e.g. ./scripts/build-prod.sh)"
  exit 1
fi

echo "Uploading debug symbols from ${UPLOAD_PATH}..."
sentry-cli debug-files upload -o "${SENTRY_ORG}" -p "${SENTRY_PROJECT}" "${UPLOAD_PATH}"

echo "Done. Use --wait to block until processing completes (e.g. for immediate testing)."
