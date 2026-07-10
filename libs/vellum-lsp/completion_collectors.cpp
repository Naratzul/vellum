#include "completion_collectors.h"

#include "analyze/import_library.h"
#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "common/string_utils.h"
#include "analyze/import_resolver.h"
#include "compiler/call_resolution.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lsp_locations.h"
#include "navigation_utils.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_property.h"
#include "vellum/vellum_value.h"
#include "vellum/vellum_variable.h"
#include "vellum_keywords.h"

#include <algorithm>
#include <climits>
#include <span>

namespace vellum {
namespace {

LocationRange statementExtent(ast::Statement& statement);

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

bool locationAfter(const Location& loc, lsp::Position pos) {
  return positionBefore(pos, toLsp(loc));
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

bool blockContainsUse(ast::BlockStatement& block, lsp::Position usePos) {
  for (const auto& stmt : block.getStatements()) {
    if (statementContainsUse(*stmt, usePos)) {
      return true;
    }
  }
  return false;
}

bool blockSpansPosition(ast::BlockStatement& block, lsp::Position usePos) {
  int minLine = INT_MAX;
  int maxLine = 0;
  for (const auto& stmt : block.getStatements()) {
    const LocationRange extent = statementExtent(*stmt);
    if (extent.start.line == 0 && extent.end.line == 0) {
      continue;
    }
    minLine = std::min(minLine, extent.start.line);
    maxLine = std::max(maxLine, extent.end.line);
  }
  if (minLine == INT_MAX) {
    return false;
  }
  return static_cast<int>(usePos.line) >= minLine &&
         static_cast<int>(usePos.line) <= maxLine;
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
    mergeExtent(extent, expr.getTargetExpression()->getLocation());
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

 private:
};

LocationRange expressionExtent(ast::Expression& expression) {
  ExpressionExtentCollector collector;
  collector.collect(expression);
  LocationRange extent = collector.extent;
  if (auto* grouping = dynamic_cast<ast::GroupingExpression*>(&expression)) {
    mergeExtent(extent, grouping->getLocation());
  }
  return extent;
}

bool dotFollowsExtent(std::string_view line, size_t dotIndex,
                      const LocationRange& extent, int lineIndex) {
  if (extent.start.line == 0 && extent.end.line == 0) {
    return false;
  }
  if (lineIndex != extent.end.line) {
    return false;
  }
  size_t pos = static_cast<size_t>(extent.end.position + 1);
  if (pos == dotIndex) {
    return true;
  }
  if (dotIndex <= pos) {
    return false;
  }
  while (pos < dotIndex && pos < line.size() &&
         (line[pos] == ' ' || line[pos] == '\t')) {
    pos++;
  }
  if (pos < dotIndex && pos < line.size() && line[pos] == '(') {
    pos++;
    int depth = 1;
    while (pos < dotIndex && pos < line.size() && depth > 0) {
      if (line[pos] == '(') {
        depth++;
      } else if (line[pos] == ')') {
        depth--;
      }
      pos++;
    }
    while (pos < dotIndex && pos < line.size() &&
           (line[pos] == ' ' || line[pos] == '\t')) {
      pos++;
    }
  } else if (pos < dotIndex && pos < line.size() && line[pos] == ')') {
    pos++;
    while (pos < dotIndex && pos < line.size() &&
           (line[pos] == ' ' || line[pos] == '\t')) {
      pos++;
    }
  }
  return pos == dotIndex;
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
};

LocationRange statementExtent(ast::Statement& statement) {
  StatementExtentCollector collector;
  collector.collect(statement);
  return collector.extent;
}

bool isIdentifierChar(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

std::string typeDetail(const VellumType& type) {
  return std::string(type.toString());
}

std::string functionDetail(const VellumFunction& func) {
  std::string detail = "(";
  bool first = true;
  for (const auto& param : func.getParameters()) {
    if (!first) {
      detail += ", ";
    }
    first = false;
    detail += param.getName().toString();
    detail += ": ";
    detail += param.getType().toString();
  }
  detail += ") -> ";
  detail += func.getReturnType().toString();
  return detail;
}

void addCandidate(Vec<CompletionCandidate>& out, std::string label,
                  lsp::CompletionItemKind kind, std::string detail,
                  std::string sortPrefix) {
  CompletionCandidate item;
  item.label = std::move(label);
  item.kind = kind;
  item.detail = std::move(detail);
  item.filterText = item.label;
  item.sortText = sortPrefix + item.label;
  out.push_back(std::move(item));
}

void addArrayMembers(const Resolver& resolver, VellumType objectType,
                    Vec<CompletionCandidate>& out,
                    common::Set<std::string_view>& seen) {
  if (seen.insert(common::normalizeToLower("length")).second) {
    addCandidate(out, "length", lsp::CompletionItemKind::Property, "Int", "1_");
  }
  for (const char* fn : {"find", "rfind"}) {
    if (seen.insert(common::normalizeToLower(fn)).second) {
      if (const auto arrayFunc = resolver.resolveFunction(
              objectType, VellumIdentifier(fn))) {
        addCandidate(out, fn, lsp::CompletionItemKind::Function,
                     functionDetail(*arrayFunc), "1_");
      }
    }
  }
}

std::string functionDetailFromDecl(const ast::FunctionDeclaration& decl) {
  std::string detail = "(";
  bool first = true;
  for (const auto& param : decl.getParameters()) {
    if (!first) {
      detail += ", ";
    }
    first = false;
    detail += param.name;
    detail += ": ";
    detail += param.type.toString();
  }
  detail += ") -> ";
  detail += decl.getReturnTypeName().toString();
  return detail;
}

class AstMembersCollector : public ast::DeclarationVisitor {
 public:
  AstMembersCollector(Vec<CompletionCandidate>& out,
                      common::Set<std::string_view>& seen,
                      bool includeStaticMembers)
      : out(out), seen(seen), includeStaticMembers(includeStaticMembers) {}

  void collect(const ParserResult& parseResult) {
    auto& declarations =
        const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
            parseResult.declarations);
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
    if (currentState_) {
      return;
    }
    const Opt<VellumIdentifier> savedState = currentState_;
    currentState_ = VellumIdentifier(declaration.getStateName());
    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
    currentState_ = savedState;
  }
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override {
    if (currentState_) {
      return;
    }
    const auto name = declaration.name();
    const auto key = common::normalizeToLower(name);
    if (!seen.insert(key).second) {
      return;
    }
    const VellumType type =
        declaration.typeName().value_or(VellumType::none());
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Variable,
                 typeDetail(type), "1_");
  }
  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (currentState_ || !declaration.getName()) {
      return;
    }
    if (declaration.isStatic() && !includeStaticMembers) {
      return;
    }
    const auto name = declaration.getName()->data();
    const auto key = common::normalizeToLower(name);
    if (!seen.insert(key).second) {
      return;
    }
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Function,
                 functionDetailFromDecl(declaration), "1_");
  }
  void visitPropertyDeclaration(
      ast::PropertyDeclaration& declaration) override {
    if (currentState_) {
      return;
    }
    const auto name = declaration.getName();
    const auto key = common::normalizeToLower(name);
    if (!seen.insert(key).second) {
      return;
    }
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Property,
                 typeDetail(declaration.getTypeName()), "1_");
  }

 private:
  Vec<CompletionCandidate>& out;
  common::Set<std::string_view>& seen;
  bool includeStaticMembers;
  Opt<VellumIdentifier> currentState_;
};

