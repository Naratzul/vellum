#include "common/sentry_report.h"

#include <cpptrace/from_current.hpp>

#include <atomic>
#include <string>
#include <typeinfo>
#include <vector>

#define SENTRY_BUILD_STATIC 1
#include <sentry.h>

namespace vellum::common {

namespace {
std::atomic<bool> g_sentryManualCapture{false};

void applyExceptionCpptraceStack(sentry_value_t exc,
                                 const cpptrace::stacktrace& st) {
  if (st.empty()) {
    sentry_value_set_stacktrace(exc, nullptr, 0);
    return;
  }
  std::vector<void*> ips;
  ips.reserve(st.frames.size());
  for (const auto& frame : st) {
    ips.push_back(reinterpret_cast<void*>(frame.raw_address));
  }
  sentry_value_set_stacktrace(exc, ips.data(), ips.size());
}

void setEventExtra(sentry_value_t event, const char* context,
                   const cpptrace::stacktrace& st) {
  const bool hasContext = context != nullptr && context[0] != '\0';
  const bool hasCpptrace = !st.empty();
  if (!hasContext && !hasCpptrace) {
    return;
  }
  sentry_value_t extra = sentry_value_new_object();
  if (hasContext) {
    sentry_value_set_by_key(extra, "context", sentry_value_new_string(context));
  }
  if (hasCpptrace) {
    const std::string cppText = st.to_string();
    sentry_value_set_by_key(extra, "cpptrace",
                            sentry_value_new_string(cppText.c_str()));
  }
  sentry_value_set_by_key(event, "extra", extra);
}
}  // namespace

void setSentryManualCaptureEnabled(bool enabled) {
  g_sentryManualCapture.store(enabled, std::memory_order_relaxed);
}

void captureSentryException(const std::exception& e, const char* context) {
  if (!g_sentryManualCapture.load(std::memory_order_relaxed)) {
    return;
  }
  const cpptrace::stacktrace& st = cpptrace::from_current_exception();
  sentry_value_t event = sentry_value_new_event();
  setEventExtra(event, context, st);
  sentry_value_t exc =
      sentry_value_new_exception(typeid(e).name(), e.what());
  applyExceptionCpptraceStack(exc, st);
  sentry_event_add_exception(event, exc);
  sentry_capture_event(event);
  sentry_flush(2000);
}

void captureSentryUnknown(const char* context) {
  if (!g_sentryManualCapture.load(std::memory_order_relaxed)) {
    return;
  }
  const cpptrace::stacktrace& st = cpptrace::from_current_exception();
  sentry_value_t event = sentry_value_new_event();
  setEventExtra(event, context, st);
  sentry_value_t exc = sentry_value_new_exception(
      "unknown", "Non-std exception (catch ...)");
  applyExceptionCpptraceStack(exc, st);
  sentry_event_add_exception(event, exc);
  sentry_capture_event(event);
  sentry_flush(2000);
}

}  // namespace vellum::common
