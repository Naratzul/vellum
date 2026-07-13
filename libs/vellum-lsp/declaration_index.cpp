#include "declaration_index.h"

#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "common/string_utils.h"
#include "common/types.h"
#include "lsp_locations.h"
#include "vellum/vellum_type.h"

namespace vellum {
namespace {

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

bool stateMatches(Opt<VellumIdentifier> queryState,
                  Opt<VellumIdentifier> declState) {
  if (!queryState && !declState) {
    return true;
  }
  if (queryState && declState) {
    return namesMatch(*queryState, *declState);
  }
  return false;
}

class DeclarationIndexVisitor : public ast::DeclarationVisitor {
 public:
  DeclarationIndexVisitor(VellumIdentifier name, MemberKind kind,
                          Opt<VellumIdentifier> state)
      : queryName(name), queryKind(kind), queryState(state) {}

  Opt<Token> result() const { return found; }

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      if (found) {
        return;
      }
      decl->accept(*this);
    }
  }

  void visitImportDeclaration(ast::ImportDeclaration&) override {}

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    if (queryKind == MemberKind::Script) {
      if (const auto scriptName =
              identifierFromType(declaration.getScriptName());
          scriptName && namesMatch(queryName, *scriptName)) {
        found = declaration.getScriptNameLocation();
        return;
      }
    }

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
      if (found) {
        return;
      }
    }
  }

  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    if (queryKind == MemberKind::State &&
        namesMatch(queryName, VellumIdentifier(declaration.getStateName()))) {
      found = declaration.getStateNameLocation();
      return;
    }

    const Opt<VellumIdentifier> savedState = currentState;
    currentState = VellumIdentifier(declaration.getStateName());

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
      if (found) {
        currentState = savedState;
        return;
      }
    }

    currentState = savedState;
  }

  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override {
    if (currentState) {
      return;
    }
    if (queryKind != MemberKind::GlobalVariable ||
        !namesMatch(queryName, VellumIdentifier(declaration.name()))) {
      return;
    }
    if (const auto loc = declaration.getNameLocation()) {
      found = *loc;
    }
  }

  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (queryKind != MemberKind::Function || !declaration.getName()) {
      return;
    }
    if (!namesMatch(queryName, VellumIdentifier(*declaration.getName()))) {
      return;
    }
    if (!stateMatches(queryState, currentState)) {
      return;
    }
    if (const auto loc = declaration.getNameLocation()) {
      found = *loc;
    }
  }

  void visitPropertyDeclaration(
      ast::PropertyDeclaration& declaration) override {
    if (currentState) {
      return;
    }
    if (queryKind != MemberKind::Property ||
        !namesMatch(queryName, VellumIdentifier(declaration.getName()))) {
      return;
    }
    found = declaration.getNameLocation();
  }

 private:
  VellumIdentifier queryName;
  MemberKind queryKind;
  Opt<VellumIdentifier> queryState;
  Opt<VellumIdentifier> currentState;
  Opt<Token> found;
};

bool locationAfter(const Location& loc, lsp::Position pos) {
  return positionBefore(pos, toLsp(loc));
}

void mergeExtent(LocationRange& extent, const LocationRange& range) {
  if (range.start.line == 0 && range.end.line == 0) {
    return;
  }
  if (extent.start.line == 0 && extent.end.line == 0) {
    extent = range;
    return;
  }
  if (positionBefore(toLsp(range.start), toLsp(extent.start))) {
    extent.start = range.start;
  }
  if (positionBefore(toLsp(extent.end), toLsp(range.end))) {
    extent.end = range.end;
  }
}

void mergeExtent(LocationRange& extent, const Token& token) {
  if (token.lexeme.empty()) {
    return;
  }
  mergeExtent(extent, token.location);
}

class ExpressionExtentCollector : public ast::ExpressionVisitor {
 public:
  void collect(ast::Expression& expression) {
    mergeExtent(extent, expression.getLocation());
    expression.accept(*this);
  }

  LocationRange extent{};

