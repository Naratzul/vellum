#pragma once

#include <string>

namespace vellum::diag {

struct CrashReportingOptions {
  std::string appName;
  std::string release;
  bool enabled = true;
  std::string dsn;
  std::string environment;
};

class SentrySession {
 public:
  explicit SentrySession(CrashReportingOptions options);
  ~SentrySession();

  SentrySession(const SentrySession&) = delete;
  SentrySession& operator=(const SentrySession&) = delete;

  bool active() const { return active_; }

 private:
  bool active_ = false;
  bool initialized_ = false;
};

}  // namespace vellum::diag
