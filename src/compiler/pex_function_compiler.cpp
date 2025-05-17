#include "pex_function_compiler.h"

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/string_set.h"
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

  std::optional<pex::PexString> name;
  if (auto funcName = func.getName()) {
    name = file.getString(funcName.value());
  }

  pex::PexString returnTypeName =
      file.getString(func.getReturnTypeName().toString());
  pex::PexString documentationString = file.getString("");

  std::vector<pex::PexFunctionParameter> parameters;
  parameters.reserve(func.getParameters().size());

  for (const auto& param : func.getParameters()) {
    assert(param.type.isResolved());
    parameters.emplace_back(file.getString(param.name),
                            file.getString(param.type.toString()));
  }

  return pex::PexFunction(name, returnTypeName, documentationString, 0,
                          parameters, localVariables, instructions);
}

void PexFunctionCompiler::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void PexFunctionCompiler::visitReturnStatement(
    ast::ReturnStatement& statement) {
  const VellumValue value = statement.getExpression()->produceValue();
  instructions.emplace_back(
      pex::PexOpCode::Return,
      std::vector<pex::PexValue>{makeValueFromToken(value)});
}

void PexFunctionCompiler::visitCallExpression(const ast::CallExpression& expr) {
  const VellumFunction function = expr.produceValue().asFunction();

  pex::PexOpCode opcode;
  std::vector<pex::PexValue> args;

  if (function.isStatic()) {
    opcode = pex::PexOpCode::CallStatic;
    args = {
        pex::PexIdentifier(file.getString(function.getObject().getValue())),
        pex::PexIdentifier(file.getString(function.getFunction().getValue())),
        pex::PexIdentifier(file.getString("::nonevar"))};
  } else {
    opcode = pex::PexOpCode::CallMethod;
    args = {file.getString(function.getFunction().getValue()),
            file.getString(function.getObject().getValue()), pex::PexValue()};
  }

  std::vector<pex::PexValue> variadicArgs;
  variadicArgs.reserve(expr.getArguments().size());

  for (const auto& arg : expr.getArguments()) {
    variadicArgs.push_back(makeValueFromToken(arg->produceValue()));
  }

  instructions.emplace_back(opcode, std::move(args), std::move(variadicArgs));
}

void PexFunctionCompiler::visitGetExpression(const ast::GetExpression& expr) {
  const VellumPropertyAccess property = expr.produceValue().asPropertyAccess();
  const pex::PexValue retVal =
      makeTempVar(file.getString(expr.getType().toString()));

  std::vector<pex::PexValue> args = {
      file.getString(property.getProperty().getValue()),
      file.getString(property.getObject().getValue()), retVal};

  instructions.emplace_back(pex::PexOpCode::PropGet, args);
}

pex::PexValue PexFunctionCompiler::makeValueFromToken(VellumValue value) {
  std::optional<pex::PexValue> pexValue = makePexValue(value, file);
  if (!pexValue) {
    errorHandler->errorAt(Token(), "Unexpected variable initializer type.");
    return pex::PexValue();
  }
  return pexValue.value();
}

pex::PexTemporaryVariable PexFunctionCompiler::makeTempVar(
    const pex::PexString& typeName) {
  constexpr size_t PrefixLength = 6;
  std::array<char,
             PrefixLength + std::numeric_limits<uint16_t>::max_digits10 + 1>
      buf = {
          ':', ':', 't', 'e', 'm', 'p',  // "::temp" prefix
          '\0'                           // rest inited with 0 automatically
      };

  if (tempVarCount > std::numeric_limits<uint16_t>::max()) {
    errorHandler->errorAt(
        Token(),
        "Exceeded the maximum number of temp vars possible in a function.");
  }

  auto result = std::to_chars(buf.data() + PrefixLength,
                              buf.data() + buf.size(), tempVarCount);
  if (result.ec != std::errc{}) {
    errorHandler->errorAt(
        Token(), "Failed to convert the current temp var index to a string.");
  }

  tempVarCount++;

  const std::string_view tempVarName = common::StringSet::insert(result.ptr);

  auto tempVar =
      pex::PexTemporaryVariable(file.getString(tempVarName), typeName);
  localVariables.push_back(tempVar);

  return tempVar;
}
}  // namespace vellum