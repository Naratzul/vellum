#pragma once

#include "lexer/ilexer.h"
#include <memory>

namespace vellum {
Token makeToken(TokenType type, int line, std::string_view lexeme,
                VellumValue value = VellumValue());
std::vector<Token> scanTokens(std::unique_ptr<ILexer> lexer);
}  // namespace vellum