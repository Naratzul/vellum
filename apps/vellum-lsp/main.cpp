#include "server.h"

#include "run_guarded.h"
#include "sentry_report.h"
#include "sentry_session.h"

#ifndef VELLUM_SENTRY_RELEASE
#error VELLUM_SENTRY_RELEASE must be defined by CMake
#endif

namespace {
vellum::diag::CrashReportingOptions crashReportingOptions() {
  vellum::diag::CrashReportingOptions options{
      .appName = "vellum-lsp",
      .release = VELLUM_SENTRY_RELEASE,
  };
#ifdef VELLUM_SENTRY_DSN
  options.dsn = VELLUM_SENTRY_DSN;
#endif
#ifdef VELLUM_SENTRY_ENVIRONMENT
  options.environment = VELLUM_SENTRY_ENVIRONMENT;
#endif
  return options;
}
}  // namespace

int main() {
  using namespace vellum;

  const vellum::diag::SentrySession sentrySession(crashReportingOptions());

  bool result = false;
  vellum::diag::runGuarded(
      [&] { result = LspServer().run(); },
      [&](const std::exception& e) {
        vellum::diag::captureException(e, "main");
      },
      [&] { vellum::diag::captureUnknown("main"); });

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
