#pragma once

#include <lsp/types.h>

#include <string_view>

#include "common/flags.h"
#include "parser/parser.h"

namespace vellum {

// Legend indices must match semanticTokensProvider.legend in server.cpp.
enum class SemanticTokenLegendType {
  Keyword,
  String,
  Number,
  Operator,
  Variable,
  Class,
  Type,
  Function,
  Property,
  Parameter,
};

enum class SemanticTokenLegendModifierBits {
  Declaration = 1u << 0,
  Readonly = 1u << 1,
  Static = 1u << 2,
};

using SemanticTokenLegendModifier =
    common::Flags<SemanticTokenLegendModifierBits>;

lsp::SemanticTokens buildSemanticTokensFromParse(ParserResult& parseResult,
                                                 std::string_view source);
lsp::SemanticTokens buildSemanticTokensLexerFallback(std::string_view source);

}  // namespace vellum