  void visitIdentifierExpression(ast::IdentifierExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
  }
  void visitCallExpression(ast::CallExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getCallee()->accept(*this);
    for (const auto& arg : expr.getArguments()) {
      arg->accept(*this);
    }
  }
  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getObject()->accept(*this);
  }
  void visitPropertySetExpression(ast::PropertySetExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getObject()->accept(*this);
    expr.getValue()->accept(*this);
  }
  void visitAssignExpression(ast::AssignExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getName()->accept(*this);
    expr.getValue()->accept(*this);
  }
  void visitBinaryExpression(ast::BinaryExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }
  void visitUnaryExpression(ast::UnaryExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getOperand()->accept(*this);
  }
  void visitCastExpression(ast::CastExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getExpression()->accept(*this);
  }
  void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getArray()->accept(*this);
    expr.getIndex()->accept(*this);
  }
  void visitArrayIndexSetExpression(
      ast::ArrayIndexSetExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getArray()->accept(*this);
    expr.getIndex()->accept(*this);
    expr.getValue()->accept(*this);
  }
  void visitSelfExpression(ast::SelfExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
  }
  void visitSuperExpression(ast::SuperExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
  }
  void visitNewArrayExpression(ast::NewArrayExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    if (const auto& subtypeLocation = expr.getSubtypeLocation()) {
      mergeExtent(extent, *subtypeLocation);
    }
  }
  void visitNewArrayElementsExpression(
      ast::NewArrayElementsExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    for (const auto& element : expr.getElements()) {
      element->accept(*this);
    }
  }
  void visitTernaryExpression(ast::TernaryExpression& expr) override {
    mergeExtent(extent, expr.getLocation());
    expr.getCondition()->accept(*this);
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }
};

LocationRange expressionExtent(ast::Expression& expression) {
  ExpressionExtentCollector collector;
  collector.collect(expression);
  return collector.extent;
}

class StatementExtentCollector : public ast::StatementVisitor {
 public:
  void collect(ast::Statement& statement) { statement.accept(*this); }

  LocationRange extent{};

  void visitExpressionStatement(ast::ExpressionStatement& statement) override {
    mergeExtent(extent, expressionExtent(*statement.getExpression()));
  }
  void visitReturnStatement(ast::ReturnStatement& statement) override {
    mergeExtent(extent, statement.getLocation());
    if (statement.getExpression()) {
      mergeExtent(extent, expressionExtent(*statement.getExpression()));
    }
  }
  void visitBlockStatement(ast::BlockStatement& statement) override {
    for (const auto& stmt : statement.getStatements()) {
      StatementExtentCollector nested;
      nested.collect(*stmt);
      mergeExtent(extent, nested.extent);
    }
  }
  void visitIfStatement(ast::IfStatement& statement) override {
    mergeExtent(extent, expressionExtent(*statement.getCondition()));
    StatementExtentCollector thenCollector;
    thenCollector.collect(*statement.getThenBlock());
    mergeExtent(extent, thenCollector.extent);
    if (statement.getElseBlock()) {
      StatementExtentCollector elseCollector;
      elseCollector.collect(*statement.getElseBlock());
      mergeExtent(extent, elseCollector.extent);
    }
  }
  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override {
    mergeExtent(extent, statement.getNameLocation());
    if (statement.getInitializer()) {
      mergeExtent(extent, expressionExtent(*statement.getInitializer()));
    }
  }
  void visitWhileStatement(ast::WhileStatement& statement) override {
    mergeExtent(extent, expressionExtent(*statement.getCondition()));
    StatementExtentCollector bodyCollector;
    bodyCollector.collect(*statement.getBody());
    mergeExtent(extent, bodyCollector.extent);
  }
  void visitBreakStatement(ast::BreakStatement& statement) override {
    mergeExtent(extent, statement.getLocation());
  }
  void visitContinueStatement(ast::ContinueStatement& statement) override {
    mergeExtent(extent, statement.getLocation());
  }
  void visitForStatement(ast::ForStatement& statement) override {
    mergeExtent(extent, statement.getVariableNameLocation());
    mergeExtent(extent, statement.getArrayLocation());
    mergeExtent(extent, expressionExtent(*statement.getVariableName()));
    mergeExtent(extent, expressionExtent(*statement.getArray()));
    StatementExtentCollector bodyCollector;
    bodyCollector.collect(*statement.getBody());
    mergeExtent(extent, bodyCollector.extent);
  }
  void visitMatchStatement(ast::MatchStatement& statement) override {
    mergeExtent(extent, expressionExtent(*statement.getScrutinee()));
    for (const auto& arm : statement.getArms()) {
      for (const auto& pattern : arm.patterns) {
        mergeExtent(extent, expressionExtent(*pattern));
      }
      StatementExtentCollector bodyCollector;
      bodyCollector.collect(*arm.body);
      mergeExtent(extent, bodyCollector.extent);
    }
    if (statement.getElseBody()) {
      StatementExtentCollector elseCollector;
      elseCollector.collect(*statement.getElseBody());
      mergeExtent(extent, elseCollector.extent);
    }
  }
};

