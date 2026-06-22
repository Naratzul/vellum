#pragma once

#include <exception>

namespace vellum::diag {

void setManualCaptureEnabled(bool enabled);

void captureException(const std::exception& e, const char* context = nullptr);
void captureUnknown(const char* context = nullptr);

}  // namespace vellum::diag
