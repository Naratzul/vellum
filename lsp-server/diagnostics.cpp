#include "diagnostics.h"

#include <lsp/types.h>

#include "static_analyze.h"

namespace vellum {

static lsp::Position convert(const Location& loc) {
  return lsp::Position{.line = (unsigned int)loc.line,
                       .character = (unsigned int)loc.position};
}

static lsp::Diagnostic convert(const DiagnosticMessage& message) {
  return lsp::Diagnostic{
      .range = {.start = convert(message.token.location.start),
                .end = convert(message.token.location.end)},
      .message = message.message,
      .severity = lsp::DiagnosticSeverity::Error};
}

lsp::requests::TextDocument_Diagnostic::Result Diagnostics::getDiagnostics(
    std::string_view filename, std::string_view sourceCode) {
  StaticAnalyze analyzer;
  const std::vector<DiagnosticMessage> messages =
      analyzer.analyze(filename, sourceCode);

  lsp::RelatedFullDocumentDiagnosticReport result;
  for (const auto& message : messages) {
    result.items.push_back(convert(message));
  }

  return lsp::DocumentDiagnosticReport{result};
}
}  // namespace vellum