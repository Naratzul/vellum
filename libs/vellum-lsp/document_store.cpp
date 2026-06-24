#include "document_store.h"

#include <stdexcept>
#include <utility>

#include "common/fs.h"
#include "lsp_utf16.h"

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

void DocumentStore::applyChange(const path& filePath, const lsp::Range& range,
                                std::string replacementText) {
  const auto it = documents.find(filePath);
  if (it == documents.end()) {
    throw std::out_of_range("DocumentStore::applyChange: unknown document");
  }

  auto& doc = it->second;
  size_t start = utf8ByteOffsetAtLspPosition(doc.text, range.start);
  size_t end = utf8ByteOffsetAtLspPosition(doc.text, range.end);
  if (start > end) {
    std::swap(start, end);
  }

  doc.text.replace(start, end - start, replacementText);
  ++doc.version;
  doc.cache.reset();
}

void DocumentStore::close(const path& filePath) { documents.erase(filePath); }

bool DocumentStore::has(const path& filePath) const {
  return documents.contains(filePath);
}

std::string_view DocumentStore::text(const path& filePath) const {
  return documents.at(filePath).text;
}

std::string_view DocumentStore::scriptName(const path& filePath) const {
  return documents.at(filePath).scriptName;
}

void DocumentStore::ensureAnalysis(DocumentState& doc,
                                   const Shared<ImportLibrary>& importLibrary) {
  if (doc.cache && doc.cache->version == doc.version) {
    return;
  }

  AnalysisResult result =
      DocumentAnalyzer::run(doc.text, doc.scriptName, importLibrary);

  if (!doc.cache) {
    doc.cache = CachedAnalysis{};
  }

  doc.cache->version = doc.version;
  doc.cache->semanticTokens = std::move(result.semanticTokens);
  doc.cache->diagnostics = std::move(result.diagnostics);
  if (result.navigation) {
    doc.cache->navigation = std::move(*result.navigation);
  } else {
    doc.cache->navigation.reset();
  }
}

void DocumentStore::invalidateAll() {
  for (auto& [_, doc] : documents) {
    doc.cache.reset();
  }
}

const CachedAnalysis& DocumentStore::getOrAnalyze(
    const path& filePath, const Shared<ImportLibrary>& importLibrary) {
  const auto it = documents.find(filePath);
  if (it == documents.end()) {
    throw std::out_of_range("DocumentStore::getOrAnalyze: unknown document");
  }

  ensureAnalysis(it->second, importLibrary);
  return *it->second.cache;
}

}  // namespace vellum
