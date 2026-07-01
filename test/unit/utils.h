#pragma once

#include "common/types.h"
#include "lexer/ilexer.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Opt;
using common::Unique;
using common::Vec;

inline constexpr VellumModifiers noFunctionModifiers{};
inline constexpr VellumModifiers staticFunctionModifier{VellumModifier::Static};
inline constexpr VellumModifiers nativeFunctionModifier{VellumModifier::Native};
inline const auto staticNativeFunctionModifier =
    VellumModifiers{VellumModifier::Static} |
    VellumModifiers{VellumModifier::Native};

Token makeToken(TokenType type, int line, std::string_view lexeme,
                Opt<VellumLiteral> value = std::nullopt);
Vec<Token> scanTokens(Unique<ILexer> lexer);
}  // namespace vellum