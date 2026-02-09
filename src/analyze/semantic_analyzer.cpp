#include "semantic_analyzer.h"

#include <bsm/audit.h>

#include <cassert>
#include <optional>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_identifier.h"

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
  errorHandler->setCanEnterPanicMode(false);

  for (auto& declaration : declarations) {
    declaration->accept(*this);
  }

  errorHandler->setCanEnterPanicMode(true);

  return SemanticAnalyzeResult{std::move(declarations)};
}

void SemanticAnalyzer::visitImportDeclaration(
    ast::ImportDeclaration& declaration) {
  (void)declaration;
}

void SemanticAnalyzer::visitScriptDeclaration(ast::ScriptDeclaration& decl) {
  for (const auto& memberDecl : decl.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void SemanticAnalyzer::visitStateDeclaration(
    ast::StateDeclaration& declaration) {
  VellumIdentifier stateName(declaration.getStateName());
  state = resolver->getState(stateName);
  assert(state);

  for (const auto& func : state->getFunctions()) {
    stateFunc.emplace(func.getName(), func);
  }

  for (const auto& memberDecl : declaration.getMemberDecls()) {
    assert(memberDecl->getOrder() == ast::DeclarationOrder::Function);
    memberDecl->accept(*this);
  }

  state = std::nullopt;
  stateFunc.clear();
}

void SemanticAnalyzer::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& statement) {
  (void)statement;
}

void SemanticAnalyzer::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  assert(declaration.getName().has_value());
  VellumIdentifier funcName(declaration.getName().value());

  Opt<VellumFunction> func;
  if (state.has_value()) {
    func = state->getFunction(funcName);
  } else {
    func = resolver->getFunction(funcName);
  }

  assert(func.has_value());

  if (state.has_value()) {
    auto rootFunc = resolver->getFunction(funcName);
    if (!rootFunc) {
      auto nameLocation = declaration.getNameLocation();
      assert(nameLocation.has_value());

      errorHandler->errorAt(
          nameLocation.value(),
          "Function '{}' cannot be defined in state '{}' without also being "
          "defined in the empty state.",
          funcName.toString(), state->getName().toString());
      return;
    }

    if (!func->matchSignature(rootFunc.value())) {
      auto nameLocation = declaration.getNameLocation();
      assert(nameLocation.has_value());

      errorHandler->errorAt(
          nameLocation.value(),
          "The signature of function '{}' in state '{}' doesn't match the "
          "signature in the root state.",
          funcName.toString(), state->getName().toString());
      return;
    }
  }

  resolver->startFunction(func.value());
  for (auto& statement : declaration.getBody()) {
    statement->accept(*this);
  }
  resolver->endFunction();
}

