#pragma once

#include <optional>
#include <ostream>
#include <string_view>

#include "vellum/vellum_literal.h"

namespace vellum {

enum class TokenType {
  // Single-character tokens.
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  COLON,
  SEMICOLON,
  SLASH,
  STAR,
  PERCENT,
  // One or two character tokens.
  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,
  ARROW,
  // Literals.
  IDENTIFIER,
  STRING,
  INT,
  FLOAT,
  // Keywords.
  AND,
  AS,
  ELSE,
  EVENT,
  FALSE,
  FOR,
  FUN,
  GET,
  IF,
  NIL,
  NOT,
  OR,
  PRINT,
  RETURN,
  SET,
  SCRIPT,
  SUPER,
  THIS,
  TRUE,
  VAR,
  WHILE,

  ERROR,
  END_OF_FILE
};

struct Token {
  TokenType type = TokenType::END_OF_FILE;
  std::string_view lexeme;
  int line = -1;
  std::optional<VellumLiteral> value;
};

inline bool operator==(const Token& lhs, const Token& rhs) {
  return lhs.type == rhs.type && lhs.line == rhs.line &&
         lhs.lexeme == rhs.lexeme && lhs.value == rhs.value;
}

inline bool operator!=(const Token& lhs, const Token& rhs) {
  return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Token& token) {
  os << "{" << (int)token.type << "; " << token.lexeme << "; " << token.line;
  if (token.value) {
    os << "; " << *token.value;
  }
  os << "}";
  return os;
}
}  // namespace vellum