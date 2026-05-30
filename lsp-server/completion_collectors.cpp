#include "completion_collectors.h"

#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "common/string_utils.h"
#include "analyze/import_resolver.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "vellum/vellum_object.h"
#include "lsp_locations.h"
#include "navigation_utils.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_property.h"
#include "vellum/vellum_value.h"
#include "vellum/vellum_variable.h"
#include "vellum_keywords.h"

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
                          Vec<CompletionCandidate>& out,
                          common::Set<std::string_view>& seen) {
  VellumType curType = objectType;

  for (int depth = 0; depth < 64; ++depth) {
    if (curType.isArray()) {
      addArrayMembers(rootResolver, curType, out, seen);
      break;
    }

    const Resolver* typeResolver =
        resolverForType(rootResolver, curType, importLibrary);
    if (!typeResolver) {
      break;
    }

    addMemberFromObject(typeResolver->getObject(), *typeResolver, curType, out,
                        seen, /*includeStaticMembers=*/true);

    const Opt<VellumType> parentType = typeResolver->getParentType();
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
  while (end > 0 && (line[end - 1] == ' ' || line[end - 1] == '\t')) {
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

Opt<VellumType> resolveIdentifierBeforeDot(const Resolver& resolver,
                                           std::string_view line,
                                           size_t dotIndex) {
  const Opt<std::string> name = identifierTextBeforeDot(line, dotIndex);
  if (!name) {
    return std::nullopt;
  }

  const VellumIdentifier id(*name);

  if (const auto value = resolver.resolveIdentifier(id)) {
    switch (value->getType()) {
      case VellumValueType::Variable:
        return value->asVariable().getType();
      case VellumValueType::Property:
        return value->asProperty().getType();
      case VellumValueType::ScriptType:
        return value->asScriptType();
      default:
        break;
    }
  }

  if (const auto scriptType = resolver.resolveScriptType(id)) {
    return *scriptType;
  }

  return std::nullopt;
}

bool functionContainsUse(ast::FunctionDeclaration& declaration,
                         lsp::Position usePos) {
  for (const auto& param : declaration.getParameters()) {
    if (positionInRange(usePos, param.nameLocation.location) ||
        positionInRange(usePos, param.typeLocation.location)) {
      return true;
    }
  }
  return declaration.getBody() &&
         blockContainsUse(*declaration.getBody(), usePos);
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

    if (declaration.getBody() &&
        blockContainsUse(*declaration.getBody(), usePos)) {
      searchBlock(declaration.getBody()->getStatements());
    }
  }

 private:
  lsp::Position usePos;
  common::Set<std::string_view> seen_;

  void addLocal(std::string_view name, VellumType type, bool isParam) {
    const auto key = common::normalizeToLower(name);
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

  if (!owner.found && afterDot) {
    if (const Opt<size_t> dotAt =
            dotIndexAtOrBeforeCursor(sourceLine, pos.character)) {
      if (const auto type =
              resolveIdentifierBeforeDot(resolver, sourceLine, *dotAt)) {
        owner.objectType = *type;
        owner.found = true;
      } else if (importLibrary) {
        if (const Opt<std::string> partialName =
                identifierTextBeforeDot(sourceLine, *dotAt)) {
          const Vec<VellumIdentifier> matches =
              findImportModulesForQualifier(importLibrary, *partialName);
          if (!matches.empty()) {
            common::Set<std::string_view> seen;
            for (const VellumIdentifier& moduleId : matches) {
              if (navigation.importResolver) {
                navigation.importResolver->ensureModule(moduleId);
              }
              enumerateTypeMembers(resolver, VellumType::identifier(moduleId),
                                   importLibrary, out, seen);
            }
            return;
          }
        }
      }
    }
  }

  if (!owner.found) {
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

  if (navigation.importResolver &&
      objectType.getState() == VellumTypeState::Identifier) {
    navigation.importResolver->ensureModule(objectType.asIdentifier());
  }

  common::Set<std::string_view> seen;
  enumerateTypeMembers(resolver, objectType, importLibrary, out, seen);
}

void collectTypeMembersFromLine(const Shared<ImportLibrary>& importLibrary,
                                std::string_view sourceLine, lsp::Position pos,
                                bool afterDot, Vec<CompletionCandidate>& out) {
  if (!afterDot || !importLibrary) {
    return;
  }

  const Opt<size_t> dotAt =
      dotIndexAtOrBeforeCursor(sourceLine, pos.character);
  if (!dotAt) {
    return;
  }

  const Opt<std::string> partialName =
      identifierTextBeforeDot(sourceLine, *dotAt);
  if (!partialName) {
    return;
  }

  const Vec<VellumIdentifier> matches =
      findImportModulesForQualifier(importLibrary, *partialName);
  if (matches.empty()) {
    return;
  }

  auto errorHandler = common::makeShared<CompilerErrorHandler>();
  auto importResolver =
      common::makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = common::makeShared<BuiltinFunctions>();
  auto rootResolver = common::makeShared<Resolver>(
      VellumObject(VellumType::identifier(VellumIdentifier("_"))), errorHandler,
      importLibrary, builtinFunctions);

  common::Set<std::string_view> seen;
  for (const VellumIdentifier& moduleId : matches) {
    importResolver->ensureModule(moduleId);
    enumerateTypeMembers(*rootResolver, VellumType::identifier(moduleId),
                         importLibrary, out, seen);
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
            identifierFromType(navigation.resolver->getObject().getType())) {
      addCandidate(out, std::string(currentId->toString()),
                   lsp::CompletionItemKind::Class, "current script", "2_");
    }
  }
}

}  // namespace vellum
