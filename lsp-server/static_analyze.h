#pragma once

#include "compiler/compiler_error_handler.h"
#include <vector>

namespace vellum {
class StaticAnalyze {
 public:
  std::vector<DiagnosticMessage> analyze(std::string_view filename, std::string_view sourceCode);
 private:
};
}  // namespace vellum