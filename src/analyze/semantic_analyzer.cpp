#include "semantic_analyzer.h"

#include <cassert>
#include <format>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"

namespace vellum {

SemanticAnalyzer::SemanticAnalyzer(
    std::shared_ptr<CompilerErrorHandler> errorHandler,
    std::shared_ptr<Resolver> resolver)
    : errorHandler(errorHandler), resolver(resolver) {
  assert(resolver);
}

SemanticAnalyzeResult SemanticAnalyzer::analyze(
    std::vector<std::unique_ptr<ast::Declaration>>&& declarations) {
  for (auto& declaration : declarations) {
    declaration->accept(*this);
  }

  return SemanticAnalyzeResult{std::move(declarations)};
}

void SemanticAnalyzer::visitScriptDeclaration(
    ast::ScriptDeclaration& statement) {
  (void)statement;
}

void SemanticAnalyzer::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& statement) {
  std::optional<VellumType> annotatedType;
  if (auto type = statement.typeName()) {
    annotatedType = resolveType(type.value());
    statement.typeName() = annotatedType;
  }

  std::optional<VellumType> deducedType;
  if (statement.initializer()) {
    deducedType = deduceType(statement.initializer());
    if (!annotatedType) {
      statement.typeName() = deducedType;
    }
  }

  if (annotatedType && deducedType) {
    if (annotatedType.value() != deducedType.value()) {
      errorHandler->errorAt(
          Token(),
          std::format("Variable type mismatch: got {}, expected {}.",
                      annotatedType->toString(), deducedType->toString()));
    }
  }

  resolver->addVariable(VellumVariable(VellumIdentifier(statement.name()),
                                       statement.typeName().value()));
}

void SemanticAnalyzer::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  std::vector<VellumVariable> parameters;
  parameters.reserve(declaration.getParameters().size());

  for (auto& param : declaration.getParameters()) {
    if (!param.type.isResolved()) {
      param.type = resolveType(param.type);
    }
    parameters.emplace_back(VellumIdentifier(param.name), param.type);
  }

  if (!declaration.getReturnTypeName().isResolved()) {
    declaration.getReturnTypeName() =
        resolveType(declaration.getReturnTypeName());
  }

  VellumFunction func(VellumIdentifier(declaration.getName().value()),
                      declaration.getReturnTypeName(), std::move(parameters));
  resolver->pushFunction(func);

  for (auto& statement : declaration.getBody()) {
    statement->accept(*this);
  }

  resolver->popFunction();
  resolver->addFunction(func);
}

void SemanticAnalyzer::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  if (!declaration.getTypeName().isResolved()) {
    declaration.getTypeName() = resolveType(declaration.getTypeName());
  }

  resolver->addProperty(VellumProperty(VellumIdentifier(declaration.getName()),
                                       declaration.getTypeName()));
}

void SemanticAnalyzer::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void SemanticAnalyzer::visitReturnStatement(ast::ReturnStatement& statement) {
  statement.getExpression()->accept(*this);
}

void SemanticAnalyzer::visitIdentifierExpression(
    ast::IdentifierExpression& expr) {
  // TODO: hack for testing, need to remove it
  if (expr.getIdentifier() == VellumIdentifier("Debug")) {
    return;
  }

  const auto value = resolver->resolveIdentifier(expr.getIdentifier());
  if (!value) {
    errorHandler->errorAt(Token(),
                          std::format("Undefined identifier '{}'",
                                      expr.getIdentifier().toString()));
    return;
  }

  switch (value->getType()) {
    case VellumValueType::Property:
      expr.setType(value->asProperty().getType());
      break;
    case VellumValueType::Variable:
      expr.setType(value->asVariable().getType());
      break;
    case VellumValueType::Function:
      expr.setType(value->asFunction().getReturnType());
      break;
    default:
      assert(false && "Unsuported identifier type");
  }
}

void SemanticAnalyzer::visitCallExpression(ast::CallExpression& expr) {
  expr.getCallee()->accept(*this);
  const VellumValue callee = expr.getCallee()->produceValue();

  switch (callee.getType()) {
    case VellumValueType::Identifier:
      expr.setFunctionCall(VellumFunctionCall(VellumIdentifier("self"),
                                              callee.asIdentifier(), false));
      break;
    case VellumValueType::PropertyAccess:
      expr.setFunctionCall(
          VellumFunctionCall(callee.asPropertyAccess().getObject(),
                             callee.asPropertyAccess().getProperty(), true));
      break;
    default:
      assert(false && "Unknown callee type");
      break;
  }

  for (auto& arg : expr.getArguments()) {
    arg->accept(*this);
  }
}

void SemanticAnalyzer::visitPropertyGetExpression(
    ast::PropertyGetExpression& expr) {
  expr.getObject()->accept(*this);

  const VellumIdentifier object =
      expr.getObject()->produceValue().asIdentifier();

  const auto property = resolver->resolveProperty(object, expr.getProperty());
  if (!property) {
    errorHandler->errorAt(Token(), std::format("Undefined property '{}'.",
                                               expr.getProperty().toString()));
    return;
  }

  switch (property->getType()) {
    case VellumValueType::Property:
      expr.setType(property->asProperty().getType());
      break;
    case VellumValueType::Function:
      expr.setType(property->asFunction().getReturnType());
      break;
    default:
      errorHandler->errorAt(Token(),
                            std::format("Property '{}' is not accessible.",
                                        expr.getProperty().toString()));
      break;
  }
}