LocationRange statementExtent(ast::Statement& statement) {
  StatementExtentCollector collector;
  collector.collect(statement);
  return collector.extent;
}

bool statementStartsAfterUse(ast::Statement& statement, lsp::Position usePos) {
  const LocationRange extent = statementExtent(statement);
  if (extent.start.line == 0 && extent.end.line == 0) {
    return false;
  }
  return locationAfter(extent.start, usePos);
}

bool statementEndsBeforeUse(ast::Statement& statement, lsp::Position usePos) {
  const LocationRange extent = statementExtent(statement);
  if (extent.start.line == 0 && extent.end.line == 0) {
    return false;
  }
  return locationBefore(extent.end, usePos);
}

bool statementContainsUse(ast::Statement& statement, lsp::Position usePos) {
  return !statementStartsAfterUse(statement, usePos) &&
         !statementEndsBeforeUse(statement, usePos);
}

bool blockContainsUse(const ast::BlockStatement& block, lsp::Position usePos) {
  for (const auto& stmt : block.getStatements()) {
    if (statementContainsUse(*stmt, usePos)) {
      return true;
    }
  }
  return false;
}

bool functionContainsUse(ast::FunctionDeclaration& declaration,
                         lsp::Position usePos) {
  for (const auto& param : declaration.getParameters()) {
    if (positionInRange(usePos, param.nameLocation.location) ||
        positionInRange(usePos, param.typeLocation.location)) {
      return true;
    }
  }
  if (declaration.getBody() &&
      blockContainsUse(*declaration.getBody(), usePos)) {
    return true;
  }
  return false;
}

Opt<Token> tokenForName(VellumIdentifier name, const Token& token) {
  if (token.lexeme.empty()) {
    return std::nullopt;
  }
  return token;
}

class LocalBindingFinder : public ast::DeclarationVisitor {
 public:
  LocalBindingFinder(lsp::Position usePos, VellumIdentifier name)
      : usePos(usePos), queryName(name) {}

