#include "diagnostics.h"

#include <lsp/types.h>

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

lsp::requests::TextDocument_Diagnostic::Result Diagnostics::fromCache(
    const CachedAnalysis& analysis) {
  lsp::RelatedFullDocumentDiagnosticReport result;
  result.items.reserve(analysis.diagnostics.size());

  for (const auto& message : analysis.diagnostics) {
    result.items.push_back(convert(message));
  }

  return lsp::DocumentDiagnosticReport{result};
}
}  // namespace vellum