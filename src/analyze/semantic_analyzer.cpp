#include "semantic_analyzer.h"

#include <cassert>
#include <format>
#include <optional>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "common/string_set.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lexer/token.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_property.h"

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

  if (state->isAuto() && !visitedStates.contains(state->getName())) {
    autoStateCount++;
  }

  visitedStates.insert(state->getName());

  if (autoStateCount > 1) {
    errorHandler->errorAt(declaration.getStateNameLocation(),
                          CompilerErrorKind::MultipleAutoStates,
                          "Only one auto state is allowed.");
    state = std::nullopt;
    return;
  }

  for (const auto& memberDecl : declaration.getMemberDecls()) {
    assert(memberDecl->getOrder() == ast::DeclarationOrder::Function);
    memberDecl->accept(*this);
  }

  state = std::nullopt;
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
          nameLocation.value(), CompilerErrorKind::StateFunctionNotInEmptyState,
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
          CompilerErrorKind::StateFunctionSignatureMismatch,
          "The signature of function '{}' in state '{}' doesn't match the "
          "signature in the root state.",
          funcName.toString(), state->getName().toString());
      return;
    }
  }

  inStaticContext = declaration.isStatic();
  resolver->startFunction(func.value());
  declaration.getBody()->accept(*this);
  resolver->endFunction();
  inStaticContext = false;
  loopCount = 0;
}

void SemanticAnalyzer::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  VellumFunction dummyFunc(VellumIdentifier(declaration.getName()),
                           declaration.getTypeName(), {}, false);

  if (auto& getBlock = declaration.getGetAccessor()) {
    resolver->startFunction(dummyFunc);
    getBlock.value()->accept(*this);
    resolver->endFunction();
  }

  if (auto& setBlock = declaration.getSetAccessor()) {
    resolver->startFunction(dummyFunc);
    setBlock.value()->accept(*this);
    resolver->endFunction();
  }
}

void SemanticAnalyzer::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void SemanticAnalyzer::visitReturnStatement(ast::ReturnStatement& statement) {
  const auto& func = resolver->getCurrentFunction();
  assert(func.has_value());

  if (!statement.getExpression()) {
    if (!func->getReturnType().isNone()) {
      errorHandler->errorAt(
          statement.getLocation(), CompilerErrorKind::ReturnTypeMismatch,
          "Function expects return type '{}' but no value was returned.",
          func->getReturnType().toString());
    }
    return;
  }

  statement.getExpression()->accept(*this);
  auto result = checker.check(statement.getExpression(), func->getReturnType(),
                              TypeChecker::Context::Return);
  if (!result.compatible) {
    errorHandler->errorAt(statement.getExpression()->getLocation(),
                          result.errorKind, result.errorMessage);
    return;
  }
}

void SemanticAnalyzer::visitIfStatement(ast::IfStatement& statement) {
  auto& condition = statement.getCondition();
  condition->accept(*this);

  if (!condition->hasResolvedType()) {
    return;
  }

  if (!condition->getType().isBool()) {
    errorHandler->errorAt(
        statement.getCondition()->getLocation(),
        "Condition expression must have 'Bool' type. Given type is '{}'.",
        condition->getType().toString());
    return;
  }

  resolver->pushScope();
  statement.getThenBlock()->accept(*this);
  resolver->popScope();

  if (auto& elseBlock = statement.getElseBlock()) {
    resolver->pushScope();
    elseBlock->accept(*this);
    resolver->popScope();
  }
}

