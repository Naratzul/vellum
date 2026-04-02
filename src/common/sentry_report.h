#pragma once

#include <exception>

namespace vellum::common {

// Call with true only after sentry_init() succeeds (sentry-native 0.13.x has no
// sentry_get_options()). Call with false before sentry_close().
void setSentryManualCaptureEnabled(bool enabled);

// Sends a Sentry error event when manual capture is enabled. No-op otherwise.
void captureSentryException(const std::exception& e,
                            const char* context = nullptr);
void captureSentryUnknown(const char* context = nullptr);

}  // namespace vellum::common
