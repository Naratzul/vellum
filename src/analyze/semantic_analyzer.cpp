#include "semantic_analyzer.h"

#include <cassert>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "declaration_collector.h"

namespace vellum {
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

SemanticAnalyzer::SemanticAnalyzer(Shared<CompilerErrorHandler> errorHandler,
                                   Shared<Resolver> resolver,
                                   std::string_view scriptFilename)
    : errorHandler(errorHandler),
      resolver(resolver),
      scriptFilename(std::move(scriptFilename)) {
  assert(resolver);
}

SemanticAnalyzeResult SemanticAnalyzer::analyze(
    Vec<Unique<ast::Declaration>>&& declarations) {
  DeclarationCollector collector(errorHandler, resolver, scriptFilename);
  collector.collect(declarations);

  errorHandler->setCanEnterPanicMode(false);

  for (auto& declaration : declarations) {
    declaration->accept(*this);
  }

  errorHandler->setCanEnterPanicMode(true);

  return SemanticAnalyzeResult{std::move(declarations)};
}

void SemanticAnalyzer::visitScriptDeclaration(ast::ScriptDeclaration& decl) {
  for (const auto& memberDecl : decl.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void SemanticAnalyzer::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& statement) {
  (void)statement;
}

void SemanticAnalyzer::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  assert(declaration.getName().has_value());
  Opt<VellumFunction> func =
      resolver->getFunction(VellumIdentifier(declaration.getName().value()));

  assert(func.has_value());

  resolver->startFunction(func.value());
  for (auto& statement : declaration.getBody()) {
    statement->accept(*this);
  }
  resolver->endFunction();
}

void SemanticAnalyzer::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  VellumFunction dummyFunc(VellumIdentifier(declaration.getName()),
                           declaration.getTypeName(), {});

  if (auto getFunc = declaration.getGetAccessor()) {
    resolver->startFunction(dummyFunc);
    for (auto& statement : getFunc.value()) {
      statement->accept(*this);
    }
    resolver->endFunction();
  }

  if (auto setFunc = declaration.getSetAccessor()) {
    resolver->startFunction(dummyFunc);
    for (auto& statement : setFunc.value()) {
      statement->accept(*this);
    }
    resolver->endFunction();
  }
}

void SemanticAnalyzer::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void SemanticAnalyzer::visitReturnStatement(ast::ReturnStatement& statement) {
  statement.getExpression()->accept(*this);
  const auto& func = resolver->getCurrentFunction();
  assert(func.has_value());
  if (statement.getExpression()->getType() != func->getReturnType()) {
    std::string gotType;
    if (statement.getExpression()->getType().isResolved()) {
      gotType = std::format("Got type is '{}'.",
                            statement.getExpression()->getType().toString());
    }
    errorHandler->errorAt(statement.getExpression()->getLocation(),
                          CompilerErrorKind::ReturnTypeMismatch,
                          "Return type mismatch. Expected type is '{}'. {}",
                          func->getReturnType().toString(), gotType);
    return;
  }
}

void SemanticAnalyzer::visitIfStatement(ast::IfStatement& statement) {
  statement.getCondition()->accept(*this);

  resolver->pushScope();
  for (const auto& stmt : statement.getThenBlock()) {
    stmt->accept(*this);
  }
  resolver->popScope();

  if (statement.getElseBlock().has_value()) {
    resolver->pushScope();
    for (const auto& stmt : statement.getElseBlock().value()) {
      stmt->accept(*this);
    }
    resolver->popScope();
  }
}

void SemanticAnalyzer::visitLocalVariableStatement(
    ast::LocalVariableStatement& statement) {
  Opt<VellumType> annotatedType;
  if (auto type = statement.getType()) {
    annotatedType = resolver->resolveType(type.value());
    statement.setType(annotatedType.value());
  }

  Opt<VellumType> deducedType;
  if (statement.getInitializer()) {
    statement.getInitializer()->accept(*this);
    deducedType = statement.getInitializer()->getType();
    if (!annotatedType) {
      statement.setType(deducedType.value());
    }
  }

  if (annotatedType && deducedType) {
    if (annotatedType.value() != deducedType.value()) {
      std::string gotType;
      if (deducedType->isResolved()) {
        gotType = std::format(", got '{}'.", deducedType->toString());
      }

      errorHandler->errorAt(statement.getInitializer()->getLocation(),
                            CompilerErrorKind::VariableTypeMismatch,
                            "Variable type mismatch: expected '{}'{}.",
                            annotatedType->toString(), gotType);
    }
  }

  VellumVariable var(statement.getName(), statement.getType().value());
  resolver->pushLocalVar(var);
}

