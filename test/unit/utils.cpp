#include "utils.h"

namespace vellum {
Token makeToken(TokenType type, int line, std::string_view lexeme,
                Opt<VellumLiteral> value) {
  Token token;
  token.type = type;
  token.lexeme = lexeme;
  token.location = {.start = {.line = line, .position = 0},
                    .end = {.line = line, .position = 0}};
  token.value = value;
  return token;
}

Vec<Token> scanTokens(Unique<ILexer> lexer) {
  Vec<Token> tokens;

  for (;;) {
    Token token = lexer->scanToken();
    if (token.type == TokenType::END_OF_FILE) {
      break;
    }
    tokens.push_back(token);
  }

  return tokens;
}
}  // namespace vellum