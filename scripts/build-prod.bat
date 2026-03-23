@echo off
REM Production build script for Vellum (Windows).
REM Sensitive config (DSN) lives in local\, which is gitignored.
REM
REM Usage:
REM   1. Create local\sentry-dsn with your DSN (one line, no quotes)
REM   2. Run: scripts\build-prod.bat
REM
REM Or set VELLUM_SENTRY_DSN env var to override the config file.

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "CONFIG_FILE=%PROJECT_ROOT%\local\sentry-dsn"

REM DSN: env var overrides, then config file
if not defined VELLUM_SENTRY_DSN (
  if exist "%CONFIG_FILE%" (
    set /p VELLUM_SENTRY_DSN=<"%CONFIG_FILE%"
    REM Trim leading/trailing whitespace
    for /f "tokens=* delims= " %%a in ("!VELLUM_SENTRY_DSN!") do set VELLUM_SENTRY_DSN=%%a
  )
)

if not defined VELLUM_SENTRY_DSN (
  echo Error: Sentry DSN not found.
  echo Copy and edit: copy local-sentry-dsn.example local\sentry-dsn
  echo Or create %CONFIG_FILE% with your DSN ^(one line^).
  echo Or set VELLUM_SENTRY_DSN env var.
  exit /b 1
)

cmake -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVELLUM_SENTRY_DSN="%VELLUM_SENTRY_DSN%"
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config RelWithDebInfo --parallel
if errorlevel 1 exit /b 1

echo Production build complete. See %BUILD_DIR%\src
endlocal