void addMembersFromAst(const ParserResult& parseResult,
                       Vec<CompletionCandidate>& out,
                       common::Set<std::string_view>& seen) {
  AstMembersCollector collector(out, seen, /*includeStaticMembers=*/true);
  collector.collect(parseResult);
}

void addMembersFromModule(const ImportModule& module,
                          Vec<CompletionCandidate>& out,
                          common::Set<std::string_view>& seen) {
  if (!module.isParsed()) {
    return;
  }
  addMembersFromAst(module.getAst(), out, seen);
}

Opt<VellumType> parentTypeFromAst(const ParserResult& parseResult) {
  for (const auto& decl : parseResult.declarations) {
    auto* script = dynamic_cast<ast::ScriptDeclaration*>(decl.get());
    if (!script) {
      continue;
    }
    const VellumType parent = script->getParentScriptName();
    if (parent.getState() == VellumTypeState::Identifier) {
      return parent;
    }
    if (parent.getState() == VellumTypeState::Unresolved) {
      const std::string_view raw = parent.asRawType();
      if (!raw.empty()) {
        return VellumType::identifier(VellumIdentifier(raw));
      }
    }
    return std::nullopt;
  }
  return std::nullopt;
}

Opt<VellumType> parentTypeForModule(const ImportModule& module,
                                    const Shared<ImportLibrary>& importLibrary,
                                    const Resolver& rootResolver) {
  if (!module.isParsed()) {
    return std::nullopt;
  }
  if (const Opt<VellumType> fromAst = parentTypeFromAst(module.getAst())) {
    if (fromAst->getState() == VellumTypeState::Identifier) {
      return fromAst;
    }
  }
  const VellumIdentifier typeId = module.getName();
  if (const Resolver* typeResolver =
          resolverForType(rootResolver, VellumType::identifier(typeId),
                          importLibrary)) {
    return typeResolver->getParentType();
  }
  return std::nullopt;
}

void addMemberFromObject(const VellumObject& object, const Resolver& resolver,
                         VellumType objectType, Vec<CompletionCandidate>& out,
                         common::Set<std::string_view>& seen,
                         bool includeStaticMembers) {
  if (objectType.isArray()) {
    addArrayMembers(resolver, objectType, out, seen);
    return;
  }

  for (const auto& var : object.getVariables()) {
    const auto name = var.getName().toString();
    if (!seen.insert(common::normalizeToLower(name)).second) {
      continue;
    }
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Variable,
                 typeDetail(var.getType()), "1_");
  }

  for (const auto& prop : object.getProperties()) {
    const auto name = prop.getName().toString();
    if (!seen.insert(common::normalizeToLower(name)).second) {
      continue;
    }
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Property,
                 typeDetail(prop.getType()), "1_");
  }

  for (const auto& func : object.getFunctions()) {
    if (func.isStatic() && !includeStaticMembers) {
      continue;
    }
    const auto name = func.getName().toString();
    if (!seen.insert(common::normalizeToLower(name)).second) {
      continue;
    }
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Function,
                 functionDetail(func), "1_");
  }
}

