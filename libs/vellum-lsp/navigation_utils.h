#pragma once

#include <lsp/types.h>

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/types.h"
#include "compiler/resolver.h"
#include "declaration_index.h"
#include "document_analyzer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_type.h"

namespace vellum {

bool namesMatch(VellumIdentifier a, VellumIdentifier b);

Opt<VellumIdentifier> identifierFromType(const VellumType& type);

const Resolver* resolverForType(const Resolver& currentResolver, VellumType type,
                              const Shared<ImportLibrary>& importLibrary);

struct PropertyOwnerContext {
  VellumType objectType{VellumType::unresolved()};
  Opt<MemberKind> memberKind;
  bool isSuper{false};
  bool found{false};
};

PropertyOwnerContext findPropertyOwnerAtPosition(
    const ParserResult& parseResult, lsp::Position pos,
    Opt<VellumIdentifier> memberFilter);

Opt<MemberKind> memberKindForTypeMember(const Resolver& resolver,
                                        VellumType objectType,
                                        VellumIdentifier member,
                                        Opt<MemberKind> astHint);

Opt<lsp::DefinitionLink> definitionForTypeMember(
    const NavigationContext& navigation, const fs::path& filePath,
    const Token& originToken, VellumIdentifier member, VellumType objectType,
    MemberKind kind, const Shared<ImportLibrary>& importLibrary);

}  // namespace vellum