void SemanticAnalyzer::visitLocalVariableStatement(
    ast::LocalVariableStatement& statement) {
  Opt<VellumType> annotatedType;
  if (auto type = statement.getType()) {
    Token typeLocation = statement.getTypeLocation().value_or(Token());
    auto resolvedType = resolver->resolveType(type.value(), typeLocation);
    statement.setType(resolvedType);
    if (resolvedType.isNone()) {
      errorHandler->errorAt(typeLocation, CompilerErrorKind::NoneNotValidType,
                            "None is not a valid type for a variable.");
      return;
    }
    annotatedType = resolvedType;
  }

  Opt<VellumType> deducedType;
  if (statement.getInitializer()) {
    statement.getInitializer()->accept(*this);
    if (statement.getInitializer()->hasResolvedType()) {
      deducedType = statement.getInitializer()->getType();
      if (!annotatedType) {
        statement.setType(*deducedType);
      }
    }
  }

  if (annotatedType && deducedType && annotatedType->isResolved() &&
      deducedType->isResolved()) {
    auto result =
        checker.check(statement.getInitializer(), annotatedType.value(),
                      TypeChecker::Context::Initialization);
    if (!result.compatible) {
      errorHandler->errorAt(statement.getInitializer()->getLocation(),
                            result.errorKind, result.errorMessage);
    }
  }

  if (auto type = statement.getType()) {
    VellumVariable var(statement.getName(), *type);
    Token varLocation = statement.getNameLocation();
    resolver->pushLocalVar(var, varLocation);
  }
}

void SemanticAnalyzer::visitWhileStatement(ast::WhileStatement& statement) {
  statement.getCondition()->accept(*this);
  if (statement.getCondition()->hasResolvedType() &&
      !statement.getCondition()->getType().isBool()) {
    errorHandler->errorAt(
        statement.getCondition()->getLocation(),
        "Condition expression must have 'Bool' type. Given type is '{}'.",
        statement.getCondition()->getType().toString());
  }

  loopDepth++;
  statement.getBody()->accept(*this);
  loopDepth--;
}

void SemanticAnalyzer::visitBreakStatement(ast::BreakStatement& statement) {
  if (loopDepth == 0) {
    errorHandler->errorAt(statement.getLocation(),
                          CompilerErrorKind::BreakOutsideLoop,
                          "'break' is only allowed inside a loop.");
  }
}

void SemanticAnalyzer::visitContinueStatement(
    ast::ContinueStatement& statement) {
  if (loopDepth == 0) {
    errorHandler->errorAt(statement.getLocation(),
                          CompilerErrorKind::ContinueOutsideLoop,
                          "'continue' is only allowed inside a loop.");
  }
}

void SemanticAnalyzer::visitForStatement(ast::ForStatement& statement) {
  auto& arrayExpr = statement.getArray();
  arrayExpr->accept(*this);

  if (!arrayExpr->hasResolvedType()) {
    return;
  }

  if (!arrayExpr->getType().isArray()) {
    errorHandler->errorAt(statement.getArrayLocation(), "Array type expected.");
    return;
  }

  loopCount++;

  auto variableName =
      statement.getVariableName()->asIdentifier().getIdentifier();
  VellumVariable loopVar(variableName, *arrayExpr->getType().asArraySubtype());

  VellumIdentifier pexName(mangleLoopVariable(variableName.getValue()));
  loopVar.setPexName(pexName);

  resolver->pushScope();
  resolver->pushLocalVar(loopVar, statement.getVariableNameLocation());

  statement.getVariableName()->accept(*this);

  statement.setCounterMangledName(
      mangleLoopVariable(std::format("{}_index", variableName.getValue())));

  loopDepth++;
  statement.getBody()->accept(*this);
  loopDepth--;

  resolver->popLocalVar();
  resolver->popScope();
}

