#pragma once

#include <string>
#include <string_view>

#include "token.h"

namespace vellum {

class Lexer {
 public:
  explicit Lexer(const std::string& source);

  Token scanToken();

 private:
  const char* start;
  const char* current;
  int line;

  Token makeToken(TokenType type) const;
  Token errorToken(std::string_view message) const;
  Token string();
  Token number();
  Token identifier();

  TokenType identifierType() const;
  TokenType checkKeyword(int start, int length, const char* rest,
                         TokenType type) const;

  char advance();
  char peek();
  char peekNext();
  bool match(char expected);

  void skipWhitespaces();

  bool isAtEnd() const;
  bool isDigit(char c) const;
  bool isAlpha(char c) const;
};
}  // namespace vellum