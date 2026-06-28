#pragma once

#include "common/types.h"
#include "lexer/ilexer.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Opt;
using common::Unique;
using common::Vec;

inline constexpr VellumFunctionModifier noFunctionModifiers{};
inline constexpr VellumFunctionModifier staticFunctionModifier{
    VellumFunctionModifierBits::Static};
inline constexpr VellumFunctionModifier nativeFunctionModifier{
    VellumFunctionModifierBits::Native};
inline const VellumFunctionModifier staticNativeFunctionModifier =
    VellumFunctionModifier{VellumFunctionModifierBits::Static} |
    VellumFunctionModifier{VellumFunctionModifierBits::Native};

Token makeToken(TokenType type, int line, std::string_view lexeme,
                Opt<VellumLiteral> value = std::nullopt);
Vec<Token> scanTokens(Unique<ILexer> lexer);
}  // namespace vellum