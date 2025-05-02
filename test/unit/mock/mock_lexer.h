#pragma once

#include <vector>

#include "lexer/ilexer.h"

namespace vellum {
class LexerMock : public ILexer {
 public:
  explicit LexerMock(std::vector<Token> tokens);
  Token scanToken() override;

 private:
  std::vector<Token> tokens;
  std::size_t currentToken = 0;
};
}  // namespace vellum