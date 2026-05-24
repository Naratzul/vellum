#pragma once

#include <lsp/messages.h>

namespace vellum {
class DefinitionsProvider {
 public:
  lsp::Array<lsp::DefinitionLink> getDefinitions(lsp::Position pos);
};
}  // namespace vellum