void SemanticAnalyzer::visitWhileStatement(ast::WhileStatement& statement) {
  statement.getCondition()->accept(*this);
  for (auto& stmt : statement.getBody()) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visitIdentifierExpression(
    ast::IdentifierExpression& expr) {
  const auto value = resolver->resolveIdentifier(expr.getIdentifier());
  if (!value) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::UndefinedIdentifier,
        "Undefined identifier '{}'", expr.getIdentifier().toString());
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
    case VellumValueType::ScriptType:
      expr.setType(value->asScriptType());
      break;
    default:
      assert(false && "Unsupported identifier type");
  }

  expr.setIdentifierType(value->getType());
}

void SemanticAnalyzer::visitCallExpression(ast::CallExpression& expr) {
  expr.getCallee()->accept(*this);
  const auto& callee = expr.getCallee();

  if (callee->isIdentifierExpression()) {
    expr.setFunctionCall(VellumFunctionCall::methodCall(
        VellumIdentifier("self"), resolver->getObject().getType(),
        callee->asIdentifier().getIdentifier()));
  } else if (callee->isPropertyGetExpression()) {
    const auto& calleeIdentifier =
        callee->asPropertyGet().getObject()->asIdentifier();

    if (calleeIdentifier.getIdentifierType() == VellumValueType::ScriptType) {
      expr.setFunctionCall(VellumFunctionCall::staticCall(
          calleeIdentifier.getType(), callee->asPropertyGet().getProperty()));
    } else {
      expr.setFunctionCall(VellumFunctionCall::methodCall(
          calleeIdentifier.getIdentifier(), calleeIdentifier.getType(),
          callee->asPropertyGet().getProperty()));
    }

  } else {
    assert(false && "Unknown callee type");
  }

  const auto& func =
      resolver->resolveFunction(expr.getFunctionCall()->getObjectType(),
                                expr.getFunctionCall()->getFunction());
  if (!func) {
    errorHandler->errorAt(expr.getCallee()->getLocation(),
                          CompilerErrorKind::UndefinedFunction,
                          "Undefined function '{}'.",
                          expr.getFunctionCall()->getFunction().toString());
    return;
  }

  if (func->getParameters().size() != expr.getArguments().size()) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::FunctionArgumentsCountMismatch,
        "Function '{}' expects {} arguments, but {} were provided.",
        func->getName().toString(), func->getParameters().size(),
        expr.getArguments().size());
    return;
  }

  for (size_t i = 0; i < expr.getArguments().size(); i++) {
    auto& arg = expr.getArguments()[i];
    arg->accept(*this);
    if (arg->getType() != func->getParameters()[i].getType()) {
      errorHandler->errorAt(
          arg->getLocation(), CompilerErrorKind::FunctionArgumentTypeMismatch,
          "Argument {} of function '{}' expects type '{}', but got type '{}'.",
          i, func->getName().toString(),
          func->getParameters()[i].getType().toString(),
          arg->getType().toString());
      return;
    }
  }

  expr.setType(func->getReturnType());
}

void SemanticAnalyzer::visitPropertyGetExpression(
    ast::PropertyGetExpression& expr) {
  expr.getObject()->accept(*this);

  auto property = resolver->resolveProperty(expr.getObject()->getType(),
                                            expr.getProperty());

  if (!property) {
    property = resolver->resolveFunction(expr.getObject()->getType(),
                                         expr.getProperty());
  }

  if (!property) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::UndefinedProperty,
        "Undefined property '{}'.", expr.getProperty().toString());
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
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::PropertyNotAccessible,
          "Property '{}' is not accessible.", expr.getProperty().toString());
      break;
  }
}

void SemanticAnalyzer::visitPropertySetExpression(
    ast::PropertySetExpression& expr) {
  expr.getObject()->accept(*this);

  auto property = resolver->resolveProperty(expr.getObject()->getType(),
                                            expr.getProperty());

  if (!property) {
    property = resolver->resolveFunction(expr.getObject()->getType(),
                                         expr.getProperty());
  }

  if (!property) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::UndefinedProperty,
        "Undefined property '{}'.", expr.getProperty().toString());
    return;
  }

  switch (property->getType()) {
    case VellumValueType::Property:
      expr.setType(property->asProperty().getType());
      break;
    default:
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::NotAssignable,
          "'{}' is not assignable.", expr.getProperty().toString());
      return;
  }

  // TODO: check if value is assignable

  expr.getValue()->accept(*this);

  if (expr.getType() != expr.getValue()->getType()) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::AssignTypeMismatch,
        "Can't assign a value of type '{}' to a property '{}' of type '{}'.",
        expr.getValue()->getType().toString(), expr.getProperty().toString(),
        expr.getType().toString());
    return;
  }
}

