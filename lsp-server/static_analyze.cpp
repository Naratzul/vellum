#include "static_analyze.h"

#include "document_analyzer.h"

namespace vellum {
using common::Vec;

Vec<DiagnosticMessage> StaticAnalyze::analyze(
    std::string_view filename, std::string_view sourceCode,
    const Shared<ImportLibrary>& importLibrary) {
  return DocumentAnalyzer::run(sourceCode, filename, importLibrary,
                               AnalysisKind::Full)
      .diagnostics;
}

}  // namespace vellum