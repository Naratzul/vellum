#include "papyrus_lexer.h"

#include <cctype>
#include <charconv>

#include "common/string_set.h"
#include "lexer/token.h"

#ifdef __APPLE__
#include <cstdlib>
#endif

namespace vellum {

PapyrusLexer::PapyrusLexer(std::string_view source)
    : start(source.data()), current(source.data()) {}

Token PapyrusLexer::scanToken() {
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
    case '\\':
      // Handle line continuation: backslash followed by optional whitespace and
      // newline
      advance();  // consume backslash
      // Skip whitespace (but not newlines) after backslash
      while (peek() == ' ' || peek() == '\t' || peek() == '\r') {
        advance();
      }
      // Now check for newline
      if (peek() == '\n') {
        advance();  // consume newline
        line++;
        position = 0;
        return scanToken();  // recursively scan next token
      }
      return errorToken("Unexpected character '\\'.");
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
    case '&':
      if (match('&')) {
        return makeToken(TokenType::AND);
      }
      break;
    case '|':
      if (match('|')) {
        return makeToken(TokenType::AND);
      }
      break;
    case '"':
      return string();
  }

  return errorToken(
      common::StringSet::insert(std::format("Unexpected character '{}'.", c)));
}

Token PapyrusLexer::makeToken(TokenType type,
                              common::Opt<VellumLiteral> value) const {
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

Token PapyrusLexer::errorToken(std::string_view message) const {
  Token token;
  token.type = TokenType::ERROR;
  token.lexeme = message;
  token.location = {
      .start = {.line = line,
                .position = position - (int)token.lexeme.length() - 1},
      .end = {.line = line, .position = position}};
  return token;
}

Token PapyrusLexer::string() {
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

  advance();
  return makeToken(TokenType::STRING,
                   std::string_view(start + 1, current - start - 2));
}

Token PapyrusLexer::number() {
  while (isDigit(peek())) advance();

  bool isFloat = false;

  if (peek() == '.' && isDigit(peekNext())) {
    isFloat = true;
    advance();
    while (isDigit(peek())) advance();
  }

  return isFloat ? parseFloat() : parseInt();
}

Token PapyrusLexer::identifier() {
  while (isAlphaNumeric(peek())) advance();

  const TokenType type = identifierType();
  switch (type) {
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

TokenType PapyrusLexer::identifierType() const {
  switch (std::tolower(start[0])) {
    case 'a':
      if (current - start > 1) {
        switch (std::tolower(start[1])) {
          case 's':
            return checkKeyword(2, 0, "", TokenType::AS);
          case 'u':
            if (checkKeyword(2, 2, "to", TokenType::AUTO) == TokenType::AUTO) {
              return TokenType::AUTO;
            }
            return checkKeyword(2, 10, "toreadonly", TokenType::AUTOREADONLY);
        }
      }
      break;
    case 'e':
      if (current - start > 1) {
        switch (std::tolower(start[1])) {
          case 'v':
            return checkKeyword(2, 3, "ent", TokenType::EVENT);
          case 'l':
            return checkKeyword(2, 2, "se", TokenType::ELSE);
          case 'x':
            return checkKeyword(2, 5, "tends", TokenType::EXTENDS);
          case 'n':
            if (checkKeyword(2, 5, "devent", TokenType::ENDEVENT) ==
                TokenType::ENDEVENT) {
              return TokenType::ENDEVENT;
            }
            if (checkKeyword(2, 9, "dfunction", TokenType::ENDFUNCTION) == TokenType::ENDFUNCTION) {
              return TokenType::ENDFUNCTION;
            }
            return checkKeyword(2, 9, "dproperty", TokenType::ENDPROPERTY);
        }
      }
      break;
    case 'f':
      if (current - start > 1) {
        switch (std::tolower(start[1])) {
          case 'a':
            return checkKeyword(2, 3, "lse", TokenType::FALSE);
          case 'u':
            return checkKeyword(2, 6, "nction", TokenType::FUNCTION);
        }
      }
      break;
    case 'g':
      return checkKeyword(1, 5, "lobal", TokenType::GLOBAL);
    case 'h':
      return checkKeyword(1, 5, "idden", TokenType::HIDDEN);
    case 'i':
      if (current - start > 1) {
        switch (std::tolower(start[1])) {
          case 'f':
            return checkKeyword(2, 0, "", TokenType::IF);
          case 'm':
            return checkKeyword(2, 4, "port", TokenType::IMPORT);
        }
      }
      break;
    case 'n':
      if (current - start > 1) {
        switch (std::tolower(start[1])) {
          case 'a':
            return checkKeyword(2, 4, "tive", TokenType::NATIVE);
          case 'o':
            if (current - start > 2) {
              switch (std::tolower(start[2])) {
                case 't':
                  return checkKeyword(3, 0, "", TokenType::NOT);
                case 'n':
                  return checkKeyword(3, 1, "e", TokenType::NONE);
              }
            }
            break;
        }
      }
      break;
    case 'p':
      return checkKeyword(1, 7, "roperty", TokenType::PROPERTY);
    case 'r':
      return checkKeyword(1, 5, "eturn", TokenType::RETURN);
    case 's':
      return checkKeyword(1, 9, "criptname", TokenType::SCRIPTNAME);
    case 't':
      return checkKeyword(1, 3, "rue", TokenType::TRUE);
    case 'v':
      return checkKeyword(1, 2, "ar", TokenType::VAR);
    case 'w':
      return checkKeyword(1, 4, "hile", TokenType::WHILE);
  }

  return TokenType::IDENTIFIER;
}

TokenType PapyrusLexer::checkKeyword(int start, int length, const char* rest,
                                     TokenType type) const {
  if (current - this->start == start + length) {
    // Case-insensitive comparison for Papyrus keywords
    for (int i = 0; i < length; i++) {
      if (std::tolower(this->start[start + i]) != std::tolower(rest[i])) {
        return TokenType::IDENTIFIER;
      }
    }
    return type;
  }
  return TokenType::IDENTIFIER;
}

char PapyrusLexer::advance() {
  position++;
  return *current++;
}

char PapyrusLexer::peek() { return *current; }

char PapyrusLexer::peekNext() {
  if (isAtEnd()) return '\0';
  return *(current + 1);
}

bool PapyrusLexer::match(char expected) {
  if (isAtEnd()) return false;
  if (*current != expected) return false;
  current++;
  position++;
  return true;
}

void PapyrusLexer::skipWhitespaces() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\\':
        // Check for line continuation: backslash followed by newline
        if (peekNext() == '\n') {
          advance();  // consume backslash
          advance();  // consume newline
          line++;
          position = 0;
        } else {
          return;
        }
        break;
      case '\n':
        line++;
        position = 0;
        advance();
        break;
      case ';':
        skipLineComment();
        break;
      case '{':
        skipMultilineComment();
        break;
      default:
        return;
    }
  }
}

