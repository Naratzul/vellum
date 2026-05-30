#include "definitions_provider.h"

#include "analyze/import_library.h"
#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "ast_locator.h"
#include "common/string_utils.h"
#include "compiler/resolver.h"
#include "declaration_index.h"
#include "document_analyzer.h"
#include "document_store.h"
#include "lsp_locations.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace {

Opt<VellumIdentifier> identifierFromType(const VellumType& type) {
  if (type.getState() != VellumTypeState::Identifier) {
    return std::nullopt;
  }
  return type.asIdentifier();
}

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

const Resolver* resolverForType(const Resolver& currentResolver,
                                VellumType type,
                                const Shared<ImportLibrary>& importLibrary) {
  if (type.getState() != VellumTypeState::Identifier) {
    return nullptr;
  }
  const VellumIdentifier typeId = type.asIdentifier();
  if (currentResolver.getObject().getType() == type) {
    return &currentResolver;
  }
  if (const Resolver* scriptResolver =
          currentResolver.findScriptResolver(typeId)) {
    return scriptResolver;
  }
  if (const auto module = importLibrary->findModule(typeId)) {
    if (module->getResolver()) {
      return module->getResolver().get();
    }
  }
  return nullptr;
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

struct PropertyOwnerContext {
  VellumType objectType{VellumType::unresolved()};
  Opt<MemberKind> memberKind;
  bool isSuper{false};
  bool found{false};
};

class PropertyObjectTypeFinder : public ast::DeclarationVisitor,
                                 public ast::StatementVisitor,
                                 public ast::ExpressionVisitor {
 public:
  PropertyObjectTypeFinder(lsp::Position pos, VellumIdentifier member)
      : pos(pos), member(member) {}

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      decl->accept(*this);
    }
  }

  PropertyOwnerContext result;

  void visitImportDeclaration(ast::ImportDeclaration&) override {}
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }
  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override {
    if (declaration.initializer()) {
      declaration.initializer()->accept(*this);
    }
  }
  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    declaration.getBody()->accept(*this);
  }
  void visitPropertyDeclaration(
      ast::PropertyDeclaration& declaration) override {
    if (const auto& getBlock = declaration.getGetAccessor()) {
      getBlock.value()->accept(*this);
    }
    if (const auto& setBlock = declaration.getSetAccessor()) {
      setBlock.value()->accept(*this);
    }
  }

  void visitExpressionStatement(ast::ExpressionStatement& statement) override {
    statement.getExpression()->accept(*this);
  }
  void visitReturnStatement(ast::ReturnStatement& statement) override {
    if (statement.getExpression()) {
      statement.getExpression()->accept(*this);
    }
  }
  void visitIfStatement(ast::IfStatement& statement) override {
    statement.getCondition()->accept(*this);
    statement.getThenBlock()->accept(*this);
    if (statement.getElseBlock()) {
      statement.getElseBlock()->accept(*this);
    }
  }
  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override {
    if (statement.getInitializer()) {
      statement.getInitializer()->accept(*this);
    }
  }
  void visitWhileStatement(ast::WhileStatement& statement) override {
    statement.getCondition()->accept(*this);
    statement.getBody()->accept(*this);
  }
  void visitBreakStatement(ast::BreakStatement&) override {}
  void visitContinueStatement(ast::ContinueStatement&) override {}
  void visitForStatement(ast::ForStatement& statement) override {
    statement.getArray()->accept(*this);
    statement.getBody()->accept(*this);
  }
  void visitBlockStatement(ast::BlockStatement& statement) override {
    for (const auto& stmt : statement.getStatements()) {
      stmt->accept(*this);
    }
  }

  void visitIdentifierExpression(ast::IdentifierExpression&) override {}
  void visitCallExpression(ast::CallExpression& expr) override {
    visitExpression(*expr.getCallee(), depth + 1);
    for (const auto& arg : expr.getArguments()) {
      visitExpression(*arg, depth + 1);
    }
  }
  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override {
    visitExpression(*expr.getObject(), depth + 1);
    considerPropertyGet(expr);
  }
  void visitPropertySetExpression(ast::PropertySetExpression& expr) override {
    expr.getObject()->accept(*this);
    expr.getValue()->accept(*this);
  }
  void visitAssignExpression(ast::AssignExpression& expr) override {
    expr.getName()->accept(*this);
    expr.getValue()->accept(*this);
  }
  void visitBinaryExpression(ast::BinaryExpression& expr) override {
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }
  void visitUnaryExpression(ast::UnaryExpression& expr) override {
    expr.getOperand()->accept(*this);
  }
  void visitCastExpression(ast::CastExpression& expr) override {
    expr.getExpression()->accept(*this);
  }
  void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override {
    expr.getArray()->accept(*this);
    expr.getIndex()->accept(*this);
  }
  void visitArrayIndexSetExpression(
      ast::ArrayIndexSetExpression& expr) override {
    expr.getArray()->accept(*this);
    expr.getIndex()->accept(*this);
    expr.getValue()->accept(*this);
  }
  void visitSelfExpression(ast::SelfExpression&) override {}
  void visitSuperExpression(ast::SuperExpression&) override {}
  void visitNewArrayExpression(ast::NewArrayExpression&) override {}
  void visitTernaryExpression(ast::TernaryExpression& expr) override {
    expr.getCondition()->accept(*this);
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }

 private:
  lsp::Position pos;
  VellumIdentifier member;
  int depth{0};
  int bestDepth{-1};

  void considerPropertyGet(ast::PropertyGetExpression& expr) {
    if (!positionInRange(pos, expr.getLocation().location)) {
      return;
    }
    if (common::normalizeToLower(expr.getProperty().toString()) !=
        common::normalizeToLower(member.toString())) {
      return;
    }
    if (!expr.getObject()->hasResolvedType()) {
      return;
    }
    if (depth < bestDepth) {
      return;
    }

    bestDepth = depth;
    result.isSuper = expr.getObject()->isSuperExpression();
    result.objectType = expr.getObject()->getType();
    result.memberKind = std::nullopt;
    if (expr.getIdentifierType() == VellumValueType::Function) {
      result.memberKind = MemberKind::Function;
    } else if (expr.getIdentifierType() == VellumValueType::Property) {
      result.memberKind = MemberKind::Property;
    }
    result.found = true;
  }

  void visitExpression(ast::Expression& expr, int childDepth) {
    const int saved = depth;
    depth = childDepth;
    expr.accept(*this);
    depth = saved;
  }
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
                identifierFromType(resolver.getObject().getType());
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

