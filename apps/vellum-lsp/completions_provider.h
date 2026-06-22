#pragma once

#include <lsp/messages.h>

#include "common/fs.h"
#include "common/types.h"

namespace vellum {
using common::Shared;
using common::fs::path;

class DocumentStore;
class ImportLibrary;

class CompletionsProvider {
 public:
  lsp::OneOf<lsp::Array<lsp::CompletionItem>, lsp::CompletionList> getCompletions(
      const path& filePath,
      const lsp::requests::TextDocument_Completion::Params& params,
      DocumentStore& store, const Shared<ImportLibrary>& importLibrary);
};

}  // namespace vellum