void SemanticAnalyzer::visitAssignExpression(ast::AssignExpression& expr) {
  const auto variable = resolver->resolveVariable(expr.getName());
  if (!variable) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::UndefinedVariable,
        "Undefined variable '{}'.", expr.getName().toString());
    return;
  }

  expr.setType(variable->getType());
  expr.getValue()->accept(*this);

  if (expr.getType() != expr.getValue()->getType()) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::AssignTypeMismatch,
        "Can't assign a value of type '{}' to a variable '{}' of type '{}'.",
        expr.getValue()->getType().toString(), expr.getName().toString(),
        expr.getType().toString());
    return;
  }
}

void SemanticAnalyzer::visitBinaryExpression(ast::BinaryExpression& expr) {
  expr.getLeft()->accept(*this);
  expr.getRight()->accept(*this);

  const VellumType leftType = expr.getLeft()->getType();
  const VellumType rightType = expr.getRight()->getType();

  if (expr.getOperator() == ast::BinaryExpression::Operator::And ||
      expr.getOperator() == ast::BinaryExpression::Operator::Or) {
    if (!(leftType.isBool() && rightType.isBool())) {
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::LogicalOperationTypeMismatch,
                            "Cannot perform logical operation on types: '{}' "
                            "and '{}'. Both operands must be of Bool type.",
                            leftType.toString(), rightType.toString());
      return;
    }
    expr.setType(VellumType::literal(VellumLiteralType::Bool));
    return;
  }

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

  // string concat
  if (expr.getOperator() == ast::BinaryExpression::Operator::Add &&
      leftType.isString() && rightType.isString()) {
    expr.setType(VellumType::literal(VellumLiteralType::String));
    return;
  }

  // For arithmetic operators, both operands must be numeric
  if (expr.getOperator() == ast::BinaryExpression::Operator::Add ||
      expr.getOperator() == ast::BinaryExpression::Operator::Subtract ||
      expr.getOperator() == ast::BinaryExpression::Operator::Multiply ||
      expr.getOperator() == ast::BinaryExpression::Operator::Divide ||
      expr.getOperator() == ast::BinaryExpression::Operator::Modulo) {
    if (leftType != rightType || !(leftType.isInt() || leftType.isFloat())) {
      errorHandler->errorAt(
          expr.getLocation(),
          CompilerErrorKind::ArithmeticOperationTypeMismatch,
          "Cannot perform arithmetic operation on types: '{}' and '{}'",
          leftType.toString(), rightType.toString());
      return;
    }
    // Result type is the same as the left operand
    expr.setType(leftType);
    return;
  }

  errorHandler->errorAt(expr.getLocation(),
                        CompilerErrorKind::UnsupportedBinaryOperator,
                        "Unsupported binary operator");
}

void SemanticAnalyzer::visitUnaryExpression(ast::UnaryExpression& expr) {
  expr.getOperand()->accept(*this);
  expr.setType(expr.getOperand()->getType());

  switch (expr.getOperator()) {
    case ast::UnaryExpression::Operator::Negate:
      if (!expr.getType().isInt() || expr.getType().isFloat()) {
        errorHandler->errorAt(
            expr.getOperand()->getLocation(),
            CompilerErrorKind::UnaryOperatorTypeMismatch,
            "Unary operator '-' can be applied only to expressions with "
            "numeric types (Int, Float). Got type is '{}'",
            expr.getType().toString());
      }
      break;
    case ast::UnaryExpression::Operator::Not:
      if (!expr.getType().isBool()) {
        errorHandler->errorAt(expr.getOperand()->getLocation(),
                              CompilerErrorKind::UnaryOperatorTypeMismatch,
                              "Unary operator 'not' can be applied only to "
                              "expressions with Bool type. Got type is '{}'",
                              expr.getType().toString());
      }
      break;
  }
}

void SemanticAnalyzer::visitCastExpression(ast::CastExpression& expr) {
  expr.getExpression()->accept(*this);

  assert(!expr.getTargetType().isResolved());
  expr.setTargetType(resolver->resolveType(expr.getTargetType()));
}

void SemanticAnalyzer::visitNewArrayExpression(ast::NewArrayExpression& expr) {
  if (const auto& subtype = expr.getSubtype(); subtype.has_value()) {
    VellumType resolvedSubtype = resolver->resolveType(subtype.value());
    expr.setSubtype(resolvedSubtype);
    expr.setType(VellumType::array(resolvedSubtype));
  }

  const auto& length = expr.getLength();
  if (length.getType() != VellumLiteralType::Int) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::ArrayLengthTypeMismatch,
        "Array length is expected to be 'Int'. Got type is '{}'",
        literalTypeToString(length.getType()));
  }

  const int lengthInt = length.asInt();

  if (lengthInt < 0) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::ArrayLengthMustBePositive,
                          "Array length must be a positive number.");
  } else if (lengthInt > 128) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::ArrayLengthMaximumExceeded,
                          "Maximum array length (128) is exceeded.");
  }
}
}  // namespace vellum