common::fs::path pathForResolver(const Resolver& resolver,
                                 const common::fs::path& currentFilePath,
                                 const Shared<ImportLibrary>& importLibrary) {
  if (resolver.getObject().getType().getState() ==
      VellumTypeState::Identifier) {
    const VellumIdentifier scriptId =
        resolver.getObject().getType().asIdentifier();
    if (const auto module = importLibrary->findModule(scriptId)) {
      return module->getFilePath();
    }
  }
  return currentFilePath;
}

Opt<MemberKind> memberKindForTypeMember(const Resolver& resolver,
                                        VellumType objectType,
                                        VellumIdentifier member,
                                        Opt<MemberKind> astHint) {
  if (astHint) {
    return astHint;
  }

  if (const auto value = resolver.resolveProperty(objectType, member)) {
    switch (value->getType()) {
      case VellumValueType::Function:
        return MemberKind::Function;
      case VellumValueType::Property:
        return MemberKind::Property;
      default:
        break;
    }
  }

  if (resolver.resolveFunction(objectType, member)) {
    return MemberKind::Function;
  }

  return std::nullopt;
}

Opt<lsp::DefinitionLink> definitionForTypeMember(
    const NavigationContext& navigation, const common::fs::path& filePath,
    const Token& originToken, VellumIdentifier member, VellumType objectType,
    MemberKind kind, const Shared<ImportLibrary>& importLibrary) {
  if (objectType.getState() != VellumTypeState::Identifier) {
    return std::nullopt;
  }

  const Resolver& resolver = *navigation.resolver;
  const Opt<VellumIdentifier> currentScript =
      identifierFromType(resolver.getObject().getType());

  VellumType curType = objectType;
  for (int depth = 0; depth < 64; ++depth) {
    if (curType.getState() != VellumTypeState::Identifier) {
      break;
    }
    const VellumIdentifier scriptId = curType.asIdentifier();

    if (currentScript && *currentScript == scriptId) {
      if (const auto link = definitionInAst(navigation.parseResult, filePath,
                                            originToken, member, kind)) {
        return link;
      }
    } else if (const auto module = importLibrary->findModule(scriptId)) {
      if (const auto link =
              definitionInModule(module, originToken, member, kind)) {
        return link;
      }
    }

    const Resolver* scriptResolver =
        resolverForType(resolver, curType, importLibrary);
    if (!scriptResolver) {
      break;
    }
    const Opt<VellumType> parentType = scriptResolver->getParentType();
    if (!parentType || parentType->getState() != VellumTypeState::Identifier) {
      break;
    }
    curType = *parentType;
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
  PropertyObjectTypeFinder finder(pos, member);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          navigation.parseResult.declarations);
  finder.collect(declarations);

  if (!finder.result.found) {
    return std::nullopt;
  }

  const Resolver& resolver = *navigation.resolver;
  VellumType objectType = finder.result.objectType;

  if (finder.result.isSuper) {
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

  const Opt<MemberKind> kind = memberKindForTypeMember(
      resolver, objectType, member, finder.result.memberKind);
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
  const VellumType objectType = resolver.getObject().getType();

  if (!resolver.resolveFunction(objectType, name)) {
    return std::nullopt;
  }

  return definitionForTypeMember(navigation, filePath, target.token, name,
                                 objectType, MemberKind::Function,
                                 importLibrary);
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
    case AstLocatorTargetKind::ParentScriptName: {
      if (!target->identifier) {
        break;
      }
      if (const auto scriptType =
              navigation.resolver->resolveScriptType(*target->identifier)) {
        const Opt<VellumIdentifier> scriptId = identifierFromType(*scriptType);
        if (!scriptId) {
          break;
        }
        if (const auto currentScript =
                identifierFromType(navigation.resolver->getObject().getType());
            currentScript && *scriptId == *currentScript) {
          link = definitionInAst(navigation.parseResult, filePath,
                                 target->token, *scriptId, MemberKind::Script);
        } else {
          link =
              definitionInModule(importLibrary->findModule(*scriptId),
                                 target->token, *scriptId, MemberKind::Script);
        }
      }
      break;
    }
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