void SemanticAnalyzer::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  VellumFunction dummyFunc(VellumIdentifier(declaration.getName()),
                           declaration.getTypeName(), {}, false);

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

  auto result = checker.check(statement.getExpression(), func->getReturnType(),
                              TypeChecker::Context::Return);
  if (!result.compatible) {
    errorHandler->errorAt(statement.getExpression()->getLocation(),
                          result.errorKind, result.errorMessage);
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
    Token typeLocation = statement.getTypeLocation().value_or(Token());
    annotatedType = resolver->resolveType(type.value(), typeLocation);
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

  if (annotatedType && deducedType && statement.getInitializer()) {
    auto result =
        checker.check(statement.getInitializer(), annotatedType.value(),
                      TypeChecker::Context::Initialization);
    if (!result.compatible) {
      errorHandler->errorAt(statement.getInitializer()->getLocation(),
                            result.errorKind, result.errorMessage);
      return;
    }
  }

  VellumVariable var(statement.getName(), statement.getType().value());
  Token varLocation = statement.getNameLocation();
  resolver->pushLocalVar(var, varLocation);
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
    if (callee->asPropertyGet().getObject()->isSuperExpression()) {
      if (!resolver->getParentType()) {
        errorHandler->errorAt(expr.getCallee()->getLocation(),
                              CompilerErrorKind::UndefinedFunction,
                              "super can only be used in a script that extends "
                              "another script.");
        return;
      }
      expr.setFunctionCall(VellumFunctionCall::parentCall(
          *resolver->getParentType(), callee->asPropertyGet().getProperty()));
    } else {
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
    }
  } else {
    assert(false && "Unknown callee type");
  }

  Opt<VellumFunction> func;
  if (expr.getFunctionCall()->isParentCall()) {
    func =
        resolver->resolveParentFunction(expr.getFunctionCall()->getFunction());
  } else {
    func = resolver->resolveFunction(expr.getFunctionCall()->getObjectType(),
                                     expr.getFunctionCall()->getFunction());
  }
  if (!func) {
    if (expr.getFunctionCall()->isParentCall()) {
      errorHandler->errorAt(expr.getCallee()->getLocation(),
                            CompilerErrorKind::UndefinedFunction,
                            "Parent has no member '{}'.",
                            expr.getFunctionCall()->getFunction().toString());
    } else {
      errorHandler->errorAt(expr.getCallee()->getLocation(),
                            CompilerErrorKind::UndefinedFunction,
                            "Undefined function '{}'.",
                            expr.getFunctionCall()->getFunction().toString());
    }
    return;
  }

  if (!expr.getFunctionCall()->isParentCall() &&
      expr.getFunctionCall()->isStatic() && !func->isStatic()) {
    errorHandler->errorAt(expr.getCallee()->getLocation(),
                          CompilerErrorKind::UndefinedFunction,
                          "Instance member '{}' cannot be used on type '{}'.",
                          func->getName().toString(),
                          expr.getFunctionCall()->getObjectType().toString());
    return;
  }

  if (func->getArity() != expr.getArguments().size()) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::FunctionArgumentsCountMismatch,
        "Function '{}' expects {} arguments, but {} were provided.",
        func->getName().toString(), func->getParameters().size(),
        expr.getArguments().size());
  }

  auto argsCount = std::min((int)expr.getArguments().size(), func->getArity());
  for (size_t i = 0; i < argsCount; i++) {
    auto& arg = expr.getArguments()[i];
    arg->accept(*this);

    std::string contextInfo =
        std::string(func->getName().toString()) + "," + std::to_string(i);
    auto result = checker.check(arg, func->getParameters()[i].getType(),
                                TypeChecker::Context::Argument, contextInfo);
    if (!result.compatible) {
      errorHandler->errorAt(arg->getLocation(), result.errorKind,
                            result.errorMessage);
      return;
    }
  }

  expr.setType(func->getReturnType());
}

void SemanticAnalyzer::visitPropertyGetExpression(
    ast::PropertyGetExpression& expr) {
  expr.getObject()->accept(*this);

  Opt<VellumValue> property;
  if (expr.getObject()->isSuperExpression()) {
    if (!resolver->getParentType()) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::UndefinedProperty,
          "super can only be used in a script that extends another script.");
      return;
    }
    if (auto func = resolver->resolveParentFunction(expr.getProperty())) {
      expr.setType(func->getReturnType());
      expr.setIdentifierType(VellumValueType::Function);
      return;
    }
    property = resolver->resolveParentProperty(expr.getProperty());
    if (!property) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::UndefinedProperty,
          "Parent has no member '{}'.", expr.getProperty().toString());
      return;
    }
    expr.setType(property->asProperty().getType());
    expr.setIdentifierType(VellumValueType::Property);
    return;
  } else {
    property = resolver->resolveProperty(expr.getObject()->getType(),
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
      expr.setIdentifierType(VellumValueType::Property);
      break;
    case VellumValueType::Function:
      expr.setType(property->asFunction().getReturnType());
      expr.setIdentifierType(VellumValueType::Function);
      break;
    case VellumValueType::ScriptType:
      expr.setType(property->asScriptType());
      expr.setIdentifierType(VellumValueType::ScriptType);
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

  Opt<VellumValue> property;
  if (expr.getObject()->isSuperExpression()) {
    if (!resolver->getParentType()) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::UndefinedProperty,
          "super can only be used in a script that extends another script.");
      return;
    }
    property = resolver->resolveParentProperty(expr.getProperty());
  } else {
    property = resolver->resolveProperty(expr.getObject()->getType(),
                                         expr.getProperty());
  }

  if (!property) {
    if (expr.getObject()->isSuperExpression()) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::UndefinedProperty,
          "Parent has no member '{}'.", expr.getProperty().toString());
    } else {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::UndefinedProperty,
          "Undefined property '{}'.", expr.getProperty().toString());
    }
    return;
  }

  switch (property->getType()) {
    case VellumValueType::Property:
      if (property->asProperty().isReadonly()) {
        errorHandler->errorAt(expr.getLocation(),
                              CompilerErrorKind::NotAssignable,
                              "Can't assign to readonly property '{}'",
                              expr.getProperty().toString());
      }
      expr.setType(property->asProperty().getType());
      break;
    default:
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::NotAssignable,
          "'{}' is not assignable.", expr.getProperty().toString());
      return;
  }

  expr.getValue()->accept(*this);

  std::string contextInfo =
      "property," + std::string(expr.getProperty().toString());
  auto result = checker.check(expr.getValue(), expr.getType(),
                              TypeChecker::Context::Assignment, contextInfo);
  if (!result.compatible) {
    errorHandler->errorAt(expr.getValue()->getLocation(), result.errorKind,
                          result.errorMessage);
    return;
  }
}