void SemanticAnalyzer::visitBlockStatement(ast::BlockStatement& statement) {
  for (auto& stmt : statement.getStatements()) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visitIdentifierExpression(
    ast::IdentifierExpression& expr) {
  const auto value = resolver->resolveIdentifier(expr.getIdentifier());
  if (!value) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::UndefinedIdentifier,
        "Undefined identifier '{}'.", expr.getIdentifier().toString());
    return;
  }

  if (inStaticContext && resolver->isInstanceMember(expr.getIdentifier())) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::InstanceMemberInStaticContext,
                          "Instance member '{}' is not allowed in a static "
                          "function.",
                          expr.getIdentifier().toString());
    return;
  }

  switch (value->getType()) {
    case VellumValueType::Property:
      expr.setType(value->asProperty().getType());
      break;
    case VellumValueType::Variable:
      expr.setType(value->asVariable().getType());
      if (auto pexName = value->asVariable().getPexName()) {
        expr.setMangledIdentifier(*pexName);
      }
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
  if (!expr.getCallee()->hasResolvedType()) {
    return;
  }

  const auto& callee = expr.getCallee();

  if (callee->isIdentifierExpression()) {
    expr.setFunctionCall(VellumFunctionCall::methodCall(
        VellumIdentifier("self"), resolver->getObject().getType(),
        callee->asIdentifier().getIdentifier()));
  } else if (callee->isPropertyGetExpression()) {
    if (callee->asPropertyGet().getObject()->isSuperExpression()) {
      expr.setFunctionCall(VellumFunctionCall::parentCall(
          *resolver->getParentType(), callee->asPropertyGet().getProperty()));
    } else {
      const VellumType objectType =
          callee->asPropertyGet().getObject()->getType();

      if (callee->asPropertyGet().getObject()->isSelfExpression()) {
        expr.setFunctionCall(VellumFunctionCall::methodCall(
            VellumIdentifier("self"), resolver->getObject().getType(),
            callee->asPropertyGet().getProperty()));
      } else if (callee->asPropertyGet()
                     .getObject()
                     ->isIdentifierExpression()) {
        const auto& calleeIdentifier =
            callee->asPropertyGet().getObject()->asIdentifier();

        if (calleeIdentifier.getIdentifierType() ==
            VellumValueType::ScriptType) {
          expr.setFunctionCall(VellumFunctionCall::staticCall(
              calleeIdentifier.getType(),
              callee->asPropertyGet().getProperty()));
        } else {
          expr.setFunctionCall(VellumFunctionCall::methodCall(
              calleeIdentifier.getIdentifier(), objectType,
              callee->asPropertyGet().getProperty()));
        }
      } else {
        VellumIdentifier objectName("self");
        expr.setFunctionCall(VellumFunctionCall::methodCall(
            objectName, objectType, callee->asPropertyGet().getProperty()));
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

  if (inStaticContext && callee->isIdentifierExpression() &&
      !func->isStatic()) {
    errorHandler->errorAt(expr.getCallee()->getLocation(),
                          CompilerErrorKind::InstanceMemberInStaticContext,
                          "Instance member '{}' is not allowed in a static "
                          "function.",
                          func->getName().toString());
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

  bool isArrayIntrinsic =
      func->getIntrinsicKind() == IntrinsicKind::ArrayFind ||
      func->getIntrinsicKind() == IntrinsicKind::ArrayRFind;

  if (!isArrayIntrinsic) {
    auto requiredArity = static_cast<size_t>(func->getArity());
    for (size_t i = 0; i < static_cast<size_t>(func->getArity()); ++i) {
      if (func->getParameters()[i].getDefaultValue().has_value()) {
        requiredArity = i;
        break;
      }
    }

    if (expr.getArguments().size() < requiredArity) {
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::FunctionArgumentsCountMismatch,
                            "Function '{}' expects at least {} arguments, "
                            "but {} were provided.",
                            func->getName().toString(), requiredArity,
                            expr.getArguments().size());
      return;
    }

    if (expr.getArguments().size() > static_cast<size_t>(func->getArity())) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::FunctionArgumentsCountMismatch,
          "Function '{}' expects {} arguments, but {} were provided.",
          func->getName().toString(), func->getArity(),
          expr.getArguments().size());
      return;
    }

    while (expr.getArguments().size() < static_cast<size_t>(func->getArity())) {
      size_t idx = expr.getArguments().size();
      const auto& defaultVal =
          func->getParameters()[idx].getDefaultValue().value();
      expr.addArgument(common::makeUnique<ast::LiteralExpression>(
          defaultVal, expr.getLocation()));
    }
  }

  if (isArrayIntrinsic) {
    if (expr.getArguments().size() < 1 || expr.getArguments().size() > 2) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::FunctionArgumentsCountMismatch,
          "Array '{}' expects 1-2 arguments, but {} were provided.",
          func->getName().toString(), expr.getArguments().size());
      return;
    }
  }

  auto argsCount = expr.getArguments().size();
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
    }
  }

  if (!isArrayIntrinsic && argsCount > 0) {
    Vec<VellumType> paramTypes;
    for (size_t i = 0; i < argsCount; i++)
      paramTypes.push_back(func->getParameters()[i].getType());
    expr.setArgumentTypes(std::move(paramTypes));
  }

  expr.setType(func->getReturnType());
}

