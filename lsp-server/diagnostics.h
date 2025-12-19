#pragma once

#include <lsp/messages.h>

namespace vellum {
class Diagnostics {
 public:
  lsp::requests::TextDocument_Diagnostic::Result getDiagnostics(
      std::string_view filename, std::string_view sourceCode);

 private:
};
}  // namespace vellum