void SemanticAnalyzer::visitPropertySetExpression(
    ast::PropertySetExpression& expr) {
  expr.getObject()->accept(*this);

  const auto property = resolver->resolveProperty(
      expr.getObject()->produceValue().asIdentifier(), expr.getProperty());
  if (!property) {
    errorHandler->errorAt(Token(), std::format("Undefined property '{}'.",
                                               expr.getProperty().toString()));
    return;
  }

  switch (property->getType()) {
    case VellumValueType::Property:
      expr.setType(property->asProperty().getType());
      break;
    default:
      errorHandler->errorAt(Token(),
                            std::format("'{}' is not assignable.",
                                        expr.getProperty().toString()));
      return;
  }

  expr.getValue()->accept(*this);

  if (expr.getType() != expr.getValue()->getType()) {
    errorHandler->errorAt(
        Token(),
        std::format("Can't assign a value of type '{}' to a property '{}' of "
                    "type '{}'.",
                    expr.getValue()->getType().toString(),
                    expr.getProperty().toString(), expr.getType().toString()));
    return;
  }
}

void SemanticAnalyzer::visitAssignExpression(ast::AssignExpression& expr) {
  const auto variable = resolver->resolveVariable(expr.getName());
  if (!variable) {
    errorHandler->errorAt(Token(), std::format("Undefined variable '{}'.",
                                               expr.getName().toString()));
    return;
  }

  expr.setType(variable->getType());
  expr.getValue()->accept(*this);

  if (expr.getType() != expr.getValue()->getType()) {
    errorHandler->errorAt(
        Token(),
        std::format("Can't assign a value of type '{}' to a variable '{}' of "
                    "type '{}'.",
                    expr.getValue()->getType().toString(),
                    expr.getName().toString(), expr.getType().toString()));
    return;
  }
}

void SemanticAnalyzer::visitBinaryExpression(ast::BinaryExpression& expr) {
  expr.getLeft()->accept(*this);
  expr.getRight()->accept(*this);

  const VellumType leftType = expr.getLeft()->getType();
  const VellumType rightType = expr.getRight()->getType();

  // For comparison operators, result is always boolean
  if (expr.getOperator() == ast::BinaryExpression::Operator::Equal ||
      expr.getOperator() == ast::BinaryExpression::Operator::NotEqual ||
      expr.getOperator() == ast::BinaryExpression::Operator::LessThan ||
      expr.getOperator() == ast::BinaryExpression::Operator::LessThanEqual ||
      expr.getOperator() == ast::BinaryExpression::Operator::GreaterThan ||
      expr.getOperator() == ast::BinaryExpression::Operator::GreaterThanEqual ||
      expr.getOperator() == ast::BinaryExpression::Operator::And ||
      expr.getOperator() == ast::BinaryExpression::Operator::Or) {
    expr.setType(VellumType::literal(VellumLiteralType::Bool));
    return;
  }

  // For arithmetic operators, both operands must be numeric
  if (expr.getOperator() == ast::BinaryExpression::Operator::Add ||
      expr.getOperator() == ast::BinaryExpression::Operator::Subtract ||
      expr.getOperator() == ast::BinaryExpression::Operator::Multiply ||
      expr.getOperator() == ast::BinaryExpression::Operator::Divide ||
      expr.getOperator() == ast::BinaryExpression::Operator::Modulo) {
    if (leftType != rightType && (leftType.isInt() || leftType.isFloat())) {
      errorHandler->errorAt(
          Token(), std::format("Cannot perform arithmetic operation on "
                               "types: {} and {}",
                               leftType.toString(), rightType.toString()));
      return;
    }
    // Result type is the same as the left operand
    expr.setType(leftType);
    return;
  }

  errorHandler->errorAt(Token(), "Unsupported binary operator");
}

VellumType SemanticAnalyzer::resolveType(VellumType unresolvedType) const {
  assert(!unresolvedType.isResolved());
  const std::string_view rawType = unresolvedType.asRawType();
  assert(!rawType.empty());

  VellumType type = VellumType::unresolved("");
  if (literalTypeFromString(unresolvedType.asRawType()).has_value()) {
    type = VellumType::literal(resolveValueType(unresolvedType.asRawType()));
  } else {
    type =
        VellumType::identifier(resolveObjectType(unresolvedType.asRawType()));
  }

  return type;
}

VellumType SemanticAnalyzer::deduceType(
    const std::unique_ptr<ast::Expression>& init) const {
  assert(init);
  return init->getType();
}

VellumLiteralType SemanticAnalyzer::resolveValueType(
    std::string_view rawType) const {
  return literalTypeFromString(rawType).value();
}

VellumIdentifier SemanticAnalyzer::resolveObjectType(
    std::string_view rawType) const {
  return VellumIdentifier(rawType);
}
}  // namespace vellum