void SemanticAnalyzer::visitPropertyGetExpression(
    ast::PropertyGetExpression& expr) {
  expr.getObject()->accept(*this);
  if (!expr.getObject()->hasResolvedType()) {
    return;
  }

  if (inStaticContext) {
    if (expr.getObject()->isSuperExpression()) {
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::InstanceMemberInStaticContext,
                            "super is not allowed in a static function.");
      return;
    }
    if (expr.getObject()->isSelfExpression()) {
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::InstanceMemberInStaticContext,
                            "Instance member '{}' is not allowed in a static "
                            "function.",
                            expr.getProperty().toString());
      return;
    }
  }

  Opt<VellumValue> property;
  if (expr.getObject()->isSuperExpression()) {
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
    const VellumType objectType = expr.getObject()->getType();

    property = resolver->resolveProperty(objectType, expr.getProperty());
    if (objectType.isArray() && !property) {
      property = resolver->resolveFunction(objectType, expr.getProperty());
    }
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
  if (!expr.getObject()->hasResolvedType()) {
    return;
  }

  if (inStaticContext) {
    if (expr.getObject()->isSuperExpression()) {
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::InstanceMemberInStaticContext,
                            "super is not allowed in a static function.");
      return;
    }
    if (expr.getObject()->isSelfExpression()) {
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::InstanceMemberInStaticContext,
                            "Instance member '{}' is not allowed in a static "
                            "function.",
                            expr.getProperty().toString());
      return;
    }
  }

  Opt<VellumValue> property;
  if (expr.getObject()->isSuperExpression()) {
    property = resolver->resolveParentProperty(expr.getProperty());
  } else {
    property = resolver->resolveProperty(expr.getObject()->getType(),
                                         expr.getProperty());
  }

  if (!property) {
    if (expr.getObject()->isSuperExpression()) {
      errorHandler->errorAt(
          expr.getPropertyLocation(), CompilerErrorKind::UndefinedProperty,
          "Parent has no member '{}'.", expr.getProperty().toString());
    } else {
      errorHandler->errorAt(
          expr.getPropertyLocation(), CompilerErrorKind::UndefinedProperty,
          "Undefined property '{}'.", expr.getProperty().toString());
    }
    return;
  }

  switch (property->getType()) {
    case VellumValueType::Property:
      if (property->asProperty().getIntrinsicKind() ==
          IntrinsicKind::ArrayLength) {
        errorHandler->errorAt(
            expr.getLocation(), CompilerErrorKind::NotAssignable,
            "Cannot assign to the 'length' property of an array.");
        return;
      }
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

  if (expr.getOperator() != ast::AssignOperator::Assign) {
    if (!validateComposedAssignTypes(expr.getOperator(), expr.getType(),
                                     expr.getValue()->getType(),
                                     expr.getLocation())) {
      return;
    }
  }

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
  expr.getName()->accept(*this);

  if (!expr.getName()->hasResolvedType()) {
    return;
  }

  if (!expr.getName()->isIdentifierExpression()) {
    errorHandler->errorAt(expr.getName()->getLocation(),
                          CompilerErrorKind::NotAssignable,
                          "Invalid assignment target.");
    return;
  }

  VellumIdentifier identifier(expr.getName()->asIdentifier().getIdentifier());
  const auto value = resolver->resolveIdentifier(identifier);
  if (!value) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::UndefinedIdentifier,
                          "Undefined identifier '{}'.", identifier.toString());
    return;
  }

  if (inStaticContext && resolver->isInstanceMember(identifier)) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::InstanceMemberInStaticContext,
                          "Instance member '{}' is not allowed in a static "
                          "function.",
                          identifier.toString());
    return;
  }

  switch (value->getType()) {
    case VellumValueType::Property:
      if (value->asProperty().isReadonly()) {
        errorHandler->errorAt(
            expr.getLocation(), CompilerErrorKind::NotAssignable,
            "Can't assign to readonly property '{}'", identifier.toString());
      }
      expr.setType(value->asProperty().getType());
      break;

    case VellumValueType::Variable:
      expr.setType(value->asVariable().getType());
      break;
    default:
      errorHandler->errorAt(expr.getLocation(),
                            CompilerErrorKind::NotAssignable,
                            "'{}' is not assignable.", identifier.toString());
      return;
  }

  expr.getValue()->accept(*this);

  if (expr.getOperator() != ast::AssignOperator::Assign) {
    if (!validateComposedAssignTypes(expr.getOperator(), expr.getType(),
                                     expr.getValue()->getType(),
                                     expr.getLocation())) {
      return;
    }
  }

  std::string contextInfo = "variable," + std::string(identifier.toString());
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

  if (!expr.getLeft()->hasResolvedType() ||
      !expr.getRight()->hasResolvedType()) {
    return;
  }

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
      expr.getOperator() == ast::BinaryExpression::Operator::GreaterThanEqual) {
    auto commonType = checker.commonTernaryBranchType(
        expr.getLeft()->getType(), expr.getRight()->getType());
    if (!commonType) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::BinaryOperatorTypeMismatch,
          "Cannot perform comparison operation on types '{}' and '{}'.",
          expr.getLeft()->getType().toString(),
          expr.getRight()->getType().toString());
      return;
    }

    if (expr.getOperator() != ast::BinaryExpression::Operator::Equal &&
        expr.getOperator() != ast::BinaryExpression::Operator::NotEqual) {
      if (leftType.isNone() || rightType.isNone()) {
        errorHandler->errorAt(expr.getLocation(),
                              CompilerErrorKind::BinaryOperatorTypeMismatch,
                              "Cannot relatively compare to none.");
        return;
      }
    }

    expr.setComparisonOperandType(*commonType);
    expr.setType(VellumType::literal(VellumLiteralType::Bool));
    return;
  }

  // string concat
  if (expr.getOperator() == ast::BinaryExpression::Operator::Add &&
      leftType.isString() && rightType.isString()) {
    expr.setType(VellumType::literal(VellumLiteralType::String));
    return;
  }

  // For arithmetic operators, operands must be numeric (Int or Float). Bool
  // is not implicitly converted; use (b as Int) for numeric use. Int + Float
  // promotes to Float.
  if (expr.getOperator() == ast::BinaryExpression::Operator::Add ||
      expr.getOperator() == ast::BinaryExpression::Operator::Subtract ||
      expr.getOperator() == ast::BinaryExpression::Operator::Multiply ||
      expr.getOperator() == ast::BinaryExpression::Operator::Divide ||
      expr.getOperator() == ast::BinaryExpression::Operator::Modulo) {
    const bool leftNumeric = leftType.isInt() || leftType.isFloat();
    const bool rightNumeric = rightType.isInt() || rightType.isFloat();
    if (!leftNumeric || !rightNumeric) {
      errorHandler->errorAt(
          expr.getLocation(),
          CompilerErrorKind::ArithmeticOperationTypeMismatch,
          "Cannot perform arithmetic operation on types '{}' and '{}'.",
          leftType.toString(), rightType.toString());
      return;
    }
    if (leftType.isFloat() || rightType.isFloat()) {
      expr.setType(VellumType::literal(VellumLiteralType::Float));
    } else {
      expr.setType(leftType);
    }
    return;
  }

  errorHandler->errorAt(expr.getLocation(),
                        CompilerErrorKind::UnsupportedBinaryOperator,
                        "Unsupported binary operator");
}

