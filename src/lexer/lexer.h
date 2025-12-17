#pragma once

#include <string_view>

#include "ilexer.h"

namespace vellum {

class Lexer : public ILexer {
 public:
  explicit Lexer(std::string_view source);

  Token scanToken() override;

 private:
  const char* start;
  const char* current;
  int line = 0;
  int position = 0;

  Token makeToken(TokenType type,
                  common::Opt<VellumLiteral> value = std::nullopt) const;
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

  Token parseInt() const;
  Token parseFloat() const;

  std::string_view currentLexeme() const;
};
}  // namespace vellum