#include "ast_locator.h"

#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "lsp_locations.h"
#include "vellum/vellum_type.h"

namespace vellum {
namespace {

Opt<VellumIdentifier> identifierFromType(const VellumType& type) {
  if (type.getState() != VellumTypeState::Identifier) {
    return std::nullopt;
  }
  return type.asIdentifier();
}

Opt<VellumIdentifier> identifierFromAnnotatedType(const VellumType& type) {
  if (const auto id = identifierFromType(type)) {
    return id;
  }
  if (type.getState() == VellumTypeState::Array) {
    return identifierFromAnnotatedType(*type.asArraySubtype());
  }
  return std::nullopt;
}

class AstLocatorVisitor : public ast::DeclarationVisitor,
                          public ast::StatementVisitor,
                          public ast::ExpressionVisitor {
 public:
  explicit AstLocatorVisitor(lsp::Position pos) : pos(pos) {}

  Opt<AstLocatorTarget> result() const { return best; }

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      decl->accept(*this);
    }
  }

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override {
    considerToken(declaration.getImportNameLocation(),
                  AstLocatorTargetKind::ImportName,
                  VellumIdentifier(declaration.getImportName()));
  }

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    if (const auto scriptName =
            identifierFromType(declaration.getScriptName())) {
      considerToken(declaration.getScriptNameLocation(),
                    AstLocatorTargetKind::ScriptName, *scriptName);
    }
    if (const auto parentLoc = declaration.getParentScriptNameLocation()) {
      if (const auto parentName =
              identifierFromType(declaration.getParentScriptName())) {
        considerToken(*parentLoc, AstLocatorTargetKind::ParentScriptName,
                      *parentName);
      }
    }

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }

  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    considerToken(declaration.getStateNameLocation(),
                  AstLocatorTargetKind::DeclName,
                  VellumIdentifier(declaration.getStateName()));
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }

  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override {
    considerOptToken(declaration.getNameLocation(),
                     AstLocatorTargetKind::DeclName,
                     VellumIdentifier(declaration.name()));
    if (const auto& typeName = declaration.typeName()) {
      considerTypeAnnotation(declaration.getTypeLocation(), *typeName);
    }
    if (const auto& init = declaration.initializer()) {
      visitExpression(*init, depth + 1);
    }
  }

  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (declaration.getName()) {
      considerOptToken(declaration.getNameLocation(),
                       AstLocatorTargetKind::DeclName,
                       VellumIdentifier(*declaration.getName()));
    }
    for (const auto& param : declaration.getParameters()) {
      considerTypeAnnotation(param.typeLocation, param.type);
    }
    considerTypeAnnotation(declaration.getReturnTypeLocation(),
                           declaration.getReturnTypeName());
    declaration.getBody()->accept(*this);
  }

  void visitPropertyDeclaration(
      ast::PropertyDeclaration& declaration) override {
    considerToken(declaration.getNameLocation(), AstLocatorTargetKind::DeclName,
                  VellumIdentifier(declaration.getName()));
    considerTypeAnnotation(declaration.getTypeLocation(),
                           declaration.getTypeName());
    if (const auto& getBlock = declaration.getGetAccessor()) {
      getBlock.value()->accept(*this);
    }
    if (const auto& setBlock = declaration.getSetAccessor()) {
      setBlock.value()->accept(*this);
    }
  }

  void visitExpressionStatement(ast::ExpressionStatement& statement) override {
    visitExpression(*statement.getExpression(), depth + 1);
  }

  void visitReturnStatement(ast::ReturnStatement& statement) override {
    if (statement.getExpression()) {
      visitExpression(*statement.getExpression(), depth + 1);
    }
  }

  void visitIfStatement(ast::IfStatement& statement) override {
    visitExpression(*statement.getCondition(), depth + 1);
    statement.getThenBlock()->accept(*this);
    if (const auto& elseBlock = statement.getElseBlock()) {
      elseBlock->accept(*this);
    }
  }

  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override {
    considerToken(statement.getNameLocation(), AstLocatorTargetKind::DeclName,
                  statement.getName());
    if (const auto& type = statement.getType()) {
      considerTypeAnnotation(statement.getTypeLocation(), *type);
    }
    if (const auto& init = statement.getInitializer()) {
      visitExpression(*init, depth + 1);
    }
  }

  void visitWhileStatement(ast::WhileStatement& statement) override {
    visitExpression(*statement.getCondition(), depth + 1);
    statement.getBody()->accept(*this);
  }

  void visitBreakStatement(ast::BreakStatement&) override {}
  void visitContinueStatement(ast::ContinueStatement&) override {}

  void visitForStatement(ast::ForStatement& statement) override {
    const Token& varLoc = statement.getVariableNameLocation();
    considerToken(varLoc, AstLocatorTargetKind::DeclName,
                  VellumIdentifier(varLoc.lexeme));
    visitExpression(*statement.getArray(), depth + 1);
    statement.getBody()->accept(*this);
  }

  void visitMatchStatement(ast::MatchStatement& statement) override {
    visitExpression(*statement.getScrutinee(), depth + 1);
    for (const auto& arm : statement.getArms()) {
      for (const auto& pattern : arm.patterns) {
        visitExpression(*pattern, depth + 1);
      }
      arm.body->accept(*this);
    }
    if (const auto& elseBody = statement.getElseBody()) {
      elseBody->accept(*this);
    }
  }

  void visitBlockStatement(ast::BlockStatement& statement) override {
    for (const auto& stmt : statement.getStatements()) {
      stmt->accept(*this);
    }
  }

  void visitIdentifierExpression(ast::IdentifierExpression& expr) override {
    considerToken(expr.getLocation(), AstLocatorTargetKind::IdentifierUse,
                  expr.getIdentifier());
  }

  void visitCallExpression(ast::CallExpression& expr) override {
    if (expr.getCallee()->isIdentifierExpression()) {
      considerToken(expr.getCallee()->getLocation(),
                    AstLocatorTargetKind::CallCallee,
                    expr.getCallee()->asIdentifier().getIdentifier());
    } else {
      visitExpression(*expr.getCallee(), depth + 1);
    }
    for (const auto& arg : expr.getArguments()) {
      visitExpression(*arg, depth + 1);
    }
  }

  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override {
    considerToken(expr.getLocation(), AstLocatorTargetKind::PropertyUse,
                  expr.getProperty());
    visitExpression(*expr.getObject(), depth + 1);
  }

  void visitPropertySetExpression(ast::PropertySetExpression& expr) override {
    considerToken(expr.getPropertyLocation(), AstLocatorTargetKind::PropertyUse,
                  expr.getProperty());
    visitExpression(*expr.getObject(), depth + 1);
    visitExpression(*expr.getValue(), depth + 1);
  }

  void visitAssignExpression(ast::AssignExpression& expr) override {
    visitExpression(*expr.getName(), depth + 1);
    visitExpression(*expr.getValue(), depth + 1);
  }

  void visitBinaryExpression(ast::BinaryExpression& expr) override {
    visitExpression(*expr.getLeft(), depth + 1);
    visitExpression(*expr.getRight(), depth + 1);
  }

  void visitUnaryExpression(ast::UnaryExpression& expr) override {
    visitExpression(*expr.getOperand(), depth + 1);
  }

  void visitCastExpression(ast::CastExpression& expr) override {
    visitExpression(*expr.getExpression(), depth + 1);
    considerToken(expr.getTargetExpression()->getLocation(),
                  AstLocatorTargetKind::TypeReference,
                  expr.getTargetExpression()->getIdentifier());
  }

  void visitNewArrayExpression(ast::NewArrayExpression& expr) override {
    if (const auto& subtype = expr.getSubtype()) {
      if (const auto& subtypeLocation = expr.getSubtypeLocation()) {
        considerTypeAnnotation(*subtypeLocation, *subtype);
      }
    }
  }
  void visitNewArrayElementsExpression(
      ast::NewArrayElementsExpression& expr) override {
    for (const auto& element : expr.getElements()) {
      visitExpression(*element, depth + 1);
    }
  }
  void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override {
    visitExpression(*expr.getArray(), depth + 1);
    visitExpression(*expr.getIndex(), depth + 1);
  }

  void visitArrayIndexSetExpression(
      ast::ArrayIndexSetExpression& expr) override {
    visitExpression(*expr.getArray(), depth + 1);
    visitExpression(*expr.getIndex(), depth + 1);
    visitExpression(*expr.getValue(), depth + 1);
  }

  void visitSelfExpression(ast::SelfExpression&) override {}
  void visitSuperExpression(ast::SuperExpression&) override {}
  void visitTernaryExpression(ast::TernaryExpression& expr) override {
    visitExpression(*expr.getCondition(), depth + 1);
    visitExpression(*expr.getLeft(), depth + 1);
    visitExpression(*expr.getRight(), depth + 1);
  }
  void visitInterpolatedStringExpression(
      ast::InterpolatedStringExpression& expr) override {
    for (const auto& part : expr.getParts()) {
      visitExpression(*part, depth + 1);
    }
  }

 private:
  lsp::Position pos;
  int depth{0};
  int bestDepth{-1};
  Opt<AstLocatorTarget> best;

  void consider(AstLocatorTargetKind kind, const Token& token,
                Opt<VellumIdentifier> identifier, int atDepth) {
    if (!positionInRange(pos, token.location)) {
      return;
    }
    if (atDepth >= bestDepth) {
      bestDepth = atDepth;
      best = AstLocatorTarget{
          .kind = kind, .token = token, .identifier = identifier};
    }
  }

  void considerToken(const Token& token, AstLocatorTargetKind kind,
                     VellumIdentifier identifier) {
    consider(kind, token, identifier, depth);
  }

  void considerOptToken(const common::Opt<Token>& token,
                        AstLocatorTargetKind kind,
                        VellumIdentifier identifier) {
    if (token) {
      considerToken(*token, kind, identifier);
    }
  }

  void considerTypeAnnotation(const common::Opt<Token>& typeLocation,
                              const VellumType& type) {
    if (!typeLocation) {
      return;
    }
    if (const auto id = identifierFromAnnotatedType(type)) {
      considerOptToken(typeLocation, AstLocatorTargetKind::TypeReference, *id);
      return;
    }
    if (type.getState() == VellumTypeState::Unresolved &&
        !typeLocation->lexeme.empty()) {
      considerOptToken(typeLocation, AstLocatorTargetKind::TypeReference,
                       VellumIdentifier(typeLocation->lexeme));
    }
  }

  void considerTypeAnnotation(const Token& typeLocation,
                              const VellumType& type) {
    considerTypeAnnotation(common::Opt<Token>{typeLocation}, type);
  }

  void visitExpression(ast::Expression& expr, int childDepth) {
    const int saved = depth;
    depth = childDepth;
    expr.accept(*this);
    depth = saved;
  }
};

}  // namespace

Opt<AstLocatorTarget> AstLocator::locate(const ParserResult& parseResult,
                                         lsp::Position pos) {
  AstLocatorVisitor visitor(pos);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          parseResult.declarations);
  visitor.collect(declarations);
  return visitor.result();
}

}  // namespace vellum
