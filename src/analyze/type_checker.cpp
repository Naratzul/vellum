#include "type_checker.h"

#include <algorithm>
#include <format>
#include <vector>

#include "ast/expression/expression.h"

namespace vellum {
using namespace ast;

TypeChecker::Result TypeChecker::check(const Unique<Expression>& expr,
                                       VellumType expectedType, Context context,
                                       const std::string& contextInfo) {
  Result result;

  // First, check if expression is a valid value (not function/script type)
  Result valueCheck = checkValidValueExpression(expr, context, contextInfo);
  if (!valueCheck.compatible) {
    return valueCheck;
  }

  // Then check type compatibility
  const auto& actualType = expr->getType();

  if (!actualType.isResolved()) {
    // Unresolved types are handled elsewhere, consider compatible for now
    result.compatible = true;
    return result;
  }

  if (!areTypesCompatible(actualType, expectedType)) {
    result.compatible = false;
    result.errorKind = getErrorKindForContext(context);
    result.errorMessage = getTypeMismatchErrorMessage(actualType, expectedType,
                                                      context, contextInfo);
    return result;
  }

  result.compatible = true;
  return result;
}

TypeChecker::Result TypeChecker::checkValidValue(
    const Unique<Expression>& expr, Context context,
    const std::string& contextInfo) {
  return checkValidValueExpression(expr, context, contextInfo);
}

TypeChecker::Result TypeChecker::checkValidValueExpression(
    const Unique<Expression>& expr, Context context,
    const std::string& contextInfo) {
  Result result;

  VellumValueType identifierType;
  std::string identifierName;

  if (expr->isIdentifierExpression()) {
    const auto& identifierExpr = expr->asIdentifier();
    identifierType = identifierExpr.getIdentifierType();
    identifierName = std::string(identifierExpr.getIdentifier().toString());
  } else if (expr->isPropertyGetExpression()) {
    const auto& propertyExpr = expr->asPropertyGet();
    identifierType = propertyExpr.getIdentifierType();
    // Build property path for error message (e.g., "obj.bar" or "hello.zoo")
    // Recursively build the path by traversing the chain
    common::Vec<std::string> pathParts;
    const Unique<Expression>* current = &expr;

    while (current && (*current)->isPropertyGetExpression()) {
      auto& propExpr = (*current)->asPropertyGet();
      pathParts.push_back(std::string(propExpr.getProperty().toString()));
      current = &propExpr.getObject();
    }

    if (current && (*current)->isIdentifierExpression()) {
      pathParts.push_back(
          std::string((*current)->asIdentifier().getIdentifier().toString()));
    }

    // Reverse to get the correct order (e.g., ["obj", "bar"] -> "obj.bar")
    std::reverse(pathParts.begin(), pathParts.end());

    for (size_t i = 0; i < pathParts.size(); ++i) {
      if (i > 0) identifierName += ".";
      identifierName += pathParts[i];
    }

    // Fallback if we couldn't build a path
    if (identifierName.empty()) {
      identifierName = std::string(propertyExpr.getProperty().toString());
    }
  } else {
    // Non-identifier/property-get expressions are always valid values
    result.compatible = true;
    return result;
  }

  // Functions and script types cannot be used as values
  if (identifierType == VellumValueType::Function ||
      identifierType == VellumValueType::ScriptType) {
    result.compatible = false;
    result.errorKind = getErrorKindForContext(context);
    result.errorMessage = getInvalidIdentifierErrorMessage(
        identifierType, identifierName, context, contextInfo);
    return result;
  }

  result.compatible = true;
  return result;
}

bool TypeChecker::areTypesCompatible(VellumType actual, VellumType expected) {
  // For now, simple equality check
  // This can be extended later for implicit conversions:
  // - Int -> Float (widening in assignment/argument contexts)
  // - Int <-> Float (in arithmetic operations)
  // - etc.
  return actual == expected;
}

std::string TypeChecker::getInvalidIdentifierErrorMessage(
    VellumValueType identifierType, const std::string& identifierName,
    Context context, const std::string& contextInfo) {
  const bool isFunction = (identifierType == VellumValueType::Function);
  const std::string entityName = isFunction ? "function" : "script type";
  const std::string entityValue = "'" + identifierName + "'";
  const std::string suggestion =
      isFunction ? " Did you mean to call it?" : " Expected an instance.";

  switch (context) {
    case Context::Argument:
      return std::format("Cannot pass a {} {} as an argument.{}", entityName,
                         entityValue, suggestion);

    case Context::Assignment:
      return std::format(
          "Cannot assign a {} {} to a variable or property. Expected a "
          "value.{}",
          entityName, entityValue, suggestion);

    case Context::Return:
      return std::format("Cannot return a {} {}. Expected a value.", entityName,
                         entityValue);

    case Context::BinaryOperand:
      return std::format(
          "Cannot use a {} {} as an operand in binary operations. Expected a "
          "value.",
          entityName, entityValue);

    case Context::UnaryOperand:
      return std::format(
          "Cannot use a {} {} as an operand in unary operations. Expected a "
          "value.",
          entityName, entityValue);

    case Context::Initialization:
      return std::format(
          "Cannot initialize a variable with a {} {}. Expected a value.{}",
          entityName, entityValue, suggestion);
  }
  return std::format("Invalid {} {}", entityName, entityValue);
}

std::string TypeChecker::getTypeMismatchErrorMessage(
    VellumType actual, VellumType expected, Context context,
    const std::string& contextInfo) {
  const std::string actualTypeStr(actual.toString());
  const std::string expectedTypeStr(expected.toString());

  switch (context) {
    case Context::Argument: {
      // contextInfo format: "functionName,argumentIndex" or just function name
      if (contextInfo.empty()) {
        return std::format("Expected type '{}', but got type '{}'.",
                           expectedTypeStr, actualTypeStr);
      }
      size_t commaPos = contextInfo.find(',');
      if (commaPos == std::string::npos) {
        return std::format(
            "Argument of function '{}' expects type '{}', but got type '{}'.",
            contextInfo, expectedTypeStr, actualTypeStr);
      }
      std::string funcName = contextInfo.substr(0, commaPos);
      std::string argIndex = contextInfo.substr(commaPos + 1);
      return std::format(
          "Argument {} of function '{}' expects type '{}', but got type '{}'.",
          argIndex, funcName, expectedTypeStr, actualTypeStr);
    }

    case Context::Assignment: {
      // contextInfo format: "variable,targetName" or "property,targetName" or
      // just targetName
      if (contextInfo.empty()) {
        return std::format("Can't assign a value of type '{}' (expected '{}').",
                           actualTypeStr, expectedTypeStr);
      }
      size_t commaPos = contextInfo.find(',');
      if (commaPos == std::string::npos) {
        return std::format(
            "Can't assign a value of type '{}' to '{}' of type '{}'.",
            actualTypeStr, contextInfo, expectedTypeStr);
      }
      std::string targetType =
          contextInfo.substr(0, commaPos);  // "variable" or "property"
      std::string targetName = contextInfo.substr(commaPos + 1);
      return std::format(
          "Can't assign a value of type '{}' to a {} '{}' of type '{}'.",
          actualTypeStr, targetType, targetName, expectedTypeStr);
    }

    case Context::Return: {
      std::string gotType;
      if (actual.isResolved()) {
        gotType = std::format("Got type is '{}'.", actualTypeStr);
      }
      return std::format("Return type mismatch. Expected type is '{}'. {}",
                         expectedTypeStr, gotType);
    }

    case Context::Initialization: {
      std::string gotType;
      if (actual.isResolved()) {
        gotType = std::format(", got '{}'.", actualTypeStr);
      }
      return std::format("Variable type mismatch: expected '{}'{}",
                         expectedTypeStr, gotType);
    }

    case Context::BinaryOperand: {
      return std::format(
          "Cannot perform arithmetic operation on types: '{}' and '{}'",
          actualTypeStr, expectedTypeStr);
    }

    case Context::UnaryOperand: {
      // contextInfo can contain operator name or be empty
      if (contextInfo.empty()) {
        return std::format(
            "Unary operator can be applied only to expressions with type '{}'. "
            "Got type is '{}'",
            expectedTypeStr, actualTypeStr);
      }
      return std::format(
          "Unary operator '{}' can be applied only to expressions with type "
          "'{}'. Got type is '{}'",
          contextInfo, expectedTypeStr, actualTypeStr);
    }
  }
  return std::format("Type mismatch: expected '{}', got '{}'.", expectedTypeStr,
                     actualTypeStr);
}

CompilerErrorKind TypeChecker::getErrorKindForContext(Context context) {
  switch (context) {
    case Context::Argument:
      return CompilerErrorKind::FunctionArgumentTypeMismatch;
    case Context::Assignment:
      return CompilerErrorKind::AssignTypeMismatch;
    case Context::Return:
      return CompilerErrorKind::ReturnTypeMismatch;
    case Context::Initialization:
      return CompilerErrorKind::VariableTypeMismatch;
    case Context::BinaryOperand:
      return CompilerErrorKind::ArithmeticOperationTypeMismatch;
    case Context::UnaryOperand:
      return CompilerErrorKind::UnaryOperatorTypeMismatch;
  }
  return CompilerErrorKind::InvalidArgumentType;
}

}  // namespace vellum
