#include "utils.h"

namespace vellum {
Token makeToken(TokenType type, int line, std::string_view lexeme,
                VellumValue value) {
  Token token;
  token.type = type;
  token.lexeme = lexeme;
  token.line = line;
  token.value = value;
  return token;
}

std::vector<Token> scanTokens(std::unique_ptr<ILexer> lexer) {
  std::vector<Token> tokens;

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