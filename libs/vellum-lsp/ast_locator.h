#pragma once

#include <lsp/types.h>

#include "common/types.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "vellum/vellum_identifier.h"

namespace vellum {

enum class AstLocatorTargetKind {
  IdentifierUse,
  PropertyUse,
  ImportName,
  ScriptName,
  ParentScriptName,
  TypeReference,
  DeclName,
  CallCallee,
};

struct AstLocatorTarget {
  AstLocatorTargetKind kind;
  Token token;
  Opt<VellumIdentifier> identifier;
};

class AstLocator {
 public:
  static Opt<AstLocatorTarget> locate(const ParserResult& parseResult,
                                      lsp::Position pos);
};

}  // namespace vellum
