#include "definitions_provider.h"

#include "analyze/import_library.h"
#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "ast_locator.h"
#include "compiler/resolver.h"
#include "declaration_index.h"
#include "document_analyzer.h"
#include "document_store.h"
#include "lsp_locations.h"
#include "navigation_utils.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace {

lsp::DefinitionLink makeDefinitionLink(const Token& originToken,
                                       const common::fs::path& targetPath,
                                       const Token& targetToken) {
  const lsp::Range targetRange = toLspRange(targetToken);
  return lsp::DefinitionLink{
      .targetUri = pathToUri(targetPath),
      .targetRange = targetRange,
      .targetSelectionRange = targetRange,
      .originSelectionRange = toLspRange(originToken),
  };
}

Opt<lsp::DefinitionLink> definitionInAst(const ParserResult& ast,
                                         const common::fs::path& filePath,
                                         const Token& originToken,
                                         VellumIdentifier name, MemberKind kind,
                                         Opt<VellumIdentifier> state = {}) {
  if (const auto token =
          DeclarationIndex::findMemberDeclaration(ast, name, kind, state)) {
    return makeDefinitionLink(originToken, filePath, *token);
  }
  return std::nullopt;
}

Opt<lsp::DefinitionLink> definitionInModule(const ImportModulePtr& module,
                                            const Token& originToken,
                                            VellumIdentifier name,
                                            MemberKind kind,
                                            Opt<VellumIdentifier> state = {}) {
  if (!module || !module->isParsed()) {
    return std::nullopt;
  }
  return definitionInAst(module->getAst(), module->getFilePath(), originToken,
                         name, kind, state);
}

struct FunctionDeclAtCursor {
  bool onFunctionName{false};
  Opt<VellumIdentifier> enclosingState;
};

class FunctionDeclAtCursorFinder : public ast::DeclarationVisitor {
 public:
  explicit FunctionDeclAtCursorFinder(lsp::Position pos) : pos(pos) {}

  FunctionDeclAtCursor result;

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      decl->accept(*this);
    }
  }

  void visitImportDeclaration(ast::ImportDeclaration&) override {}

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }

  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    const Opt<VellumIdentifier> savedState = enclosingState;
    enclosingState = VellumIdentifier(declaration.getStateName());
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
    enclosingState = savedState;
  }

  void visitVariableDeclaration(ast::GlobalVariableDeclaration&) override {}
  void visitPropertyDeclaration(ast::PropertyDeclaration&) override {}

  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (!declaration.getName() || !declaration.getNameLocation()) {
      return;
    }
    if (!positionInRange(pos, declaration.getNameLocation()->location)) {
      return;
    }
    result.onFunctionName = true;
    result.enclosingState = enclosingState;
  }

 private:
  lsp::Position pos;
  Opt<VellumIdentifier> enclosingState;
};

Opt<lsp::DefinitionLink> resolveIdentifierTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath, lsp::Position pos,
    const Shared<ImportLibrary>& importLibrary) {
  if (!target.identifier) {
    return std::nullopt;
  }

  const VellumIdentifier name = *target.identifier;
  const Resolver& resolver = *navigation.resolver;

  if (const auto local = DeclarationIndex::findLocalDeclaration(
          navigation.parseResult, pos, name)) {
    return makeDefinitionLink(target.token, filePath, *local);
  }

  if (const auto value = resolver.resolveIdentifier(name)) {
    switch (value->getType()) {
      case VellumValueType::Variable:
        return definitionInAst(navigation.parseResult, filePath, target.token,
                               name, MemberKind::GlobalVariable);
      case VellumValueType::Property:
        return definitionInAst(navigation.parseResult, filePath, target.token,
                               name, MemberKind::Property);
      case VellumValueType::Function:
        return definitionInAst(navigation.parseResult, filePath, target.token,
                               name, MemberKind::Function);
      case VellumValueType::ScriptType: {
        const Opt<VellumIdentifier> scriptName =
            identifierFromType(value->asScriptType());
        if (!scriptName) {
          break;
        }
        if (const auto currentScript =
                identifierFromType(resolver.getObjectType());
            currentScript && *scriptName == *currentScript) {
          return definitionInAst(navigation.parseResult, filePath, target.token,
                                 *scriptName, MemberKind::Script);
        }
        return definitionInModule(importLibrary->findModule(*scriptName),
                                  target.token, *scriptName,
                                  MemberKind::Script);
      }
      default:
        break;
    }
  }

  return std::nullopt;
}

Opt<lsp::DefinitionLink> resolvePropertyTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath,
    const Shared<ImportLibrary>& importLibrary, lsp::Position pos) {
  if (!target.identifier || !navigation.semanticOk) {
    return std::nullopt;
  }

  const VellumIdentifier member = *target.identifier;
  PropertyOwnerContext owner =
      findPropertyOwnerAtPosition(navigation.parseResult, pos, member);

  if (!owner.found) {
    return std::nullopt;
  }

  const Resolver& resolver = *navigation.resolver;
  VellumType objectType = owner.objectType;

  if (owner.isSuper) {
    if (const auto parentType = resolver.getParentType()) {
      objectType = *parentType;
    } else {
      return std::nullopt;
    }
    if (resolver.resolveParentFunction(member)) {
      return definitionForTypeMember(navigation, filePath, target.token, member,
                                     objectType, MemberKind::Function,
                                     importLibrary);
    }
  }

  const Opt<MemberKind> kind =
      memberKindForTypeMember(resolver, objectType, member, owner.memberKind);
  if (!kind) {
    return std::nullopt;
  }

  return definitionForTypeMember(navigation, filePath, target.token, member,
                                 objectType, *kind, importLibrary);
}

