#pragma once

#include "common/types.h"
#include "lexer/ilexer.h"

namespace vellum {
using common::Vec;

class LexerMock : public ILexer {
 public:
  explicit LexerMock(Vec<Token> tokens);
  Token scanToken() override;

 private:
  Vec<Token> tokens;
  std::size_t currentToken = 0;
};
}  // namespace vellum