void SemanticAnalyzer::visitUnaryExpression(ast::UnaryExpression& expr) {
  expr.getOperand()->accept(*this);
  if (!expr.getOperand()->hasResolvedType()) {
    return;
  }

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
  if (!expr.getExpression()->hasResolvedType()) {
    return;
  }

  if (!expr.getTargetType().isResolved()) {
    expr.setTargetType(
        resolver->resolveType(expr.getTargetType(), expr.getLocation()));
  }

  const VellumType innerType = expr.getExpression()->getType();
  const VellumType targetType = expr.getTargetType();

  if (!checker.canExplicitlyCast(innerType, targetType)) {
    if (targetType.isArray()) {
      errorHandler->errorAt(
          expr.getLocation(), CompilerErrorKind::InvalidCast,
          "Cannot cast to array type. (Skyrim: nothing can be "
          "cast to an array, including other arrays.)");
    } else {
      errorHandler->errorAt(expr.getLocation(), CompilerErrorKind::InvalidCast,
                            "Cannot cast from '{}' to '{}'.",
                            innerType.toString(), targetType.toString());
    }
    return;
  }

  expr.setType(targetType);
}

void SemanticAnalyzer::visitNewArrayExpression(ast::NewArrayExpression& expr) {
  if (const auto& subtype = expr.getSubtype()) {
    VellumType resolvedSubtype =
        resolver->resolveType(*subtype, expr.getLocation());
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

void SemanticAnalyzer::visitSelfExpression(ast::SelfExpression& expr) {
  if (inStaticContext) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::InstanceMemberInStaticContext,
                          "self is not allowed in a static function.");
    return;
  }
  expr.setType(resolver->getObject().getType());
}