Opt<lsp::DefinitionLink> resolveDeclNameTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath,
    const Shared<ImportLibrary>& importLibrary, lsp::Position pos) {
  if (!target.identifier) {
    return std::nullopt;
  }

  const VellumIdentifier name = *target.identifier;
  FunctionDeclAtCursorFinder declFinder(pos);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          navigation.parseResult.declarations);
  declFinder.collect(declarations);

  if (!declFinder.result.onFunctionName) {
    if (const auto token = DeclarationIndex::findMemberDeclaration(
            navigation.parseResult, name, MemberKind::State)) {
      return makeDefinitionLink(target.token, filePath, *token);
    }
    if (const auto token = DeclarationIndex::findMemberDeclaration(
            navigation.parseResult, name, MemberKind::Property)) {
      return makeDefinitionLink(target.token, filePath, *token);
    }
    if (const auto token = DeclarationIndex::findMemberDeclaration(
            navigation.parseResult, name, MemberKind::GlobalVariable)) {
      return makeDefinitionLink(target.token, filePath, *token);
    }
    return makeDefinitionLink(target.token, filePath, target.token);
  }

  const Opt<VellumIdentifier> enclosingState = declFinder.result.enclosingState;

  if (enclosingState) {
    if (const auto rootDecl =
            definitionInAst(navigation.parseResult, filePath, target.token,
                            name, MemberKind::Function, /*state=*/{})) {
      return rootDecl;
    }
  }

  if (navigation.semanticOk) {
    const Resolver& resolver = *navigation.resolver;
    if (const auto parentType = resolver.getParentType()) {
      if (parentType->getState() == VellumTypeState::Identifier &&
          resolver.resolveParentFunction(name)) {
        return definitionForTypeMember(navigation, filePath, target.token, name,
                                       *parentType, MemberKind::Function,
                                       importLibrary);
      }
    }
  }

  return definitionInAst(navigation.parseResult, filePath, target.token, name,
                         MemberKind::Function, enclosingState);
}

Opt<lsp::DefinitionLink> resolveCallCalleeTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath,
    const Shared<ImportLibrary>& importLibrary) {
  if (!target.identifier || !navigation.semanticOk) {
    return std::nullopt;
  }

  const VellumIdentifier name = *target.identifier;
  const Resolver& resolver = *navigation.resolver;
  const VellumType objectType = resolver.getObjectType();

  if (!resolver.resolveFunction(objectType, name)) {
    return std::nullopt;
  }

  return definitionForTypeMember(navigation, filePath, target.token, name,
                                 objectType, MemberKind::Function,
                                 importLibrary);
}

Opt<lsp::DefinitionLink> resolveScriptTypeTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath,
    const Shared<ImportLibrary>& importLibrary) {
  if (!target.identifier || !navigation.resolver) {
    return std::nullopt;
  }

  VellumIdentifier scriptId = *target.identifier;
  if (const auto scriptType =
          navigation.resolver->resolveScriptType(*target.identifier)) {
    if (const auto resolvedId = identifierFromType(*scriptType)) {
      scriptId = *resolvedId;
    }
  }

  if (const auto currentScript =
          identifierFromType(navigation.resolver->getObjectType());
      currentScript && scriptId == *currentScript) {
    if (const auto link = definitionInAst(navigation.parseResult, filePath,
                                          target.token, scriptId,
                                          MemberKind::Script)) {
      return link;
    }
  }

  return definitionInModule(importLibrary->findModule(scriptId), target.token,
                            scriptId, MemberKind::Script);
}

}  // namespace

lsp::Array<lsp::DefinitionLink> DefinitionsProvider::getDefinitions(
    const path& filePath, lsp::Position pos, DocumentStore& store,
    const Shared<ImportLibrary>& importLibrary) {
  const CachedAnalysis& cache = store.getOrAnalyze(filePath, importLibrary);

  if (!cache.navigation || !cache.navigation->parseOk) {
    return {};
  }

  const NavigationContext& navigation = *cache.navigation;
  const Opt<AstLocatorTarget> target =
      AstLocator::locate(navigation.parseResult, pos);
  if (!target) {
    return {};
  }

  Opt<lsp::DefinitionLink> link;

  switch (target->kind) {
    case AstLocatorTargetKind::ImportName: {
      if (!target->identifier) {
        break;
      }
      link = definitionInModule(importLibrary->findModule(*target->identifier),
                                target->token, *target->identifier,
                                MemberKind::Script);
      break;
    }
    case AstLocatorTargetKind::ScriptName:
    case AstLocatorTargetKind::ParentScriptName:
    case AstLocatorTargetKind::TypeReference:
      link = resolveScriptTypeTarget(*target, navigation, filePath,
                                     importLibrary);
      break;
    case AstLocatorTargetKind::IdentifierUse:
      link = resolveIdentifierTarget(*target, navigation, filePath, pos,
                                     importLibrary);
      break;
    case AstLocatorTargetKind::CallCallee:
      link =
          resolveCallCalleeTarget(*target, navigation, filePath, importLibrary);
      break;
    case AstLocatorTargetKind::PropertyUse:
      link = resolvePropertyTarget(*target, navigation, filePath, importLibrary,
                                   pos);
      break;
    case AstLocatorTargetKind::DeclName:
      link = resolveDeclNameTarget(*target, navigation, filePath, importLibrary,
                                   pos);
      break;
  }

  if (!link) {
    return {};
  }

  lsp::Array<lsp::DefinitionLink> result;
  result.push_back(*link);
  return result;
}

}  // namespace vellum
