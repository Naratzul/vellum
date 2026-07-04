#pragma once

#include "common/types.h"
#include "lexer/ilexer.h"
#include "lexer/token.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_modifier.h"
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

inline const ParsedModifiers noParsedModifiers{};
inline const ParsedModifiers staticParsedModifiers{
    {VellumModifier::Static, Token{}}};
inline const ParsedModifiers nativeParsedModifiers{
    {VellumModifier::Native, Token{}}};
inline const ParsedModifiers staticNativeParsedModifiers{
    {VellumModifier::Static, Token{}}, {VellumModifier::Native, Token{}}};
inline const ParsedModifiers autoParsedModifiers{{VellumModifier::Auto, Token{}}};
inline const ParsedModifiers hiddenParsedModifiers{
    {VellumModifier::Hidden, Token{}}};
inline const ParsedModifiers conditionalParsedModifiers{
    {VellumModifier::Conditional, Token{}}};
inline const ParsedModifiers hiddenConditionalParsedModifiers{
    {VellumModifier::Hidden, Token{}},
    {VellumModifier::Conditional, Token{}}};

Token makeToken(TokenType type, int line, std::string_view lexeme,
                Opt<VellumLiteral> value = std::nullopt);
Vec<Token> scanTokens(Unique<ILexer> lexer);
}  // namespace vellum