void enumerateTypeMembers(const Resolver& rootResolver, VellumType objectType,
                          const Shared<ImportLibrary>& importLibrary,
                          const ParserResult* localAst,
                          Vec<CompletionCandidate>& out,
                          common::Set<std::string_view>& seen) {
  VellumType curType = objectType;

  for (int depth = 0; depth < 64; ++depth) {
    if (curType.isArray()) {
      addArrayMembers(rootResolver, curType, out, seen);
      break;
    }

    if (curType.getState() != VellumTypeState::Identifier) {
      break;
    }

    const VellumIdentifier typeId = curType.asIdentifier();
    const size_t before = out.size();

    const Resolver* typeResolver =
        resolverForType(rootResolver, curType, importLibrary);
    if (typeResolver) {
      addMemberFromObject(typeResolver->getObject(), *typeResolver, curType, out,
                          seen, /*includeStaticMembers=*/true);
    }

    if (out.size() == before) {
      if (importLibrary) {
        if (const ImportModulePtr module = importLibrary->findModule(typeId)) {
          addMembersFromModule(*module, out, seen);
        }
      }
      if (out.size() == before && localAst &&
          rootResolver.getObjectType() == curType) {
        addMembersFromAst(*localAst, out, seen);
      }
    }

    Opt<VellumType> parentType;
    if (typeResolver) {
      parentType = typeResolver->getParentType();
    }
    if ((!parentType || parentType->getState() != VellumTypeState::Identifier) &&
        importLibrary) {
      if (const ImportModulePtr module = importLibrary->findModule(typeId)) {
        parentType = parentTypeForModule(*module, importLibrary, rootResolver);
      }
    }
    if (!parentType || parentType->getState() != VellumTypeState::Identifier) {
      break;
    }
    curType = *parentType;
  }
}

// Index of '.' at or before cursor (for `Math.|` or `Math|.`)
Opt<size_t> dotIndexAtOrBeforeCursor(std::string_view line, size_t cursor) {
  const size_t at = std::min(cursor, line.size());
  if (at < line.size() && line[at] == '.') {
    return at;
  }
  for (size_t i = at; i > 0; --i) {
    if (line[i - 1] == '.') {
      return i - 1;
    }
  }
  return std::nullopt;
}

Opt<std::string> identifierTextBeforeDot(std::string_view line, size_t dotIndex) {
  size_t end = dotIndex;
  while (end > 0 && (line[end - 1] == ' ' || line[end - 1] == '\t' ||
                     line[end - 1] == ')' || line[end - 1] == ']')) {
    end--;
  }
  size_t start = end;
  while (start > 0 && isIdentifierChar(line[start - 1])) {
    start--;
  }
  if (start >= end) {
    return std::nullopt;
  }
  return std::string(line.substr(start, end - start));
}

// Resolves a script qualifier before '.' for member completion.
// Exact module name (e.g. "Math") matches only that module, not "MathEx".
// Incomplete qualifiers (e.g. "Mat") match strict prefixes (Math, MathEx).
Vec<VellumIdentifier> findImportModulesForQualifier(
    const Shared<ImportLibrary>& importLibrary, std::string_view qualifier) {
  Vec<VellumIdentifier> matches;
  if (!importLibrary || qualifier.empty()) {
    return matches;
  }

  const VellumIdentifier qualifierId(qualifier);
  if (const auto module = importLibrary->findModule(qualifierId)) {
    matches.push_back(module->getName());
    return matches;
  }

  // Prefix matching is for module qualifiers like "Mat" → Math/MathEx, not
  // lowercase locals such as event parameter "p".
  if (qualifier.empty() || qualifier[0] < 'A' || qualifier[0] > 'Z') {
    return matches;
  }

  const std::string qualifierLower =
      std::string(common::normalizeToLower(qualifier));
  for (const auto& [name, module] : importLibrary->getAllModules()) {
    (void)module;
    const std::string_view nameStr = name.toString();
    if (common::normalizeToLower(nameStr).size() <= qualifierLower.size()) {
      continue;
    }
    if (startsWithIgnoreCase(nameStr, qualifier)) {
      matches.push_back(name);
    }
  }
  return matches;
}

bool functionContainsUse(ast::FunctionDeclaration& declaration,
                         lsp::Position usePos) {
  for (const auto& param : declaration.getParameters()) {
    if (positionInRange(usePos, param.nameLocation.location) ||
        positionInRange(usePos, param.typeLocation.location)) {
      return true;
    }
  }
  if (!declaration.getBody()) {
    return false;
  }
  return blockContainsUse(*declaration.getBody(), usePos) ||
         blockSpansPosition(*declaration.getBody(), usePos);
}

bool expressionContainsPosition(ast::Expression& expression, lsp::Position pos) {
  return positionInRange(pos, expressionExtent(expression));
}

class ExpressionTypeAtDotVisitor : public ast::ExpressionVisitor {
 public:
  ExpressionTypeAtDotVisitor(lsp::Position cursor, size_t dotIndex,
                             std::string_view sourceLine)
      : cursor_(cursor), dotIndex_(dotIndex), sourceLine_(sourceLine) {}

  Opt<VellumType> result() const { return result_; }

  void visitExpression(ast::Expression& expression, int childDepth) {
    const int saved = depth_;
    depth_ = childDepth;
    expression.accept(*this);
    depth_ = saved;
  }

  void visitIdentifierExpression(ast::IdentifierExpression& expr) override {
    considerLeaf(expr);
  }

  void visitSelfExpression(ast::SelfExpression& expr) override {
    considerLeaf(expr);
  }

