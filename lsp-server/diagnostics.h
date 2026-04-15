#pragma once

#include <lsp/messages.h>

#include "common/types.h"

namespace vellum {
using common::Shared;

class ImportLibrary;

class Diagnostics {
 public:
  lsp::requests::TextDocument_Diagnostic::Result getDiagnostics(
      std::string_view filename, std::string_view sourceCode,
      const Shared<ImportLibrary>& importLibrary);
};
}  // namespace vellum