void SemanticAnalyzer::visitSuperExpression(ast::SuperExpression& expr) {
  if (inStaticContext) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::InstanceMemberInStaticContext,
                          "super is not allowed in a static function.");
    return;
  }

  auto parentType = resolver->getParentType();
  if (!parentType) {
    errorHandler->errorAt(expr.getLocation(),
                          CompilerErrorKind::UnknownParentScript,
                          "super can only be used in a script that extends "
                          "another script.");
    return;
  }

  expr.setType(*parentType);
}

void SemanticAnalyzer::visitArrayIndexExpression(
    ast::ArrayIndexExpression& expr) {
  expr.getArray()->accept(*this);
  if (!expr.getArray()->hasResolvedType()) {
    return;
  }

  const VellumType arrayType = expr.getArray()->getType();

  if (!arrayType.isArray()) {
    errorHandler->errorAt(
        expr.getArray()->getLocation(), CompilerErrorKind::ArrayIndexNotArray,
        "Cannot index value of non-array type '{}'.", arrayType.toString());
    return;
  }

  expr.getIndex()->accept(*this);
  if (!expr.getIndex()->hasResolvedType()) {
    return;
  }

  const VellumType indexType = expr.getIndex()->getType();

  if (!indexType.isInt()) {
    errorHandler->errorAt(expr.getIndex()->getLocation(),
                          CompilerErrorKind::ArrayIndexNotInt,
                          "Array index must be of type 'Int'. Got type '{}'.",
                          indexType.toString());
    return;
  }

  const auto elementType = arrayType.asArraySubtype();
  assert(elementType);
  expr.setType(*elementType);
}

void SemanticAnalyzer::visitArrayIndexSetExpression(
    ast::ArrayIndexSetExpression& expr) {
  expr.getArray()->accept(*this);
  if (!expr.getArray()->hasResolvedType()) {
    return;
  }

  const VellumType arrayType = expr.getArray()->getType();

  if (!arrayType.isArray()) {
    errorHandler->errorAt(
        expr.getArray()->getLocation(), CompilerErrorKind::ArrayIndexNotArray,
        "Cannot index value of non-array type '{}'.", arrayType.toString());
    return;
  }

  expr.getIndex()->accept(*this);
  if (!expr.getIndex()->hasResolvedType()) {
    return;
  }

  const VellumType indexType = expr.getIndex()->getType();

  if (!indexType.isInt()) {
    errorHandler->errorAt(expr.getIndex()->getLocation(),
                          CompilerErrorKind::ArrayIndexNotInt,
                          "Array index must be of type 'Int'. Got type '{}'.",
                          indexType.toString());
    return;
  }

  const auto elementType = arrayType.asArraySubtype();
  assert(elementType);

  expr.getValue()->accept(*this);

  if (expr.getOperator() != ast::AssignOperator::Assign) {
    if (!validateComposedAssignTypes(expr.getOperator(), *elementType,
                                     expr.getValue()->getType(),
                                     expr.getLocation())) {
      return;
    }
  }

  auto result = checker.check(expr.getValue(), *elementType,
                              TypeChecker::Context::Assignment, "array-index");
  if (!result.compatible) {
    errorHandler->errorAt(expr.getValue()->getLocation(), result.errorKind,
                          result.errorMessage);
    return;
  }

  expr.setType(*elementType);
}