  void visitSuperExpression(ast::SuperExpression& expr) override {
    considerLeaf(expr);
  }

  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override {
    if (!cursorAtOrAfterDot()) {
      return;
    }
    const LocationRange objectExtent = expressionExtent(*expr.getObject());
    if (dotFollowsExtent(sourceLine_, dotIndex_, objectExtent,
                         static_cast<int>(cursor_.line)) &&
        expr.getObject()->hasResolvedType()) {
      setResult(expr.getObject()->getType(), depth_);
    }
    visitExpression(*expr.getObject(), depth_ + 1);
  }

  void visitCallExpression(ast::CallExpression& expr) override {
    const LocationRange extent = expressionExtent(expr);
    const bool dotAfter = cursorAtOrAfterDot() &&
                          dotFollowsExtent(sourceLine_, dotIndex_, extent,
                                           static_cast<int>(cursor_.line));
    if (!expressionContainsPosition(expr, cursor_) && !dotAfter) {
      return;
    }
    considerLeaf(expr);
    visitExpression(*expr.getCallee(), depth_ + 1);
    for (const auto& arg : expr.getArguments()) {
      visitExpression(*arg, depth_ + 1);
    }
  }

  void visitPropertySetExpression(ast::PropertySetExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getObject(), depth_ + 1);
    visitExpression(*expr.getValue(), depth_ + 1);
  }

  void visitAssignExpression(ast::AssignExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getName(), depth_ + 1);
    visitExpression(*expr.getValue(), depth_ + 1);
  }

  void visitBinaryExpression(ast::BinaryExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getLeft(), depth_ + 1);
    visitExpression(*expr.getRight(), depth_ + 1);
  }

  void visitUnaryExpression(ast::UnaryExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getOperand(), depth_ + 1);
  }

  void visitCastExpression(ast::CastExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    considerLeaf(expr);
    visitExpression(*expr.getExpression(), depth_ + 1);
  }

  void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getArray(), depth_ + 1);
    visitExpression(*expr.getIndex(), depth_ + 1);
  }

  void visitArrayIndexSetExpression(
      ast::ArrayIndexSetExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getArray(), depth_ + 1);
    visitExpression(*expr.getIndex(), depth_ + 1);
    visitExpression(*expr.getValue(), depth_ + 1);
  }

  void visitNewArrayExpression(ast::NewArrayExpression& expr) override {
    const LocationRange extent = expressionExtent(expr);
    const bool dotAfter = cursorAtOrAfterDot() &&
                          dotFollowsExtent(sourceLine_, dotIndex_, extent,
                                           static_cast<int>(cursor_.line));
    if (!expressionContainsPosition(expr, cursor_) && !dotAfter) {
      return;
    }
    considerLeaf(expr);
  }
  void visitNewArrayElementsExpression(
      ast::NewArrayElementsExpression& expr) override {
    const LocationRange extent = expressionExtent(expr);
    const bool dotAfter = cursorAtOrAfterDot() &&
                          dotFollowsExtent(sourceLine_, dotIndex_, extent,
                                           static_cast<int>(cursor_.line));
    if (!expressionContainsPosition(expr, cursor_) && !dotAfter) {
      return;
    }
    considerLeaf(expr);
    for (const auto& element : expr.getElements()) {
      visitExpression(*element, depth_ + 1);
    }
  }
  void visitTernaryExpression(ast::TernaryExpression& expr) override {
    if (!expressionContainsPosition(expr, cursor_)) {
      return;
    }
    visitExpression(*expr.getCondition(), depth_ + 1);
    visitExpression(*expr.getLeft(), depth_ + 1);
    visitExpression(*expr.getRight(), depth_ + 1);
  }

 private:
  lsp::Position cursor_;
  size_t dotIndex_;
  std::string_view sourceLine_;
  Opt<VellumType> result_;
  int depth_{0};
  int bestDepth_{-1};

  bool cursorAtOrAfterDot() const { return cursor_.character >= dotIndex_; }

  void setResult(const VellumType& type, int depth) {
    if (depth < bestDepth_) {
      return;
    }
    bestDepth_ = depth;
    result_ = type;
  }

  void considerLeaf(ast::Expression& expr) {
    if (!expr.hasResolvedType()) {
      return;
    }
    if (!dotFollowsExtent(sourceLine_, dotIndex_, expressionExtent(expr),
                          static_cast<int>(cursor_.line))) {
      return;
    }
    if (!cursorAtOrAfterDot()) {
      return;
    }
    setResult(expr.getType(), depth_);
  }
};

