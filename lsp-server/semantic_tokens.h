#pragma once

#include <lsp/types.h>

#include <string_view>

namespace vellum {

// Legend indices must match semanticTokensProvider.legend in server.cpp.
enum class SemanticTokenLegendType : unsigned int {
  Keyword = 0,
  String = 1,
  Number = 2,
  Operator = 3,
  Variable = 4,
  Class = 5,
  Type = 6,
  Function = 7,
  Property = 8,
  Parameter = 9,
};

enum class SemanticTokenLegendModifier : unsigned int {
  Declaration = 1u << 0,
  Readonly = 1u << 1,
  Static = 1u << 2,
};

class SemanticTokensBuilder {
 public:
  lsp::SemanticTokens build(std::string_view sourceCode) const;
};

}  // namespace vellum
