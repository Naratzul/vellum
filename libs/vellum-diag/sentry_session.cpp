#include "sentry_session.h"

#include "platform_paths.h"
#include "sentry_report.h"

#include <cstdlib>

#define SENTRY_BUILD_STATIC 1
#include <sentry.h>

namespace vellum::diag {

SentrySession::SentrySession(CrashReportingOptions options) {
  if (!options.enabled) {
    return;
  }

  sentry_options_t* sentry_opts = sentry_options_new();
  if (!options.dsn.empty()) {
    sentry_options_set_dsn(sentry_opts, options.dsn.c_str());
  } else if (const char* dsn = std::getenv("SENTRY_DSN")) {
    sentry_options_set_dsn(sentry_opts, dsn);
  }

  sentry_options_set_release(sentry_opts, options.release.c_str());

#ifdef _WIN32
  sentry_options_set_database_pathw(
      sentry_opts,
      sentryDatabasePathW(std::wstring(options.appName.begin(),
                                       options.appName.end()))
          .c_str());
#else
  sentry_options_set_database_path(
      sentry_opts, sentryDatabasePath(options.appName).c_str());
#endif

  if (!options.environment.empty()) {
    sentry_options_set_environment(sentry_opts, options.environment.c_str());
  } else if (const char* env = std::getenv("SENTRY_ENVIRONMENT")) {
    sentry_options_set_environment(sentry_opts, env && env[0] ? env : "development");
  } else {
    sentry_options_set_environment(sentry_opts, "development");
  }

#ifndef NDEBUG
  sentry_options_set_debug(sentry_opts, 1);
#endif

  if (sentry_init(sentry_opts) == 0) {
    active_ = true;
    initialized_ = true;
    setManualCaptureEnabled(true);
  }
}

SentrySession::~SentrySession() {
  if (initialized_) {
    setManualCaptureEnabled(false);
    sentry_close();
  }
}

}  // namespace vellum::diag
