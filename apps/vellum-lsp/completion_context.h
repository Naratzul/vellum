#pragma once

#include <lsp/messages.h>
#include <lsp/types.h>

#include <string>
#include <string_view>

#include "document_store.h"

namespace vellum {

enum class CompletionContextKind {
  MemberAccess,
  Expression,
  ImportModule,
  ScriptName,
  ParentType,
  TypeAnnotation,
  Keyword,
};

struct PrefixAtCursor {
  lsp::Range replaceRange;
  std::string prefix;
  bool afterDot{false};
};

PrefixAtCursor prefixAtPosition(std::string_view source, lsp::Position pos);

std::string_view lineAt(std::string_view source, unsigned lineIndex);

struct CompletionContext {
  CompletionContextKind kind{CompletionContextKind::Keyword};
};

CompletionContext detectContext(
    std::string_view source,
    const lsp::requests::TextDocument_Completion::Params& params,
    const CachedAnalysis& cache, const PrefixAtCursor& prefix);

}  // namespace vellum
