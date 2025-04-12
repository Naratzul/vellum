#include "lexer.h"

#include <charconv>

namespace vellum {

Lexer::Lexer(const std::string& source)
    : start(source.data()), current(source.data()), line(1) {}

Token Lexer::scanToken() {
  skipWhitespaces();
  start = current;

  if (isAtEnd()) {
    return makeToken(TokenType::END_OF_FILE);
  }

  const char c = advance();

  if (isDigit(c)) {
    return number();
  }

  if (isAlpha(c)) {
    return identifier();
  }

  switch (c) {
    case '(':
      return makeToken(TokenType::LEFT_PAREN);
    case ')':
      return makeToken(TokenType::RIGHT_PAREN);
    case '{':
      return makeToken(TokenType::LEFT_BRACE);
    case '}':
      return makeToken(TokenType::RIGHT_BRACE);
    case ':':
      return makeToken(TokenType::COLON);
    case ';':
      return makeToken(TokenType::SEMICOLON);
    case ',':
      return makeToken(TokenType::COMMA);
    case '.':
      return makeToken(TokenType::DOT);
    case '-':
      return makeToken(TokenType::MINUS);
    case '+':
      return makeToken(TokenType::PLUS);
    case '/':
      return makeToken(TokenType::SLASH);
    case '*':
      return makeToken(TokenType::STAR);
    case '!':
      return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
    case '=':
      return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
    case '<':
      return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
    case '>':
      return makeToken(match('=') ? TokenType::GREATER_EQUAL
                                  : TokenType::GREATER);
    case '"':
      return string();
  }

  return errorToken("Unexpected character.");
}

Token Lexer::makeToken(TokenType type, VellumValue value) const {
  Token token;
  token.type = type;
  token.lexeme = currentLexeme();
  token.line = line;
  token.value = value;
  return token;
}

Token Lexer::errorToken(std::string_view message) const {
  Token token;
  token.type = TokenType::ERROR;
  token.lexeme = message;
  token.line = line;
  return token;
}

Token Lexer::string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') line++;
    advance();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string.");
  }

  // the closing quote.
  advance();
  return makeToken(TokenType::STRING, currentLexeme());
}

Token Lexer::number() {
  while (isDigit(peek())) advance();

  bool isFloat = false;

  // look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    isFloat = true;

    // consume '.'.
    advance();

    while (isDigit(peek())) advance();
  }

  return isFloat ? parseFloat() : parseInt();
}

Token Lexer::identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();

  const TokenType type = identifierType();
  switch (type) {
    case TokenType::FALSE:
      return makeToken(type, false);
    case TokenType::TRUE:
      return makeToken(type, true);
    default:
      break;
  }

  return makeToken(identifierType());
}

TokenType Lexer::identifierType() const {
  switch (start[0]) {
    case 'a':
      return checkKeyword(1, 2, "nd", TokenType::AND);
    case 'e':
      return checkKeyword(1, 3, "lse", TokenType::ELSE);
    case 'f':
      if (current - start > 1) {
        switch (start[1]) {
          case 'a':
            return checkKeyword(2, 3, "lse", TokenType::FALSE);
          case 'o':
            return checkKeyword(2, 1, "r", TokenType::FOR);
          case 'u':
            return checkKeyword(2, 1, "n", TokenType::FUN);
        }
      }
      break;
    case 'i':
      return checkKeyword(1, 1, "f", TokenType::IF);
    case 'n':
      return checkKeyword(1, 2, "il", TokenType::NIL);
    case 'o':
      return checkKeyword(1, 1, "r", TokenType::OR);
    case 'p':
      return checkKeyword(1, 4, "rint", TokenType::PRINT);
    case 'r':
      return checkKeyword(1, 5, "eturn", TokenType::RETURN);
    case 's':
      if (current - start > 1) {
        switch (start[1]) {
          case 'c':
            return checkKeyword(2, 4, "ript", TokenType::SCRIPT);
          case 'u':
            return checkKeyword(2, 3, "per", TokenType::SUPER);
        }
      }
      break;
    case 't':
      if (current - start > 1) {
        switch (start[1]) {
          case 'h':
            return checkKeyword(2, 2, "is", TokenType::THIS);
          case 'r':
            return checkKeyword(2, 2, "ue", TokenType::TRUE);
        }
      }
      break;
    case 'v':
      return checkKeyword(1, 2, "ar", TokenType::VAR);
    case 'w':
      return checkKeyword(1, 4, "hile", TokenType::WHILE);
  }

  return TokenType::IDENTIFIER;
}

TokenType Lexer::checkKeyword(int start, int length, const char* rest,
                              TokenType type) const {
  if (current - this->start == start + length &&
      memcmp(this->start + start, rest, length) == 0) {
    return type;
  }
  return TokenType::IDENTIFIER;
}

char Lexer::advance() { return *current++; }

char Lexer::peek() { return *current; }

char Lexer::peekNext() {
  if (isAtEnd()) return '\0';
  return *(current + 1);
}

bool Lexer::match(char expected) {
  if (isAtEnd()) return false;
  if (*current != expected) return false;
  current++;
  return true;
}

void Lexer::skipWhitespaces() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // a comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

bool Lexer::isAtEnd() const { return *current == '\0'; }

bool Lexer::isDigit(char c) const { return c >= '0' && c <= '9'; }

bool Lexer::isAlpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

Token Lexer::parseInt() const {
  const std::string_view lexeme = currentLexeme();
  int32_t value;
  auto [ptr, ec] =
      std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value);
  if (ec == std::errc{} && ptr == lexeme.data() + lexeme.size()) {
    return makeToken(TokenType::INT, value);
  }

  return errorToken("Could not parse a number.");
}

Token Lexer::parseFloat() const { 
  const std::string_view lexeme = currentLexeme();
  float value;
  auto [ptr, ec] =
      std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value);
  if (ec == std::errc{} && ptr == lexeme.data() + lexeme.size()) {
    return makeToken(TokenType::FLOAT, value);
  }

  return errorToken("Could not parse a number.");
}

std::string_view Lexer::currentLexeme() const {
  return std::string_view(start, current - start);
}
}  // namespace vellum
