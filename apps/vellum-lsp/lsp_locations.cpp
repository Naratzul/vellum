#include "lsp_locations.h"

#include <lsp/fileuri.h>

namespace vellum {

lsp::Position toLsp(const Location& loc) {
  return lsp::Position{.line = static_cast<unsigned int>(loc.line),
                       .character = static_cast<unsigned int>(loc.position)};
}

lsp::Range toLspRange(const LocationRange& range) {
  return lsp::Range{.start = toLsp(range.start), .end = toLsp(range.end)};
}

lsp::Range toLspRange(const Token& token) { return toLspRange(token.location); }

bool positionBefore(lsp::Position a, lsp::Position b) {
  if (a.line != b.line) {
    return a.line < b.line;
  }
  return a.character < b.character;
}

bool locationBefore(const Location& loc, lsp::Position pos) {
  return positionBefore(toLsp(loc), pos);
}

bool tokenEndsBeforePosition(const Token& token, lsp::Position pos) {
  if (token.lexeme.empty()) {
    return false;
  }
  return locationBefore(token.location.end, pos);
}

bool positionInRange(lsp::Position pos, const LocationRange& range) {
  const int line = static_cast<int>(pos.line);
  const int character = static_cast<int>(pos.character);

  if (line < range.start.line || line > range.end.line) {
    return false;
  }
  if (line == range.start.line && character < range.start.position) {
    return false;
  }
  if (line == range.end.line && character > range.end.position) {
    return false;
  }
  return true;
}

lsp::DocumentUri pathToUri(const common::fs::path& filePath) {
  return lsp::DocumentUri::fromPath(filePath.string());
}

}  // namespace vellum
