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
      .originSelectionRange = toLspRange(originToken),
      .targetUri = pathToUri(targetPath),
      .targetRange = targetRange,
      .targetSelectionRange = targetRange,
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
  if (const Resolver* scriptResolver = currentResolver.findScriptResolver(typeId)) {
    return scriptResolver;
  }
  if (const auto module = importLibrary->findModule(typeId)) {
    if (module->getResolver()) {
      return module->getResolver().get();
    }
  }
  return nullptr;
}

struct PropertyOwnerContext {
  VellumType objectType{VellumType::unresolved()};
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
      if (result.found) {
        return;
      }
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
  void visitVariableDeclaration(ast::GlobalVariableDeclaration& declaration) override {
    if (declaration.initializer()) {
      declaration.initializer()->accept(*this);
    }
  }
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override {
    declaration.getBody()->accept(*this);
  }
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override {
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
  void visitLocalVariableStatement(ast::LocalVariableStatement& statement) override {
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
    expr.getCallee()->accept(*this);
    for (const auto& arg : expr.getArguments()) {
      arg->accept(*this);
    }
  }
  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override {
    if (result.found) {
      return;
    }
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
    result.objectType = expr.getObject()->getType();
    result.found = true;
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
  void visitArrayIndexSetExpression(ast::ArrayIndexSetExpression& expr) override {
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
};

Opt<lsp::DefinitionLink> resolveIdentifierTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath, const Shared<ImportLibrary>& importLibrary) {
  if (!target.identifier) {
    return std::nullopt;
  }

  const VellumIdentifier name = *target.identifier;
  const Resolver& resolver = *navigation.resolver;

  if (const auto var = resolver.resolveVariable(name)) {
    const Token& loc = var->getNameLocation();
    if (!loc.lexeme.empty()) {
      return makeDefinitionLink(target.token, filePath, loc);
    }
  }

  if (const auto value = resolver.resolveIdentifier(name)) {
    switch (value->getType()) {
      case VellumValueType::Variable:
        return definitionInAst(navigation.parseResult, filePath, target.token, name,
                               MemberKind::GlobalVariable);
      case VellumValueType::Property:
        return definitionInAst(navigation.parseResult, filePath, target.token, name,
                               MemberKind::Property);
      case VellumValueType::Function:
        return definitionInAst(navigation.parseResult, filePath, target.token, name,
                               MemberKind::Function);
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
        return definitionInModule(importLibrary->findModule(*scriptName), target.token,
                                  *scriptName, MemberKind::Script);
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
  if (resolver.getObject().getType().getState() == VellumTypeState::Identifier) {
    const VellumIdentifier scriptId = resolver.getObject().getType().asIdentifier();
    if (const auto module = importLibrary->findModule(scriptId)) {
      return module->getFilePath();
    }
  }
  return currentFilePath;
}

Opt<lsp::DefinitionLink> resolvePropertyTarget(
    const AstLocatorTarget& target, const NavigationContext& navigation,
    const common::fs::path& filePath, const Shared<ImportLibrary>& importLibrary,
    lsp::Position pos) {
  if (!target.identifier || !navigation.semanticOk) {
    return std::nullopt;
  }

  const VellumIdentifier member = *target.identifier;
  PropertyObjectTypeFinder finder(pos, member);
  auto& declarations = const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
      navigation.parseResult.declarations);
  finder.collect(declarations);

  if (!finder.result.found) {
    return std::nullopt;
  }

  const VellumType objectType = finder.result.objectType;
  const Resolver& resolver = *navigation.resolver;
  if (!resolver.resolveProperty(objectType, member)) {
    return std::nullopt;
  }

  const Resolver* ownerResolver = resolverForType(resolver, objectType, importLibrary);
  if (!ownerResolver) {
    return std::nullopt;
  }

  if (ownerResolver == navigation.resolver.get()) {
    return definitionInAst(navigation.parseResult, filePath, target.token, member,
                           MemberKind::Property);
  }

  const Opt<VellumIdentifier> ownerScript =
      identifierFromType(ownerResolver->getObject().getType());
  if (!ownerScript) {
    return std::nullopt;
  }
  const auto module = importLibrary->findModule(*ownerScript);
  return definitionInModule(module, target.token, member, MemberKind::Property);
}

}  // namespace

lsp::Array<lsp::DefinitionLink> DefinitionsProvider::getDefinitions(
    const path& filePath, std::string_view scriptName, lsp::Position pos,
    DocumentStore& store, const Shared<ImportLibrary>& importLibrary) {
  (void)scriptName;

  const CachedAnalysis& cache =
      store.getOrAnalyze(filePath, AnalysisKind::Full, importLibrary);

  if (!cache.navigation || !cache.navigation->parseOk) {
    return {};
  }

  const NavigationContext& navigation = *cache.navigation;
  const Opt<AstLocatorTarget> target = AstLocator::locate(navigation.parseResult, pos);
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
                                target->token, *target->identifier, MemberKind::Script);
      break;
    }
    case AstLocatorTargetKind::ScriptName:
    case AstLocatorTargetKind::ParentScriptName: {
      if (!target->identifier) {
        break;
      }
      if (const auto scriptType = navigation.resolver->resolveScriptType(*target->identifier)) {
        const Opt<VellumIdentifier> scriptId = identifierFromType(*scriptType);
        if (!scriptId) {
          break;
        }
        if (const auto currentScript =
                identifierFromType(navigation.resolver->getObject().getType());
            currentScript && *scriptId == *currentScript) {
          link = definitionInAst(navigation.parseResult, filePath, target->token,
                                 *scriptId, MemberKind::Script);
        } else {
          link = definitionInModule(importLibrary->findModule(*scriptId), target->token,
                                    *scriptId, MemberKind::Script);
        }
      }
      break;
    }
    case AstLocatorTargetKind::IdentifierUse:
      link = resolveIdentifierTarget(*target, navigation, filePath, importLibrary);
      break;
    case AstLocatorTargetKind::CallCallee: {
      if (!target->identifier || !navigation.semanticOk) {
        break;
      }
      if (const auto func = navigation.resolver->resolveFunction(
              navigation.resolver->getObject().getType(), *target->identifier)) {
        (void)func;
        link = definitionInAst(navigation.parseResult, filePath, target->token,
                               *target->identifier, MemberKind::Function);
      }
      break;
    }
    case AstLocatorTargetKind::PropertyUse:
      link = resolvePropertyTarget(*target, navigation, filePath, importLibrary, pos);
      break;
    case AstLocatorTargetKind::DeclName:
      if (target->identifier) {
        link = makeDefinitionLink(target->token, filePath, target->token);
      }
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