void SemanticAnalyzer::visitAssignExpression(ast::AssignExpression& expr) {
  const auto identifier = resolver->resolveIdentifier(expr.getName());
  if (!identifier) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::UndefinedIdentifier,
        "Undefined identifier '{}'.", expr.getName().toString());
    return;
  }

  switch (identifier->getType()) {
    case VellumValueType::Property:
      if (identifier->asProperty().isReadonly()) {
        errorHandler->errorAt(expr.getLocation(),
                              CompilerErrorKind::NotAssignable,
                              "Can't assign to readonly property '{}'",
                              expr.getName().toString());
      }
      expr.setType(identifier->asProperty().getType());
      break;

    case VellumValueType::Variable:
      expr.setType(identifier->asVariable().getType());
      break;
    default:
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::NotAssignable,
          "'{}' is not assignable.", expr.getName().toString());
      return;
  }

  expr.getValue()->accept(*this);

  std::string contextInfo =
      "variable," + std::string(expr.getName().toString());
  auto result = checker.check(expr.getValue(), expr.getType(),
                              TypeChecker::Context::Assignment, contextInfo);
  if (!result.compatible) {
    errorHandler->errorAt(expr.getValue()->getLocation(), result.errorKind,
                          result.errorMessage);
    return;
  }
}

void SemanticAnalyzer::visitBinaryExpression(ast::BinaryExpression& expr) {
  expr.getLeft()->accept(*this);
  expr.getRight()->accept(*this);

  // Check if operands are valid values (not function/script type identifiers)
  auto leftResult = checker.checkValidValue(
      expr.getLeft(), TypeChecker::Context::BinaryOperand);
  if (!leftResult.compatible) {
    errorHandler->errorAt(expr.getLeft()->getLocation(), leftResult.errorKind,
                          leftResult.errorMessage);
    return;
  }

  auto rightResult = checker.checkValidValue(
      expr.getRight(), TypeChecker::Context::BinaryOperand);
  if (!rightResult.compatible) {
    errorHandler->errorAt(expr.getRight()->getLocation(), rightResult.errorKind,
                          rightResult.errorMessage);
    return;
  }

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

  std::string operatorName;
  switch (expr.getOperator()) {
    case ast::UnaryExpression::Operator::Negate:
      operatorName = "-";
      break;
    case ast::UnaryExpression::Operator::Not:
      operatorName = "not";
      break;
  }

  auto operandResult = checker.checkValidValue(
      expr.getOperand(), TypeChecker::Context::UnaryOperand, operatorName);
  if (!operandResult.compatible) {
    errorHandler->errorAt(expr.getOperand()->getLocation(),
                          operandResult.errorKind, operandResult.errorMessage);
    return;
  }

  expr.setType(expr.getOperand()->getType());

  switch (expr.getOperator()) {
    case ast::UnaryExpression::Operator::Negate:
      if (!(expr.getType().isInt() || expr.getType().isFloat())) {
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
  expr.setTargetType(
      resolver->resolveType(expr.getTargetType(), expr.getLocation()));
}

void SemanticAnalyzer::visitNewArrayExpression(ast::NewArrayExpression& expr) {
  if (const auto& subtype = expr.getSubtype(); subtype.has_value()) {
    VellumType resolvedSubtype =
        resolver->resolveType(subtype.value(), expr.getLocation());
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

void SemanticAnalyzer::visitSuperExpression(ast::SuperExpression&) {}
}  // namespace vellum