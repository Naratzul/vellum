#include "lexer.h"

#include <charconv>
#include <string>

#include "common/string_set.h"
#include "token.h"

#ifdef __APPLE__
#include <cstdlib>
#endif

namespace vellum {

Lexer::Lexer(std::string_view source)
    : start(source.data()), current(source.data()) {
  frames.emplace_back(LexerMode::Normal);
}

Token Lexer::scanToken() {
  // Spaces inside $"..." are part of the string; do not skip them in text mode.
  if (frames.back().mode != LexerMode::InterpText) {
    skipWhitespaces();
  }
  start = current;

  if (isAtEnd()) {
    const LexerMode mode = frames.back().mode;
    if (mode == LexerMode::InterpText || mode == LexerMode::InterpExpr) {
      // Leave interp modes so the next scanToken can return EOF instead of
      // repeating the same ERROR forever.
      frames.clear();
      frames.emplace_back(LexerMode::Normal);
      return errorToken(mode == LexerMode::InterpText
                            ? "Unterminated interpolated string."
                            : "Unterminated interpolated expression.");
    }
    return makeToken(TokenType::END_OF_FILE);
  }

  switch (frames.back().mode) {
    case LexerMode::Normal:
      return scanNormalToken();
    case LexerMode::InterpText:
      return interpolatedText();
    case LexerMode::InterpExpr:
      return interpolatedExpression();
  }

  return errorToken("Invalid lexer mode.");
}

Token Lexer::scanNormalToken() {
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
      return makeToken(match('=')   ? TokenType::MINUS_EQUAL
                       : match('>') ? TokenType::ARROW
                                    : TokenType::MINUS);
    case '+':
      return makeToken(match('=') ? TokenType::PLUS_EQUAL : TokenType::PLUS);
    case '/':
      return makeToken(match('=') ? TokenType::SLASH_EQUAL : TokenType::SLASH);
    case '*':
      return makeToken(match('=') ? TokenType::STAR_EQUAL : TokenType::STAR);
    case '%':
      return makeToken(match('=') ? TokenType::PERCENT_EQUAL
                                  : TokenType::PERCENT);
    case '!':
      return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
    case '?':
      return makeToken(TokenType::QUES);
    case '=':
      if (match('=')) {
        return makeToken(TokenType::EQUAL_EQUAL);
      }
      if (match('>')) {
        return makeToken(TokenType::FAT_ARROW);
      }
      return makeToken(TokenType::EQUAL);
    case '<':
      return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
    case '>':
      return makeToken(match('=') ? TokenType::GREATER_EQUAL
                                  : TokenType::GREATER);
    case '&':
      if (match('&')) {
        return makeToken(TokenType::AND);
      }
      break;
    case '|':
      if (match('|')) {
        return makeToken(TokenType::OR);
      }
      return makeToken(TokenType::PIPE);
    case '"':
      return string();
    case '$':
      if (match('"')) {
        return startInterpolatedString();
      }
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
                .position = position - (int)token.lexeme.length()},
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
                .position = position - (int)token.lexeme.length()},
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

Token Lexer::startInterpolatedString() {
  frames.emplace_back(LexerMode::InterpText);
  return makeToken(TokenType::INTERP_START);
}

Token Lexer::endInterpolatedString() {
  frames.pop_back();
  return makeToken(TokenType::INTERP_END);
}

