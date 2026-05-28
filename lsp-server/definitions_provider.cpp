#include "definitions_provider.h"

#include "document_store.h"

namespace vellum {

lsp::Array<lsp::DefinitionLink> DefinitionsProvider::getDefinitions(
    const path& filePath, std::string_view scriptName, std::string_view text,
    lsp::Position pos, DocumentStore& store,
    const Shared<ImportLibrary>& importLibrary) {
  (void)filePath;
  (void)scriptName;
  (void)text;
  (void)pos;
  (void)store;
  (void)importLibrary;
  return lsp::Array<lsp::DefinitionLink>();
}

}  // namespace vellum