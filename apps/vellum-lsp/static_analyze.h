#pragma once

#include <vector>

#include "compiler/compiler_error_handler.h"

namespace vellum {
using common::Vec;
using common::Shared;

class ImportLibrary;

class StaticAnalyze {
 public:
  Vec<DiagnosticMessage> analyze(std::string_view filename,
                                 std::string_view sourceCode,
                                 const Shared<ImportLibrary>& importLibrary);
};
}  // namespace vellum