class ExpressionTypeBeforeDotFinder : public ast::DeclarationVisitor,
                                      public ast::StatementVisitor {
 public:
  ExpressionTypeBeforeDotFinder(lsp::Position cursor, size_t dotIndex,
                                std::string_view sourceLine)
      : visitor_(cursor, dotIndex, sourceLine),
        cursor_(cursor),
        dotIndex_(dotIndex),
        sourceLine_(sourceLine) {}

  Opt<VellumType> collect(
      common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      decl->accept(*this);
    }
    return visitor_.result();
  }

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
  void visitVariableDeclaration(ast::GlobalVariableDeclaration&) override {}
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override {
    if (const auto& getBlock = declaration.getGetAccessor()) {
      if (blockContainsUse(*getBlock.value(), cursor_)) {
        searchBlock(getBlock.value()->getStatements());
      }
    }
    if (const auto& setBlock = declaration.getSetAccessor()) {
      if (blockContainsUse(*setBlock.value(), cursor_)) {
        searchBlock(setBlock.value()->getStatements());
      }
    }
  }
  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (!functionContainsUse(declaration, cursor_)) {
      return;
    }
    if (declaration.getBody()) {
      searchBlock(declaration.getBody()->getStatements());
    }
  }

  void visitExpressionStatement(ast::ExpressionStatement& statement) override {
    visitor_.visitExpression(*statement.getExpression(), 0);
  }
  void visitReturnStatement(ast::ReturnStatement& statement) override {
    if (statement.getExpression()) {
      visitor_.visitExpression(*statement.getExpression(), 0);
    }
  }
  void visitIfStatement(ast::IfStatement& statement) override {
    if (expressionContainsPosition(*statement.getCondition(), cursor_)) {
      visitor_.visitExpression(*statement.getCondition(), 0);
      return;
    }
    if (statementContainsUse(*statement.getThenBlock(), cursor_)) {
      searchStatement(*statement.getThenBlock());
      return;
    }
    if (statement.getElseBlock() &&
        statementContainsUse(*statement.getElseBlock(), cursor_)) {
      searchStatement(*statement.getElseBlock());
    }
  }
  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override {
    if (statement.getInitializer()) {
      visitor_.visitExpression(*statement.getInitializer(), 0);
    }
  }
  void visitWhileStatement(ast::WhileStatement& statement) override {
    if (expressionContainsPosition(*statement.getCondition(), cursor_)) {
      visitor_.visitExpression(*statement.getCondition(), 0);
      return;
    }
    searchStatement(*statement.getBody());
  }
  void visitBreakStatement(ast::BreakStatement&) override {}
  void visitContinueStatement(ast::ContinueStatement&) override {}
  void visitForStatement(ast::ForStatement& statement) override {
    if (statementContainsUse(*statement.getBody(), cursor_)) {
      searchStatement(*statement.getBody());
      return;
    }
    if (expressionContainsPosition(*statement.getArray(), cursor_)) {
      visitor_.visitExpression(*statement.getArray(), 0);
      return;
    }
    if (statement.getVariableName()->isIdentifierExpression() &&
        expressionContainsPosition(*statement.getVariableName(), cursor_)) {
      visitor_.visitExpression(*statement.getVariableName(), 0);
    }
  }
  void visitBlockStatement(ast::BlockStatement& statement) override {
    searchBlock(statement.getStatements());
  }

 private:
  ExpressionTypeAtDotVisitor visitor_;
  lsp::Position cursor_;
  size_t dotIndex_;
  std::string_view sourceLine_;

  bool visitExpressionStatementWithTrailingDot(
      ast::ExpressionStatement& statement) {
    const LocationRange extent = expressionExtent(*statement.getExpression());
    if (!dotFollowsExtent(sourceLine_, dotIndex_, extent,
                          static_cast<int>(cursor_.line))) {
      return false;
    }
    visitor_.visitExpression(*statement.getExpression(), 0);
    return true;
  }

  void searchBlock(const common::Vec<common::Unique<ast::Statement>>& statements) {
    for (const auto& stmt : statements) {
      if (statementStartsAfterUse(*stmt, cursor_)) {
        break;
      }
      if (statementEndsBeforeUse(*stmt, cursor_)) {
        if (auto* exprStmt =
                dynamic_cast<ast::ExpressionStatement*>(stmt.get())) {
          if (visitExpressionStatementWithTrailingDot(*exprStmt)) {
            return;
          }
        }
        continue;
      }
      searchStatement(*stmt);
      return;
    }
  }

  void searchStatement(ast::Statement& statement) { statement.accept(*this); }
};

Opt<VellumType> resolveExpressionTypeBeforeDot(const ParserResult& parseResult,
                                               lsp::Position pos,
                                               size_t dotIndex,
                                               std::string_view sourceLine) {
  ExpressionTypeBeforeDotFinder finder(pos, dotIndex, sourceLine);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          parseResult.declarations);
  return finder.collect(declarations);
}

