#include "definitions_provider.h"

namespace vellum {
lsp::Array<lsp::DefinitionLink> DefinitionsProvider::getDefinitions(
    lsp::Position pos) {
  return lsp::Array<lsp::DefinitionLink>();
}
}  // namespace vellum