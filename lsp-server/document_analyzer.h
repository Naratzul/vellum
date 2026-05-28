#pragma once

#include <lsp/types.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "common/types.h"
#include "compiler/compiler_error_handler.h"

namespace vellum {
using common::Shared;
using common::Vec;

class ImportLibrary;

enum class AnalysisKind { SemanticTokens, Full };

struct AnalysisResult {
  Vec<DiagnosticMessage> diagnostics;
  lsp::SemanticTokens semanticTokens;
};

class DocumentAnalyzer {
 public:
  static AnalysisResult run(std::string_view source, std::string_view scriptName,
                            const Shared<ImportLibrary>& importLibrary,
                            AnalysisKind kind);
};

}  // namespace vellum
