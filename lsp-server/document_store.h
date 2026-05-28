#pragma once

#include <lsp/messages.h>
#include <lsp/types.h>

#include <cstdint>
#include <optional>
#include <string>

#include "common/fs.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "document_analyzer.h"

namespace vellum {
using common::Map;
using common::Opt;
using common::Shared;
using common::Vec;
using common::fs::path;

class ImportLibrary;

common::fs::path pathFromDocumentUri(const lsp::DocumentUri& uri);

struct CachedAnalysis {
  uint64_t version{0};
  bool fullAnalysisComplete{false};
  lsp::SemanticTokens semanticTokens;
  Vec<DiagnosticMessage> diagnostics;
};

class DocumentStore {
 public:
  void openOrUpdate(const path& filePath, std::string text);
  bool has(const path& filePath) const;
  std::string_view text(const path& filePath) const;
  std::string_view scriptName(const path& filePath) const;
  const CachedAnalysis& getOrAnalyze(
      const path& filePath, AnalysisKind kind,
      const Shared<ImportLibrary>& importLibrary);
  void invalidateAll();

 private:
  struct DocumentState {
    path filePath;
    std::string scriptName;
    std::string text;
    uint64_t version{0};
    Opt<CachedAnalysis> cache;
  };

  void ensureAnalysis(DocumentState& doc, AnalysisKind kind,
                      const Shared<ImportLibrary>& importLibrary);

  Map<path, DocumentState> documents;
};

}  // namespace vellum
