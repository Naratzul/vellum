#include "document_store.h"

#include <stdexcept>
#include <string_view>

#include "common/fs.h"

namespace vellum {
using common::filenameWithoutExt;

path pathFromDocumentUri(const lsp::DocumentUri& uri) {
  return path{uri.path()};
}

void DocumentStore::openOrUpdate(const path& filePath, std::string text) {
  auto& doc = documents[filePath];
  doc.filePath = filePath;
  doc.scriptName = filenameWithoutExt(filePath.string());
  doc.text = std::move(text);
  ++doc.version;
  doc.cache.reset();
}

bool DocumentStore::has(const path& filePath) const {
  return documents.contains(filePath);
}

std::string_view DocumentStore::text(const path& filePath) const {
  return documents.at(filePath).text;
}

std::string_view DocumentStore::scriptName(const path& filePath) const {
  return documents.at(filePath).scriptName;
}

void DocumentStore::ensureAnalysis(DocumentState& doc, AnalysisKind kind,
                                   const Shared<ImportLibrary>& importLibrary) {
  if (doc.cache && doc.cache->version == doc.version) {
    if (kind == AnalysisKind::SemanticTokens) {
      return;
    }
    if (kind == AnalysisKind::Full && doc.cache->fullAnalysisComplete) {
      return;
    }
  }

  const AnalysisResult result = DocumentAnalyzer::run(
      doc.text, doc.scriptName, importLibrary, kind);

  if (!doc.cache) {
    doc.cache = CachedAnalysis{};
  }

  doc.cache->version = doc.version;
  doc.cache->semanticTokens = std::move(result.semanticTokens);

  if (kind == AnalysisKind::Full) {
    doc.cache->diagnostics = std::move(result.diagnostics);
    doc.cache->fullAnalysisComplete = true;
  } else {
    doc.cache->fullAnalysisComplete = false;
  }
}

const CachedAnalysis& DocumentStore::getOrAnalyze(
    const path& filePath, AnalysisKind kind,
    const Shared<ImportLibrary>& importLibrary) {
  const auto it = documents.find(filePath);
  if (it == documents.end()) {
    throw std::out_of_range("DocumentStore::getOrAnalyze: unknown document");
  }

  ensureAnalysis(it->second, kind, importLibrary);
  return *it->second.cache;
}

}  // namespace vellum