void SemanticAnalyzer::visitTernaryExpression(ast::TernaryExpression& expr) {
  expr.getCondition()->accept(*this);

  if (expr.getCondition()->hasResolvedType() &&
      !expr.getCondition()->getType().isBool()) {
    errorHandler->errorAt(
        expr.getCondition()->getLocation(),
        "Condition expression must have 'Bool' type. Given type is '{}'.",
        expr.getCondition()->getType().toString());
  }

  expr.getLeft()->accept(*this);
  expr.getRight()->accept(*this);

  if (!expr.getLeft()->hasResolvedType()) {
    return;
  }

  auto leftValid = checker.checkValidValue(expr.getLeft(),
                                           TypeChecker::Context::TernaryBranch);
  if (!leftValid.compatible) {
    errorHandler->errorAt(expr.getLeft()->getLocation(), leftValid.errorKind,
                          leftValid.errorMessage);
    return;
  }

  if (!expr.getRight()->hasResolvedType()) {
    return;
  }

  auto rightValid = checker.checkValidValue(
      expr.getRight(), TypeChecker::Context::TernaryBranch);
  if (!rightValid.compatible) {
    errorHandler->errorAt(expr.getRight()->getLocation(), rightValid.errorKind,
                          rightValid.errorMessage);
    return;
  }

  const VellumType leftType = expr.getLeft()->getType();
  const VellumType rightType = expr.getRight()->getType();
  const auto resultType = checker.commonTernaryBranchType(leftType, rightType);
  if (!resultType.has_value()) {
    errorHandler->errorAt(
        expr.getLocation(), CompilerErrorKind::TernaryTypeMismatch,
        "Ternary branch types '{}' and '{}' are incompatible.",
        leftType.toString(), rightType.toString());
    return;
  }

  auto leftCheck = checker.check(expr.getLeft(), resultType.value(),
                                 TypeChecker::Context::TernaryBranch, "true");
  if (!leftCheck.compatible) {
    errorHandler->errorAt(expr.getLeft()->getLocation(), leftCheck.errorKind,
                          leftCheck.errorMessage);
    return;
  }

  auto rightCheck = checker.check(expr.getRight(), resultType.value(),
                                  TypeChecker::Context::TernaryBranch, "false");
  if (!rightCheck.compatible) {
    errorHandler->errorAt(expr.getRight()->getLocation(), rightCheck.errorKind,
                          rightCheck.errorMessage);
    return;
  }

  expr.setType(resultType.value());
}

bool SemanticAnalyzer::validateComposedAssignTypes(ast::AssignOperator op,
                                                   const VellumType& lhsType,
                                                   const VellumType& rhsType,
                                                   const Token& location) {
  switch (op) {
    case ast::AssignOperator::Add:
      if (!lhsType.isInt() && !lhsType.isFloat() && !lhsType.isString()) {
        errorHandler->errorAt(
            location, CompilerErrorKind::UnsupportedBinaryOperator,
            "Add assignment requires Int, Float, or String type.");
        return false;
      }
      return true;
    case ast::AssignOperator::Subtract:
    case ast::AssignOperator::Multiply:
    case ast::AssignOperator::Divide:
      if (lhsType != rhsType || !(lhsType.isInt() || lhsType.isFloat())) {
        errorHandler->errorAt(
            location, CompilerErrorKind::ArithmeticOperationTypeMismatch,
            "Cannot perform arithmetic assignment on types: '{}' and '{}'",
            lhsType.toString(), rhsType.toString());
        return false;
      }
      return true;
    case ast::AssignOperator::Modulo:
      if (!lhsType.isInt() || lhsType != rhsType) {
        errorHandler->errorAt(
            location, CompilerErrorKind::ArithmeticOperationTypeMismatch,
            "Modulo assignment requires Int type.");
        return false;
      }
      return true;
    default:
      return true;
  }
}

std::string_view SemanticAnalyzer::mangleLoopVariable(std::string_view name) {
  auto str = std::format("{}_{}", name, loopCount);
  return common::StringSet::insert(str);
}
}  // namespace vellum