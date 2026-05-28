#pragma once

#include <lsp/messages.h>

#include "common/types.h"
#include "document_store.h"

namespace vellum {

class Diagnostics {
 public:
  static lsp::requests::TextDocument_Diagnostic::Result fromCache(
      const CachedAnalysis& analysis);
};
}  // namespace vellum