class LocalScopeCollector : public ast::DeclarationVisitor {
 public:
  explicit LocalScopeCollector(lsp::Position usePos) : usePos(usePos) {}

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      decl->accept(*this);
    }
  }

  Vec<CompletionCandidate> result;

  Opt<VellumType> lookupType(VellumIdentifier name) const {
    const auto key = std::string(common::normalizeToLower(name.toString()));
    const auto it = localTypes_.find(key);
    if (it == localTypes_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

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
  void visitVariableDeclaration(ast::GlobalVariableDeclaration&) override {}
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override {
    if (const auto& getBlock = declaration.getGetAccessor()) {
      if (blockContainsUse(*getBlock.value(), usePos)) {
        searchBlock(getBlock.value()->getStatements());
      }
    }
    if (const auto& setBlock = declaration.getSetAccessor()) {
      if (blockContainsUse(*setBlock.value(), usePos)) {
        searchBlock(setBlock.value()->getStatements());
      }
    }
  }

  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    if (!functionContainsUse(declaration, usePos)) {
      return;
    }

    for (const auto& param : declaration.getParameters()) {
      addLocal(param.name, param.type, true);
    }

    if (declaration.getBody()) {
      searchBlock(declaration.getBody()->getStatements());
    }
  }

 private:
  lsp::Position usePos;
  common::Set<std::string_view> seen_;
  common::Map<std::string, VellumType> localTypes_;

  void addLocal(std::string_view name, VellumType type, bool isParam) {
    const auto key = common::normalizeToLower(name);
    localTypes_[std::string(key)] = type;
    if (!seen_.insert(key).second) {
      return;
    }
    CompletionCandidate item;
    item.label = std::string(name);
    item.kind = lsp::CompletionItemKind::Variable;
    item.detail = typeDetail(type);
    item.filterText = item.label;
    item.sortText = std::string(isParam ? "0_" : "0_") + item.label;
    result.push_back(std::move(item));
  }

  void addLocalFromStatement(ast::Statement& statement) {
    if (auto* local = dynamic_cast<ast::LocalVariableStatement*>(&statement)) {
      const VellumType type = local->getType().value_or(VellumType::none());
      addLocal(local->getName().toString(), type, false);
    }
  }

  void searchBlock(const common::Vec<common::Unique<ast::Statement>>& statements) {
    for (const auto& stmt : statements) {
      if (statementStartsAfterUse(*stmt, usePos)) {
        break;
      }
      if (statementEndsBeforeUse(*stmt, usePos)) {
        addLocalFromStatement(*stmt);
        continue;
      }
      if (statementContainsUse(*stmt, usePos)) {
        searchStatement(*stmt);
        return;
      }
    }
  }

  void searchStatement(ast::Statement& statement) {
    if (auto* block = dynamic_cast<ast::BlockStatement*>(&statement)) {
      searchBlock(block->getStatements());
      return;
    }

    if (auto* local = dynamic_cast<ast::LocalVariableStatement*>(&statement)) {
      if (local->getInitializer() &&
          positionInRange(usePos,
                          expressionExtent(*local->getInitializer()))) {
        return;
      }
      addLocalFromStatement(*local);
      return;
    }

    if (auto* ifStmt = dynamic_cast<ast::IfStatement*>(&statement)) {
      if (positionInRange(usePos, expressionExtent(*ifStmt->getCondition()))) {
        return;
      }
      if (statementContainsUse(*ifStmt->getThenBlock(), usePos)) {
        searchStatement(*ifStmt->getThenBlock());
        return;
      }
      if (ifStmt->getElseBlock() &&
          statementContainsUse(*ifStmt->getElseBlock(), usePos)) {
        searchStatement(*ifStmt->getElseBlock());
      }
      return;
    }

    if (auto* whileStmt = dynamic_cast<ast::WhileStatement*>(&statement)) {
      if (positionInRange(usePos,
                          expressionExtent(*whileStmt->getCondition()))) {
        return;
      }
      searchStatement(*whileStmt->getBody());
      return;
    }

    if (auto* forStmt = dynamic_cast<ast::ForStatement*>(&statement)) {
      if (statementContainsUse(*forStmt->getBody(), usePos)) {
        if (forStmt->getVariableName()->isIdentifierExpression()) {
          const VellumIdentifier loopName =
              forStmt->getVariableName()->asIdentifier().getIdentifier();
          addLocal(loopName.toString(), VellumType::none(), false);
        }
        searchStatement(*forStmt->getBody());
        return;
      }
      if (positionInRange(usePos, expressionExtent(*forStmt->getArray()))) {
        return;
      }
    }
  }
};

VellumType normalizeTypeForMemberAccess(const Resolver& resolver,
                                        VellumType type) {
  if (type.isArray()) {
    return VellumType::array(
        normalizeTypeForMemberAccess(resolver, *type.asArraySubtype()));
  }
  if (!type.isResolved()) {
    return resolver.resolveType(type, Token{});
  }
  return type;
}

Opt<VellumType> resolveKeywordQualifier(VellumIdentifier id,
                                        const Resolver& resolver) {
  if (namesMatch(id, VellumIdentifier("self"))) {
    return resolver.getObjectType();
  }
  if (namesMatch(id, VellumIdentifier("super"))) {
    return resolver.getParentType();
  }
  return std::nullopt;
}

Opt<VellumType> resolveTypeBeforeDot(const NavigationContext& navigation,
                                     const Resolver& resolver, lsp::Position pos,
                                     std::string_view line, size_t dotIndex) {
  auto finish = [&](VellumType type) -> Opt<VellumType> {
    type = normalizeTypeForMemberAccess(resolver, type);
    if (type.getState() == VellumTypeState::Identifier || type.isArray()) {
      return type;
    }
    return std::nullopt;
  };

  if (navigation.parseOk) {
    if (const Opt<VellumType> astType = resolveExpressionTypeBeforeDot(
            navigation.parseResult, pos, dotIndex, line)) {
      if (const Opt<VellumType> finished = finish(*astType)) {
        return finished;
      }
    }
  }

  const Opt<std::string> name = identifierTextBeforeDot(line, dotIndex);
  if (!name) {
    return std::nullopt;
  }

  const VellumIdentifier id(*name);

  if (const auto value = resolver.resolveIdentifier(id)) {
    switch (value->getType()) {
      case VellumValueType::Variable:
        return finish(value->asVariable().getType());
      case VellumValueType::Property:
        return finish(value->asProperty().getType());
      case VellumValueType::ScriptType:
        return finish(value->asScriptType());
      case VellumValueType::Function:
        return finish(value->asFunction().getReturnType());
      default:
        break;
    }
  }

  if (navigation.parseOk) {
    LocalScopeCollector localLookup(pos);
    auto& declarations =
        const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
            navigation.parseResult.declarations);
    localLookup.collect(declarations);
    if (const Opt<VellumType> localType = localLookup.lookupType(id)) {
      return finish(*localType);
    }
  }

  if (const Opt<VellumType> keywordType = resolveKeywordQualifier(id, resolver)) {
    return finish(*keywordType);
  }

  if (const auto scriptType = resolver.resolveScriptType(id)) {
    return finish(*scriptType);
  }

  return std::nullopt;
}

