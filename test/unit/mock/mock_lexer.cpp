#include "mock_lexer.h"

namespace vellum {
LexerMock::LexerMock(Vec<Token> tokens) : tokens(std::move(tokens)) {}
Token LexerMock::scanToken() { return tokens[currentToken++]; }
}  // namespace vellum