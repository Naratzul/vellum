#include "lexer.h"

#include <charconv>

#include "common/string_set.h"

#ifdef __APPLE__
#include <cstdlib>
#endif

namespace vellum {

Lexer::Lexer(std::string_view source)
    : start(source.data()), current(source.data()) {}

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
    case '[':
      return makeToken(TokenType::LEFT_BRACK);
    case ']':
      return makeToken(TokenType::RIGHT_BRACK);
    case ':':
      return makeToken(TokenType::COLON);
    case ';':
      return makeToken(TokenType::SEMICOLON);
    case ',':
      return makeToken(TokenType::COMMA);
    case '.':
      return makeToken(TokenType::DOT);
    case '-':
      return makeToken(match('>') ? TokenType::ARROW : TokenType::MINUS);
    case '+':
      return makeToken(TokenType::PLUS);
    case '/':
      return makeToken(TokenType::SLASH);
    case '*':
      return makeToken(TokenType::STAR);
    case '%':
      return makeToken(TokenType::PERCENT);
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

  return errorToken(
      common::StringSet::insert(std::format("Unexpected character '{}'.", c)));
}

Token Lexer::makeToken(TokenType type, common::Opt<VellumLiteral> value) const {
  Token token;
  token.type = type;
  token.lexeme = currentLexeme();
  token.location = {
      .start = {.line = line,
                .position = position - (int)token.lexeme.length() - 1},
      .end = {.line = line, .position = position - 1}};
  token.value = value;
  return token;
}

Token Lexer::errorToken(std::string_view message) const {
  Token token;
  token.type = TokenType::ERROR;
  token.lexeme = message;
  token.location = {
      .start = {.line = line,
                .position = position - (int)token.lexeme.length() - 1},
      .end = {.line = line, .position = position}};
  return token;
}

Token Lexer::string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      line++;
      position = 0;
    }
    advance();
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string.");
  }

  // the closing quote.
  advance();
  return makeToken(
      TokenType::STRING,
      std::string_view(start + 1, current - start - 2));  // drop quotes
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
    case TokenType::NONE:
      return makeToken(type, VellumLiteral());
    case TokenType::FALSE:
      return makeToken(type, false);
    case TokenType::TRUE:
      return makeToken(type, true);
    case TokenType::IDENTIFIER:
      return makeToken(type);
    default:
      break;
  }

  return makeToken(type);
}

TokenType Lexer::identifierType() const {
  switch (start[0]) {
    case 'a':
      switch (start[1]) {
        case 'n':
          return checkKeyword(2, 1, "d", TokenType::AND);
        case 's':
          return checkKeyword(2, 0, "", TokenType::AS);
        case 'u':
          return checkKeyword(2, 2, "to", TokenType::AUTO);
      }
      break;
    case 'e':
      if (current - start > 1) {
        switch (start[1]) {
          case 'v':
            return checkKeyword(2, 3, "ent", TokenType::EVENT);
          case 'l':
            return checkKeyword(2, 2, "se", TokenType::ELSE);
        }
      }
      break;
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
    case 'g':
      return checkKeyword(1, 2, "et", TokenType::GET);
    case 'i':
      if (current - start > 1) {
        switch (start[1]) {
          case 'f':
            return checkKeyword(2, 0, "", TokenType::IF);
          case 'm':
            return checkKeyword(2, 4, "port", TokenType::IMPORT);
        }
      }
      break;
    case 'n':
      if (current - start > 2 && start[1] == 'o') {
        switch (start[2]) {
          case 'n':
            return checkKeyword(3, 1, "e", TokenType::NONE);
          case 't':
            return checkKeyword(3, 0, "", TokenType::NOT);
        }
      }
      break;
    case 'o':
      return checkKeyword(1, 1, "r", TokenType::OR);
    case 'r':
      return checkKeyword(1, 5, "eturn", TokenType::RETURN);
    case 's':
      if (current - start > 1) {
        switch (start[1]) {
          case 'e':
            return checkKeyword(2, 1, "t", TokenType::SET);
          case 'c':
            return checkKeyword(2, 4, "ript", TokenType::SCRIPT);
          case 't':
            if (current - start > 2 && start[2] == 'a') {
              return checkKeyword(3, 2, "te", TokenType::STATE);
            }
            return checkKeyword(2, 4, "atic", TokenType::STATIC);
          case 'u':
            return checkKeyword(2, 3, "per", TokenType::SUPER);
        }
      }
      break;
    case 'm':
      return checkKeyword(1, 4, "atch", TokenType::MATCH);
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

char Lexer::advance() {
  position++;
  return *current++;
}

char Lexer::peek() { return *current; }

char Lexer::peekNext() {
  if (isAtEnd()) return '\0';
  return *(current + 1);
}

bool Lexer::match(char expected) {
  if (isAtEnd()) return false;
  if (*current != expected) return false;
  current++;
  position++;
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
        position = 0;
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
#ifdef __APPLE__
  value = strtof(lexeme.data(), nullptr);
  return makeToken(TokenType::FLOAT, value);
#else
  auto [ptr, ec] =
      std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value);
  if (ec == std::errc{} && ptr == lexeme.data() + lexeme.size()) {
    return makeToken(TokenType::FLOAT, value);
  }
#endif

  return errorToken("Could not parse a number.");
}

std::string_view Lexer::currentLexeme() const {
  return std::string_view(start, current - start);
}
}  // namespace vellum