void appendTypeMembers(const NavigationContext& navigation,
                       const Resolver& resolver, VellumType objectType,
                       const Shared<ImportLibrary>& importLibrary,
                       Vec<CompletionCandidate>& out) {
  if (navigation.importResolver &&
      objectType.getState() == VellumTypeState::Identifier) {
    navigation.importResolver->ensureModule(objectType.asIdentifier());
  }

  common::Set<std::string_view> seen;
  enumerateTypeMembers(resolver, objectType, importLibrary,
                       &navigation.parseResult, out, seen);
}

void appendImportQualifierMembers(const NavigationContext& navigation,
                                  const Resolver& resolver,
                                  const Shared<ImportLibrary>& importLibrary,
                                  std::string_view qualifier,
                                  Vec<CompletionCandidate>& out) {
  if (!importLibrary) {
    return;
  }

  const Vec<VellumIdentifier> matches =
      findImportModulesForQualifier(importLibrary, qualifier);
  common::Set<std::string_view> seen;
  for (const VellumIdentifier& moduleId : matches) {
    if (navigation.importResolver) {
      navigation.importResolver->ensureModule(moduleId);
    }
    enumerateTypeMembers(resolver, VellumType::identifier(moduleId),
                         importLibrary, &navigation.parseResult, out, seen);
  }
}

}  // namespace