Token Lexer::interpolatedText() {
  const char c = peek();

  if (c == '"') {
    advance();
    return endInterpolatedString();
  }

  if (c == '{') {
    if (peekNext() != '{') {
      advance();
      frames.emplace_back(LexerMode::InterpExpr, 1);
      return makeToken(TokenType::LEFT_BRACE);
    }
  } else if (c == '}') {
    if (peekNext() != '}') {
      advance();
      return errorToken("Unexpected '}' in interpolated string.");
    }
  }

  std::string decoded;
  bool hasEscape = false;

  while (!isAtEnd()) {
    const char ch = peek();

    if (ch == '"') {
      break;
    }

    if (ch == '{') {
      if (peekNext() == '{') {
        hasEscape = true;
        advance();
        advance();
        decoded.push_back('{');
        continue;
      }
      break;
    }

    if (ch == '}') {
      if (peekNext() == '}') {
        hasEscape = true;
        advance();
        advance();
        decoded.push_back('}');
        continue;
      }
      break;
    }

    if (ch == '\n') {
      line++;
      position = 0;
    }

    decoded.push_back(ch);
    advance();
  }

  if (hasEscape) {
    return makeToken(TokenType::STRING_FRAGMENT,
                     common::StringSet::insert(decoded));
  }

  return makeToken(TokenType::STRING_FRAGMENT,
                   std::string_view(start, current - start));
}

Token Lexer::interpolatedExpression() {
  if (peek() == '{') {
    advance();
    frames.back().interpExprDepth++;
    return makeToken(TokenType::LEFT_BRACE);
  }

  if (peek() == '}') {
    advance();
    frames.back().interpExprDepth--;
    if (frames.back().interpExprDepth == 0) {
      frames.pop_back();
    }
    return makeToken(TokenType::RIGHT_BRACE);
  }

  return scanNormalToken();
}

TokenType Lexer::identifierType() const {
  switch (start[0]) {
    case 'a':
      switch (start[1]) {
        case 's':
          return checkKeyword(2, 0, "", TokenType::AS);
        case 'u':
          return checkKeyword(2, 2, "to", TokenType::AUTO);
      }
      break;
    case 'b':
      return checkKeyword(1, 4, "reak", TokenType::BREAK);
    case 'c':
      if (current - start > 3 && start[3] == 'd') {
        return checkKeyword(1, 10, "onditional", TokenType::CONDITIONAL);
      }
      return checkKeyword(1, 7, "ontinue", TokenType::CONTINUE);
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
    case 'h':
      return checkKeyword(1, 5, "idden", TokenType::HIDDEN);
    case 'i':
      if (current - start > 1) {
        switch (start[1]) {
          case 'f':
            return checkKeyword(2, 0, "", TokenType::IF);
          case 'm':
            return checkKeyword(2, 4, "port", TokenType::IMPORT);
          case 'n':
            return checkKeyword(2, 0, "", TokenType::IN);
        }
      }
      break;
    case 'n':
      if (current - start > 1) {
        switch (start[1]) {
          case 'a':
            return checkKeyword(2, 4, "tive", TokenType::NATIVE);
          case 'o':
            return checkKeyword(2, 2, "ne", TokenType::NONE);
        }
      }
      break;
    case 'r':
      return checkKeyword(1, 5, "eturn", TokenType::RETURN);
    case 's':
      if (current - start > 1) {
        switch (start[1]) {
          case 'e':
            if (current - start == 4 && compare(2, 2, "lf")) {
              return TokenType::SELF;
            }
            return checkKeyword(2, 1, "t", TokenType::SET);
          case 'c':
            return checkKeyword(2, 4, "ript", TokenType::SCRIPT);
          case 't':
            if (compare(2, 3, "ate")) {
              return TokenType::STATE;
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
      if (current - start > 1 && start[1] == 'r') {
        return checkKeyword(2, 2, "ue", TokenType::TRUE);
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
  return compare(start, length, rest) ? type : TokenType::IDENTIFIER;
}

bool Lexer::compare(int start, int length, const char* rest) const {
  return current - this->start == start + length &&
         memcmp(this->start + start, rest, length) == 0;
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
      case '\t':
        advance();
        break;
      case '\r':
        if (peekNext() == '\n') {
          line++;
          position = 0;
          current += 2;
        } else {
          line++;
          position = 0;
          current++;
        }
        break;
      case '\n':
        line++;
        position = 0;
        current++;
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
