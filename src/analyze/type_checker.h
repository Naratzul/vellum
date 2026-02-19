#pragma once

#include <optional>
#include <string>

#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace ast {
class Expression;
}

using common::Unique;

class TypeChecker {
 public:
  enum class Context {
    Argument,       // function call argument
    Assignment,     // var x = value, property = value
    Return,         // return statement
    BinaryOperand,  // left/right operand in binary operations
    UnaryOperand,   // operand in unary operations
    Initialization  // variable initialization
  };

  struct Result {
    bool compatible;
    CompilerErrorKind errorKind;
    std::string errorMessage;  // Empty if compatible

    Result()
        : compatible(true), errorKind(CompilerErrorKind::InvalidArgumentType) {}
  };

  // Check if expression is compatible with expected type in given context
  // Context-specific info is optional and used for better error messages:
  // - For Argument: pass function name and argument index (e.g., "foo,0")
  // - For Assignment: pass target type and name (e.g., "variable,myVar" or
  // "property,myProp")
  // - For Return: no additional info needed (function context available from
  // resolver)
  // - For BinaryOperand/UnaryOperand/Initialization: pass operand/target name
  // if available
  Result check(const Unique<ast::Expression>& expr, VellumType expectedType,
               Context context, const std::string& contextInfo = "");

  // Check if expression is a valid value (not function/script type identifier)
  // Used for binary/unary operands where we need to check validity before type
  // checking
  Result checkValidValue(const Unique<ast::Expression>& expr, Context context,
                         const std::string& contextInfo = "");

  // Explicit cast compatibility check (expr AS Type).
  // Skyrim-first: casting *to* an Array type is not allowed.
  bool canExplicitlyCast(VellumType src, VellumType dest) const;

 private:
  // Check if expression is a valid value (not function/script type identifier)
  Result checkValidValueExpression(const Unique<ast::Expression>& expr,
                                   Context context,
                                   const std::string& contextInfo);

  // Check if types are compatible (can be extended for implicit conversions)
  bool areTypesCompatible(VellumType actual, VellumType expected);

  std::string getInvalidIdentifierErrorMessage(
      VellumValueType identifierType, const std::string& identifierName,
      Context context, const std::string& contextInfo);

  std::string getTypeMismatchErrorMessage(VellumType actual,
                                          VellumType expected, Context context,
                                          const std::string& contextInfo);

  CompilerErrorKind getErrorKindForContext(Context context);
};

}  // namespace vellum
