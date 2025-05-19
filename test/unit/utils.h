#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "lexer/ilexer.h"
#include "vellum/vellum_value.h"

namespace vellum {
Token makeToken(TokenType type, int line, std::string_view lexeme,
                std::optional<VellumLiteral> value = std::nullopt);
std::vector<Token> scanTokens(std::unique_ptr<ILexer> lexer);
}  // namespace vellum