  Opt<Token> find(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      if (result) {
        return result;
      }
      decl->accept(*this);
    }
    return result;
  }

  void visitImportDeclaration(ast::ImportDeclaration&) override {}

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
      if (result) {
        return;
      }
    }
  }

  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
      if (result) {
        return;
      }
    }
  }

  void visitVariableDeclaration(ast::GlobalVariableDeclaration&) override {}

  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (!functionContainsUse(declaration, usePos)) {
      return;
    }

    for (const auto& param : declaration.getParameters()) {
      if (namesMatch(queryName, VellumIdentifier(param.name)) &&
          positionInRange(usePos, param.nameLocation.location)) {
        if (auto token = tokenForName(queryName, param.nameLocation)) {
          result = token;
          return;
        }
      }
    }

    Opt<Token> paramBinding;
    if (declaration.getBody() &&
        blockContainsUse(*declaration.getBody(), usePos)) {
      for (const auto& param : declaration.getParameters()) {
        if (namesMatch(queryName, VellumIdentifier(param.name))) {
          paramBinding = tokenForName(queryName, param.nameLocation);
          break;
        }
      }
      result =
          searchBlock(declaration.getBody()->getStatements(), paramBinding);
    }
  }

  void visitPropertyDeclaration(
      ast::PropertyDeclaration& declaration) override {
    if (const auto& getBlock = declaration.getGetAccessor()) {
      if (blockContainsUse(*getBlock.value(), usePos)) {
        result = searchBlock(getBlock.value()->getStatements(), {});
        if (result) {
          return;
        }
      }
    }
    if (const auto& setBlock = declaration.getSetAccessor()) {
      if (blockContainsUse(*setBlock.value(), usePos)) {
        result = searchBlock(setBlock.value()->getStatements(), {});
      }
    }
  }

 private:
  lsp::Position usePos;
  VellumIdentifier queryName;
  Opt<Token> result;

  void considerBinding(VellumIdentifier declName, const Token& declToken,
                       Opt<Token>& candidate) {
    if (!namesMatch(queryName, declName)) {
      return;
    }
    if (auto token = tokenForName(queryName, declToken)) {
      candidate = token;
    }
  }

  void addDeclarationFromStatement(ast::Statement& statement,
                                   Opt<Token>& candidate) {
    if (auto* local = dynamic_cast<ast::LocalVariableStatement*>(&statement)) {
      considerBinding(local->getName(), local->getNameLocation(), candidate);
    }
  }

  Opt<Token> searchBlock(
      const common::Vec<common::Unique<ast::Statement>>& statements,
      Opt<Token> outerBinding) {
    Opt<Token> candidate = outerBinding;

    for (const auto& stmt : statements) {
      if (statementStartsAfterUse(*stmt, usePos)) {
        break;
      }

      if (statementEndsBeforeUse(*stmt, usePos)) {
        addDeclarationFromStatement(*stmt, candidate);
        continue;
      }

      if (statementContainsUse(*stmt, usePos)) {
        if (auto inner = searchStatement(*stmt, candidate)) {
          return inner;
        }
        return candidate;
      }
    }

    return candidate;
  }

  Opt<Token> searchStatement(ast::Statement& statement,
                             Opt<Token> outerBinding) {
    if (auto* block = dynamic_cast<ast::BlockStatement*>(&statement)) {
      return searchBlock(block->getStatements(), outerBinding);
    }

    if (auto* local = dynamic_cast<ast::LocalVariableStatement*>(&statement)) {
      if (local->getInitializer() &&
          expressionContainsUse(*local->getInitializer(), usePos)) {
        return searchExpression(*local->getInitializer(), outerBinding);
      }
      return outerBinding;
    }

    if (auto* returnStmt = dynamic_cast<ast::ReturnStatement*>(&statement)) {
      if (returnStmt->getExpression()) {
        return searchExpression(*returnStmt->getExpression(), outerBinding);
      }
      return outerBinding;
    }

    if (auto* exprStmt = dynamic_cast<ast::ExpressionStatement*>(&statement)) {
      return searchExpression(*exprStmt->getExpression(), outerBinding);
    }

    if (auto* ifStmt = dynamic_cast<ast::IfStatement*>(&statement)) {
      if (expressionContainsUse(*ifStmt->getCondition(), usePos)) {
        return searchExpression(*ifStmt->getCondition(), outerBinding);
      }
      if (statementContainsUse(*ifStmt->getThenBlock(), usePos)) {
        return searchStatement(*ifStmt->getThenBlock(), outerBinding);
      }
      if (ifStmt->getElseBlock() &&
          statementContainsUse(*ifStmt->getElseBlock(), usePos)) {
        return searchStatement(*ifStmt->getElseBlock(), outerBinding);
      }
      return outerBinding;
    }

    if (auto* whileStmt = dynamic_cast<ast::WhileStatement*>(&statement)) {
      if (expressionContainsUse(*whileStmt->getCondition(), usePos)) {
        return searchExpression(*whileStmt->getCondition(), outerBinding);
      }
      return searchStatement(*whileStmt->getBody(), outerBinding);
    }

    if (auto* forStmt = dynamic_cast<ast::ForStatement*>(&statement)) {
      return searchForStatement(*forStmt, outerBinding);
    }

    return outerBinding;
  }

  bool expressionContainsUse(ast::Expression& expression, lsp::Position pos) {
    return positionInRange(pos, expressionExtent(expression));
  }

  Opt<Token> searchForStatement(ast::ForStatement& statement,
                                Opt<Token> outerBinding) {
    if (statementContainsUse(*statement.getBody(), usePos)) {
      Opt<Token> loopBinding;
      if (statement.getVariableName()->isIdentifierExpression()) {
        const VellumIdentifier loopName =
            statement.getVariableName()->asIdentifier().getIdentifier();
        if (namesMatch(queryName, loopName)) {
          loopBinding =
              tokenForName(queryName, statement.getVariableNameLocation());
        }
      }

      if (auto inner = searchStatement(*statement.getBody(), loopBinding)) {
        return inner;
      }
      return loopBinding ? loopBinding : outerBinding;
    }

    if (expressionContainsUse(*statement.getArray(), usePos)) {
      return searchExpression(*statement.getArray(), outerBinding);
    }

    if (statement.getVariableName()->isIdentifierExpression() &&
        expressionContainsUse(*statement.getVariableName(), usePos)) {
      const VellumIdentifier loopName =
          statement.getVariableName()->asIdentifier().getIdentifier();
      if (namesMatch(queryName, loopName)) {
        return tokenForName(queryName, statement.getVariableNameLocation());
      }
    }

    return outerBinding;
  }

  Opt<Token> searchExpression(ast::Expression& expression,
                              Opt<Token> outerBinding) {
    struct Finder : ast::ExpressionVisitor {
      Finder(lsp::Position usePos, VellumIdentifier queryName,
             Opt<Token> outerBinding, LocalBindingFinder& owner)
          : usePos(usePos),
            queryName(queryName),
            outerBinding(outerBinding),
            owner(owner) {}

      lsp::Position usePos;
      VellumIdentifier queryName;
      Opt<Token> outerBinding;
      LocalBindingFinder& owner;
      Opt<Token> result;

      void visitIdentifierExpression(ast::IdentifierExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        if (!namesMatch(queryName, expr.getIdentifier())) {
          return;
        }
        result = outerBinding;
      }

      void visitCallExpression(ast::CallExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getCallee()->accept(*this);
        if (result) {
          return;
        }
        for (const auto& arg : expr.getArguments()) {
          arg->accept(*this);
          if (result) {
            return;
          }
        }
      }

      void visitPropertyGetExpression(
          ast::PropertyGetExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getObject()->accept(*this);
      }

      void visitPropertySetExpression(
          ast::PropertySetExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getObject()->accept(*this);
        if (!result) {
          expr.getValue()->accept(*this);
        }
      }

      void visitAssignExpression(ast::AssignExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getName()->accept(*this);
        if (!result) {
          expr.getValue()->accept(*this);
        }
      }

      void visitBinaryExpression(ast::BinaryExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getLeft()->accept(*this);
        if (!result) {
          expr.getRight()->accept(*this);
        }
      }

      void visitUnaryExpression(ast::UnaryExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getOperand()->accept(*this);
      }

      void visitCastExpression(ast::CastExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getExpression()->accept(*this);
      }

      void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getArray()->accept(*this);
        if (!result) {
          expr.getIndex()->accept(*this);
        }
      }

      void visitArrayIndexSetExpression(
          ast::ArrayIndexSetExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getArray()->accept(*this);
        if (!result) {
          expr.getIndex()->accept(*this);
        }
        if (!result) {
          expr.getValue()->accept(*this);
        }
      }

      void visitSelfExpression(ast::SelfExpression&) override {}
      void visitSuperExpression(ast::SuperExpression&) override {}
      void visitNewArrayExpression(ast::NewArrayExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
      }
      void visitNewArrayElementsExpression(
          ast::NewArrayElementsExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        for (const auto& element : expr.getElements()) {
          element->accept(*this);
          if (result) {
            return;
          }
        }
      }
      void visitTernaryExpression(ast::TernaryExpression& expr) override {
        if (!positionInRange(usePos, expr.getLocation().location)) {
          return;
        }
        expr.getCondition()->accept(*this);
        if (!result) {
          expr.getLeft()->accept(*this);
        }
        if (!result) {
          expr.getRight()->accept(*this);
        }
      }
    } finder(usePos, queryName, outerBinding, *this);
    expression.accept(finder);
    return finder.result;
  }
};

}  // namespace

Opt<Token> DeclarationIndex::findMemberDeclaration(
    const ParserResult& ast, VellumIdentifier name, MemberKind kind,
    Opt<VellumIdentifier> state) {
  DeclarationIndexVisitor visitor(name, kind, state);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          ast.declarations);
  visitor.collect(declarations);
  return visitor.result();
}

Opt<Token> DeclarationIndex::findLocalDeclaration(const ParserResult& ast,
                                                  lsp::Position usePosition,
                                                  VellumIdentifier name) {
  LocalBindingFinder finder(usePosition, name);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          ast.declarations);
  return finder.find(declarations);
}

}  // namespace vellum