void PapyrusLexer::skipLineComment() {
  // Papyrus uses ; for line comments, goes until end of line
  while (peek() != '\n' && !isAtEnd()) advance();
}

void PapyrusLexer::skipMultilineComment() {
  // Papyrus uses {} for multiline comments
  advance();  // consume '{'
  while (!isAtEnd()) {
    if (peek() == '\n') {
      line++;
      position = 0;
    }
    if (peek() == '}') {
      advance();  // consume '}'
      return;
    }
    advance();
  }
  // Unterminated comment - will be handled as error by caller
}

bool PapyrusLexer::isAtEnd() const { return *current == '\0'; }

bool PapyrusLexer::isDigit(char c) const { return c >= '0' && c <= '9'; }

bool PapyrusLexer::isAlpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool PapyrusLexer::isAlphaNumeric(char c) const {
  return isAlpha(c) || isDigit(c);
}

Token PapyrusLexer::parseInt() const {
  const std::string_view lexeme = currentLexeme();
  int32_t value;
  auto [ptr, ec] =
      std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value);
  if (ec == std::errc{} && ptr == lexeme.data() + lexeme.size()) {
    return makeToken(TokenType::INT, value);
  }

  return errorToken("Could not parse a number.");
}

Token PapyrusLexer::parseFloat() const {
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

std::string_view PapyrusLexer::currentLexeme() const {
  return std::string_view(start, current - start);
}

}  // namespace vellum