bool startsWithIgnoreCase(std::string_view haystack, std::string_view prefix) {
  if (prefix.size() > haystack.size()) {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(haystack[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
}

void collectKeywords(CompletionContextKind context,
                     Vec<CompletionCandidate>& out) {
  auto addList = [&](std::span<const std::string_view> words) {
    for (const auto word : words) {
      addCandidate(out, std::string(word), lsp::CompletionItemKind::Keyword, "",
                   "9_");
    }
  };

  switch (context) {
    case CompletionContextKind::Expression:
      addList(kExpressionKeywords);
      break;
    case CompletionContextKind::Keyword:
      addList(kAllCompletionKeywords);
      break;
    default:
      addList(kExpressionKeywords);
      break;
  }
}

void collectImportModules(const Shared<ImportLibrary>& importLibrary,
                            Vec<CompletionCandidate>& out) {
  if (!importLibrary) {
    return;
  }
  for (const auto& [name, module] : importLibrary->getAllModules()) {
    addCandidate(out, std::string(name.toString()),
                 lsp::CompletionItemKind::Class, module->getFilePath().string(),
                 "2_");
  }
}

void collectLocalsInScope(const NavigationContext& navigation, lsp::Position pos,
                          Vec<CompletionCandidate>& out) {
  if (!navigation.parseOk) {
    return;
  }
  LocalScopeCollector collector(pos);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(
          navigation.parseResult.declarations);
  collector.collect(declarations);
  for (auto& item : collector.result) {
    out.push_back(std::move(item));
  }
}

void collectScriptMembers(const NavigationContext& navigation,
                          Vec<CompletionCandidate>& out) {
  if (!navigation.parseOk || !navigation.resolver) {
    return;
  }

  const Resolver& resolver = *navigation.resolver;
  const VellumObject& object = resolver.getObject();
  common::Set<std::string_view> seen;

  addMemberFromObject(object, resolver, object.getType(), out, seen,
                      /*includeStaticMembers=*/false);

  BuiltinFunctions builtins;
  for (const auto& func : {VellumIdentifier("GetState"), VellumIdentifier("GoToState")}) {
    if (const auto builtin = builtins.getFunction(func)) {
      const auto name = builtin->getName().toString();
      if (seen.insert(common::normalizeToLower(name)).second) {
        addCandidate(out, std::string(name), lsp::CompletionItemKind::Function,
                     functionDetail(*builtin), "1_");
      }
    }
  }
}

void collectBareCallableCandidates(const NavigationContext& navigation,
                                   const Shared<ImportLibrary>& importLibrary,
                                   Vec<CompletionCandidate>& out) {
  if (!navigation.parseOk || !navigation.resolver || !importLibrary) {
    return;
  }

  const Resolver& resolver = *navigation.resolver;
  for (const auto& importName : resolver.getImportedObjects()) {
    if (navigation.importResolver) {
      navigation.importResolver->ensureModule(importName);
    }
  }

  const Vec<BareCallableCandidate> candidates =
      collectBareCallableCandidates(resolver);

  common::Map<std::string, int> nameCounts;
  for (const auto& candidate : candidates) {
    const auto key =
        std::string(common::normalizeToLower(candidate.function.getName().toString()));
    nameCounts[key]++;
  }

  common::Set<std::string_view> seen;
  for (const auto& var : resolver.getObject().getVariables()) {
    seen.insert(common::normalizeToLower(var.getName().toString()));
  }
  for (const auto& prop : resolver.getObject().getProperties()) {
    seen.insert(common::normalizeToLower(prop.getName().toString()));
  }
  for (const auto& func : resolver.getObject().getFunctions()) {
    if (!func.isStatic()) {
      seen.insert(common::normalizeToLower(func.getName().toString()));
    }
  }

  for (const auto& candidate : candidates) {
    const auto name = candidate.function.getName().toString();
    const auto key = common::normalizeToLower(name);
    const auto countIt = nameCounts.find(std::string(key));
    const bool isAmbiguous = countIt != nameCounts.end() && countIt->second > 1;

    std::string dedupeKey = std::string(key);
    std::string detail;
    if (candidate.isLocal) {
      detail = functionDetail(candidate.function);
    } else {
      detail = candidate.owner.toString();
      detail += '.';
      detail += name;
      if (isAmbiguous) {
        dedupeKey += '\0';
        dedupeKey += detail;
      }
    }

    if (!seen.insert(dedupeKey).second) {
      continue;
    }

    addCandidate(out, std::string(name), lsp::CompletionItemKind::Function,
                 detail, "1_");
  }
}

void collectTypeMembers(const NavigationContext& navigation,
                        const Shared<ImportLibrary>& importLibrary,
                        lsp::Position pos, std::string_view sourceLine,
                        bool afterDot, Opt<VellumIdentifier> memberPrefix,
                        Vec<CompletionCandidate>& out) {
  if (!navigation.parseOk || !navigation.resolver) {
    return;
  }

  const Resolver& resolver = *navigation.resolver;
  PropertyOwnerContext owner = findPropertyOwnerAtPosition(
      navigation.parseResult, pos, memberPrefix);

  Opt<std::string> qualifierBeforeDot;
  if (!owner.found && afterDot) {
    if (const Opt<size_t> dotAt =
            dotIndexAtOrBeforeCursor(sourceLine, pos.character)) {
      qualifierBeforeDot = identifierTextBeforeDot(sourceLine, *dotAt);
      if (const auto type = resolveTypeBeforeDot(navigation, resolver, pos,
                                                 sourceLine, *dotAt)) {
        owner.objectType = *type;
        owner.found = true;
      }
    }
  }

  if (!owner.found) {
    if (qualifierBeforeDot && importLibrary) {
      const size_t before = out.size();
      appendImportQualifierMembers(navigation, resolver, importLibrary,
                                   *qualifierBeforeDot, out);
      if (!out.empty() && out.size() > before) {
        return;
      }
    }
    return;
  }

  VellumType objectType = owner.objectType;
  if (owner.isSuper) {
    if (const auto parentType = resolver.getParentType()) {
      objectType = *parentType;
    } else {
      return;
    }
  }

  const size_t before = out.size();
  appendTypeMembers(navigation, resolver, objectType, importLibrary, out);
  if (out.size() == before && qualifierBeforeDot && importLibrary) {
    appendImportQualifierMembers(navigation, resolver, importLibrary,
                                 *qualifierBeforeDot, out);
  }
}

void collectTypeMembersFromLine(const Shared<ImportLibrary>& importLibrary,
                                std::string_view sourceLine, lsp::Position pos,
                                bool afterDot, Vec<CompletionCandidate>& out,
                                const NavigationContext* navigation) {
  if (!afterDot) {
    return;
  }

  const Opt<size_t> dotAt =
      dotIndexAtOrBeforeCursor(sourceLine, pos.character);
  if (!dotAt) {
    return;
  }

  const Opt<std::string> partialName =
      identifierTextBeforeDot(sourceLine, *dotAt);

  if (navigation && navigation->parseOk && navigation->resolver) {
    if (const auto type = resolveTypeBeforeDot(*navigation, *navigation->resolver,
                                               pos, sourceLine, *dotAt)) {
      const size_t before = out.size();
      appendTypeMembers(*navigation, *navigation->resolver, *type,
                        importLibrary, out);
      if (!out.empty() && out.size() > before) {
        return;
      }
    }
    if (partialName && importLibrary) {
      appendImportQualifierMembers(*navigation, *navigation->resolver,
                                   importLibrary, *partialName, out);
    }
    return;
  }

  if (!importLibrary || !partialName) {
    return;
  }

  auto errorHandler = common::makeShared<CompilerErrorHandler>();
  auto importResolver =
      common::makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = common::makeShared<BuiltinFunctions>();
  auto rootResolver = common::makeShared<Resolver>(
      VellumObject(VellumType::identifier(VellumIdentifier("_"))), errorHandler,
      importLibrary, builtinFunctions);

  const Vec<VellumIdentifier> matches =
      findImportModulesForQualifier(importLibrary, *partialName);
  common::Set<std::string_view> seen;
  for (const VellumIdentifier& moduleId : matches) {
    importResolver->ensureModule(moduleId);
    enumerateTypeMembers(*rootResolver, VellumType::identifier(moduleId),
                         importLibrary, nullptr, out, seen);
  }
}

void collectTypes(const NavigationContext& navigation,
                  const Shared<ImportLibrary>& importLibrary,
                  Vec<CompletionCandidate>& out) {
  for (const auto lit :
       {VellumLiteralType::Int, VellumLiteralType::Float, VellumLiteralType::String,
        VellumLiteralType::Bool, VellumLiteralType::None}) {
    const auto name = literalTypeToString(lit);
    addCandidate(out, std::string(name), lsp::CompletionItemKind::Class,
               "literal type", "2_");
  }

  if (importLibrary) {
    for (const auto& [name, module] : importLibrary->getAllModules()) {
      addCandidate(out, std::string(name.toString()),
                   lsp::CompletionItemKind::Class,
                   module->getFilePath().string(), "2_");
    }
  }

  if (navigation.parseOk && navigation.resolver) {
    if (const auto currentId =
            identifierFromType(navigation.resolver->getObjectType())) {
      addCandidate(out, std::string(currentId->toString()),
                   lsp::CompletionItemKind::Class, "current script", "2_");
    }
  }
}

}  // namespace vellum
