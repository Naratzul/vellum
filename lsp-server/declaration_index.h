#pragma once

#include "common/types.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "vellum/vellum_identifier.h"

namespace vellum {

enum class MemberKind { Function, Property, GlobalVariable, State, Script };

class DeclarationIndex {
 public:
  static Opt<Token> findMemberDeclaration(const ParserResult& ast,
                                          VellumIdentifier name, MemberKind kind,
                                          Opt<VellumIdentifier> state = {});
};

}  // namespace vellum
