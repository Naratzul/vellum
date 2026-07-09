#include "diagnostics.h"

#include <lsp/types.h>

#include "lsp_locations.h"

namespace vellum {

static lsp::Diagnostic convert(const DiagnosticMessage& message) {
  return lsp::Diagnostic{
      .range = toLspRange(message.token),
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
