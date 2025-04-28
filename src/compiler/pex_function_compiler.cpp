#include "pex_function_compiler.h"

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "vellum/vellum_value.h"

namespace vellum {

PexFunctionCompiler::PexFunctionCompiler(
    std::shared_ptr<CompilerErrorHandler> errorHandler, pex::PexFile& file)
    : errorHandler(errorHandler), file(file) {}

pex::PexFunction PexFunctionCompiler::compile(
    const ast::FunctionDeclaration& func) {
  localVariables.clear();
  instructions.clear();

  for (const auto& statement : func.getBody()) {
    statement->accept(*this);
  }

  pex::PexString name = file.getString(func.getName());
  pex::PexString returnTypeName =
      file.getString(func.getReturnTypeName().has_value()
                         ? func.getReturnTypeName().value()
                         : valueTypeToString(VellumValueType::None));
  pex::PexString documentationString = file.getString("");

  std::vector<pex::PexFunctionParameter> parameters;
  parameters.reserve(func.getParameters().size());

  for (const auto& param : func.getParameters()) {
    parameters.emplace_back(file.getString(param.name),
                            file.getString(param.type));
  }

  return pex::PexFunction(name, returnTypeName, documentationString, 0,
                          parameters, localVariables, instructions);
}

void PexFunctionCompiler::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void PexFunctionCompiler::visitCallExpression(ast::CallExpression& expr) {
  pex::PexOpCode opcode;
  std::vector<pex::PexValue> args;

  if (expr.getModuleName().has_value()) {
    opcode = pex::PexOpCode::CallStatic;
    args = {pex::PexIdentifier(file.getString(expr.getModuleName().value())),
            pex::PexIdentifier(file.getString(expr.getFunctionName())),
            pex::PexIdentifier(file.getString("::nonevar"))};
  } else {
    opcode = pex::PexOpCode::CallMethod;
    args = {file.getString(expr.getFunctionName()), file.getString("self"),
            pex::PexValue()};
  }

  std::vector<pex::PexValue> variadicArgs;
  variadicArgs.reserve(expr.getArguments().size());

  for (const auto& arg : expr.getArguments()) {
    variadicArgs.push_back(makeValueFromToken(arg->produceValue()));
  }

  instructions.emplace_back(opcode, std::move(args), std::move(variadicArgs));
}

pex::PexValue PexFunctionCompiler::makeValueFromToken(VellumValue value) {
  std::optional<pex::PexValue> pexValue = makePexValue(value, file);
  if (!pexValue) {
    errorHandler->errorAt(Token(), "Unexpected variable initializer type.");
    return pex::PexValue();
  }
  return pexValue.value();
}
}  // namespace vellum