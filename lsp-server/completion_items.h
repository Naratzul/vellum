#pragma once

#include <lsp/types.h>

#include <vector>

#include "completion_collectors.h"
#include "completion_context.h"

namespace vellum {

lsp::CompletionList toCompletionList(const Vec<CompletionCandidate>& candidates,
                                     const PrefixAtCursor& prefix,
                                     bool isIncomplete);

}  // namespace vellum
