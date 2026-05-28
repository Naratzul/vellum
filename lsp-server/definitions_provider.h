#pragma once

#include <lsp/messages.h>

#include <string>
#include <string_view>

#include "common/fs.h"
#include "common/types.h"

namespace vellum {
using common::Shared;
using common::fs::path;

class DocumentStore;
class ImportLibrary;

class DefinitionsProvider {
 public:
  lsp::Array<lsp::DefinitionLink> getDefinitions(
      const path& filePath, std::string_view scriptName, std::string_view text,
      lsp::Position pos, DocumentStore& store,
      const Shared<ImportLibrary>& importLibrary);
};
}  // namespace vellum