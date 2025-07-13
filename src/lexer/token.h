#pragma once

#include <format>
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

struct Location {
  int line;
  int position;
};

inline bool operator==(const Location& lhs, const Location& rhs) {
  return lhs.line == rhs.line && lhs.position == rhs.position;
}

inline std::ostream& operator<<(std::ostream& os, const Location& loc) {
  os << std::format("({}, {})", loc.line, loc.position);
  return os;
}

struct LocationRange {
  Location start;
  Location end;
};

inline bool operator==(const LocationRange& lhs, const LocationRange& rhs) {
  return lhs.start == rhs.start && lhs.end == rhs.end;
}

inline std::ostream& operator<<(std::ostream& os, const LocationRange& loc) {
  os << loc.start << " - " << loc.end;
  return os;
}

struct Token {
  TokenType type = TokenType::END_OF_FILE;
  std::string_view lexeme;
  LocationRange location;
  std::optional<VellumLiteral> value;
};

inline bool operator==(const Token& lhs, const Token& rhs) {
  return lhs.type == rhs.type && lhs.location == rhs.location &&
         lhs.lexeme == rhs.lexeme && lhs.value == rhs.value;
}

inline bool operator!=(const Token& lhs, const Token& rhs) {
  return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Token& token) {
  os << "{" << (int)token.type << "; " << token.lexeme << "; "
     << token.location;
  if (token.value) {
    os << "; " << *token.value;
  }
  os << "}";
  return os;
}
}  // namespace vellum