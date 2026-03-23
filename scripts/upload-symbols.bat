@echo off
REM Upload debug symbols (PDB, dSYM, etc.) to Sentry (Windows).
REM Sensitive config (org/project) lives in local\, which is gitignored.
REM
REM Prerequisites:
REM   - sentry-cli installed (https://docs.sentry.io/cli/)
REM   - SENTRY_AUTH_TOKEN env var (create at sentry.io ^> Settings ^> Auth Tokens)
REM
REM Usage:
REM   1. Copy local-sentry-upload.example to local\sentry-upload, edit org/project
REM   2. Run: scripts\upload-symbols.bat [path]
REM
REM Path defaults to build\src (contains vellum.exe + vellum.pdb).

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
set "BUILD_SRC=%PROJECT_ROOT%\build\src"
set "CONFIG_FILE=%PROJECT_ROOT%\local\sentry-upload"

if "%~1"=="" (set "UPLOAD_PATH=%BUILD_SRC%") else (set "UPLOAD_PATH=%~1")

REM Check sentry-cli
where sentry-cli >nul 2>&1
if errorlevel 1 (
  echo Error: sentry-cli not found. Install from https://docs.sentry.io/cli/
  exit /b 1
)

REM Auth token required
if not defined SENTRY_AUTH_TOKEN (
  echo Error: SENTRY_AUTH_TOKEN is not set.
  echo Create a token at sentry.io ^> Settings ^> Auth Tokens
  exit /b 1
)

REM Read org/project from config
if exist "%CONFIG_FILE%" (
  for /f "usebackq eol=# tokens=1,* delims==" %%a in ("%CONFIG_FILE%") do (
    if "%%a"=="SENTRY_ORG" set "SENTRY_ORG=%%b"
    if "%%a"=="SENTRY_PROJECT" set "SENTRY_PROJECT=%%b"
  )
)

if not defined SENTRY_ORG (
  echo Error: SENTRY_ORG not set.
  echo Copy and edit: copy local-sentry-upload.example local\sentry-upload
  exit /b 1
)
if not defined SENTRY_PROJECT (
  echo Error: SENTRY_PROJECT not set.
  echo Copy and edit: copy local-sentry-upload.example local\sentry-upload
  exit /b 1
)

if not exist "%UPLOAD_PATH%" (
  echo Error: Upload path not found: %UPLOAD_PATH%
  echo Build the project first ^(e.g. scripts\build-prod.bat^)
  exit /b 1
)

echo Uploading debug symbols from %UPLOAD_PATH%...
sentry-cli debug-files upload -o "%SENTRY_ORG%" -p "%SENTRY_PROJECT%" "%UPLOAD_PATH%"

echo Done. Use --wait to block until processing completes ^(e.g. for immediate testing^).
endlocal
