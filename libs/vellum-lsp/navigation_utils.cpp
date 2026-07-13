#include "navigation_utils.h"

#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "common/string_utils.h"
#include "lsp_locations.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace {

class PropertyObjectTypeFinder : public ast::DeclarationVisitor,
                                 public ast::StatementVisitor,
                                 public ast::ExpressionVisitor {
 public:
  PropertyObjectTypeFinder(lsp::Position pos, Opt<VellumIdentifier> member)
      : pos(pos), member(std::move(member)) {}

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
  void visitMatchStatement(ast::MatchStatement& statement) override {
    statement.getScrutinee()->accept(*this);
    for (const auto& arm : statement.getArms()) {
      for (const auto& pattern : arm.patterns) {
        pattern->accept(*this);
      }
      arm.body->accept(*this);
    }
    if (statement.getElseBody()) {
      statement.getElseBody()->accept(*this);
    }
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
    visitExpression(*expr.getObject(), depth + 1);
    considerPropertySet(expr);
    visitExpression(*expr.getValue(), depth + 1);
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
  void visitNewArrayExpression(ast::NewArrayExpression& expr) override {
    if (!positionInRange(pos, expr.getLocation().location)) {
      return;
    }
    if (const auto& subtypeLocation = expr.getSubtypeLocation()) {
      if (positionInRange(pos, subtypeLocation->location)) {
        return;
      }
    }
  }
  void visitNewArrayElementsExpression(
      ast::NewArrayElementsExpression& expr) override {
    for (const auto& element : expr.getElements()) {
      visitExpression(*element, depth + 1);
    }
  }
  void visitTernaryExpression(ast::TernaryExpression& expr) override {
    expr.getCondition()->accept(*this);
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }

 private:
  lsp::Position pos;
  Opt<VellumIdentifier> member;
  int depth{0};
  int bestDepth{-1};

  void considerPropertyGet(ast::PropertyGetExpression& expr) {
    considerMemberAccess(expr.getLocation(), expr.getProperty(), *expr.getObject(),
                         expr.getIdentifierType());
  }

  void considerPropertySet(ast::PropertySetExpression& expr) {
    considerMemberAccess(expr.getPropertyLocation(), expr.getProperty(),
                         *expr.getObject());
  }

  void considerMemberAccess(const Token& memberLocation,
                            VellumIdentifier property,
                            const ast::Expression& object,
                            VellumValueType identifierType =
                                VellumValueType::Identifier) {
    const bool inRange = positionInRange(pos, memberLocation.location);
    const bool atMember =
        member && namesMatch(*member, property) && inRange;
    const bool anyMember = !member && inRange;
    if (!atMember && !anyMember) {
      return;
    }
    if (!object.hasResolvedType()) {
      return;
    }
    if (depth < bestDepth) {
      return;
    }

    bestDepth = depth;
    result.isSuper = object.isSuperExpression();
    result.objectType = object.getType();
    result.memberKind = std::nullopt;
    switch (identifierType) {
      case VellumValueType::Function:
        result.memberKind = MemberKind::Function;
        break;
      case VellumValueType::Property:
        result.memberKind = MemberKind::Property;
        break;
      case VellumValueType::Variable:
        result.memberKind = MemberKind::GlobalVariable;
        break;
      default:
        break;
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

}  // namespace

bool namesMatch(VellumIdentifier a, VellumIdentifier b) {
  return common::normalizeToLower(a.toString()) ==
         common::normalizeToLower(b.toString());
}

Opt<VellumIdentifier> identifierFromType(const VellumType& type) {
  if (type.getState() != VellumTypeState::Identifier) {
    return std::nullopt;
  }
  return type.asIdentifier();
}

const Resolver* resolverForType(const Resolver& currentResolver,
                                VellumType type,
                                const Shared<ImportLibrary>& importLibrary) {
  if (type.getState() != VellumTypeState::Identifier) {
    return nullptr;
  }
  const VellumIdentifier typeId = type.asIdentifier();
  if (currentResolver.getObjectType() == type) {
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

}  // namespace

PropertyOwnerContext findPropertyOwnerAtPosition(
    const ParserResult& parseResult, lsp::Position pos,
    Opt<VellumIdentifier> memberFilter) {
  PropertyObjectTypeFinder finder(pos, std::move(memberFilter));
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          parseResult.declarations);
  finder.collect(declarations);
  return finder.result;
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

  if (resolver.resolveVariable(objectType, member)) {
    return MemberKind::GlobalVariable;
  }

  return std::nullopt;
}

Opt<lsp::DefinitionLink> definitionForTypeMember(
    const NavigationContext& navigation, const common::fs::path& filePath,
    const Token& originToken, VellumIdentifier member, VellumType objectType,
    MemberKind kind, const Shared<ImportLibrary>& importLibrary) {
  if (objectType.getState() != VellumTypeState::Identifier &&
      !objectType.isArray()) {
    return std::nullopt;
  }

  const Resolver& resolver = *navigation.resolver;
  const Opt<VellumIdentifier> currentScript =
      identifierFromType(resolver.getObjectType());

  VellumType curType = objectType;
  for (int depth = 0; depth < 64; ++depth) {
    if (curType.isArray()) {
      break;
    }
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

}  // namespace vellum
