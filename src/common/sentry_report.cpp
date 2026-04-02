#include "common/sentry_report.h"

#include <atomic>
#include <typeinfo>

#define SENTRY_BUILD_STATIC 1
#include <sentry.h>

namespace vellum::common {

namespace {
std::atomic<bool> g_sentryManualCapture{false};
}

void setSentryManualCaptureEnabled(bool enabled) {
  g_sentryManualCapture.store(enabled, std::memory_order_relaxed);
}

static void applyContext(sentry_value_t event, const char* context) {
  if (context == nullptr || context[0] == '\0') {
    return;
  }
  sentry_value_t extra = sentry_value_new_object();
  sentry_value_set_by_key(extra, "context", sentry_value_new_string(context));
  sentry_value_set_by_key(event, "extra", extra);
}

void captureSentryException(const std::exception& e, const char* context) {
  if (!g_sentryManualCapture.load(std::memory_order_relaxed)) {
    return;
  }
  sentry_value_t event = sentry_value_new_event();
  applyContext(event, context);
  sentry_value_t exc =
      sentry_value_new_exception(typeid(e).name(), e.what());
  sentry_value_set_stacktrace(exc, nullptr, 0);
  sentry_event_add_exception(event, exc);
  sentry_capture_event(event);
  sentry_flush(2000);
}

void captureSentryUnknown(const char* context) {
  if (!g_sentryManualCapture.load(std::memory_order_relaxed)) {
    return;
  }
  sentry_value_t event = sentry_value_new_event();
  applyContext(event, context);
  sentry_value_t exc = sentry_value_new_exception(
      "unknown", "Non-std exception (catch ...)");
  sentry_value_set_stacktrace(exc, nullptr, 0);
  sentry_event_add_exception(event, exc);
  sentry_capture_event(event);
  sentry_flush(2000);
}

}  // namespace vellum::common
