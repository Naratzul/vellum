#pragma once

#include <lsp/types.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "parser/parser.h"

namespace vellum {
class ImportLibrary;
class ImportResolver;
class Resolver;

using common::Opt;
using common::Shared;
using common::Vec;

enum class AnalysisKind { SemanticTokens, Full };

struct NavigationContext {
  ParserResult parseResult;
  Shared<Resolver> resolver;
  Shared<ImportResolver> importResolver;
  bool parseOk{false};
  bool semanticOk{false};

  NavigationContext() = default;
  NavigationContext(NavigationContext&&) = default;
  NavigationContext& operator=(NavigationContext&&) = default;
  NavigationContext(const NavigationContext&) = delete;
  NavigationContext& operator=(const NavigationContext&) = delete;
};

struct AnalysisResult {
  Vec<DiagnosticMessage> diagnostics;
  lsp::SemanticTokens semanticTokens;
  Opt<NavigationContext> navigation;
};

class DocumentAnalyzer {
 public:
  static AnalysisResult run(std::string_view source, std::string_view scriptName,
                            const Shared<ImportLibrary>& importLibrary,
                            AnalysisKind kind);
};

}  // namespace vellum
