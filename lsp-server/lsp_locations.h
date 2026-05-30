#pragma once

#include <lsp/types.h>

#include "common/fs.h"
#include "lexer/token.h"

namespace vellum {

lsp::Position toLsp(const Location& loc);
lsp::Range toLspRange(const LocationRange& range);
lsp::Range toLspRange(const Token& token);

bool positionInRange(lsp::Position pos, const LocationRange& range);
bool positionBefore(lsp::Position a, lsp::Position b);
bool locationBefore(const Location& loc, lsp::Position pos);
bool tokenEndsBeforePosition(const Token& token, lsp::Position pos);

lsp::DocumentUri pathToUri(const common::fs::path& filePath);

}  // namespace vellum
