#include "pex_function_compiler.h"

#include <array>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/string_set.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "resolver.h"
#include "vellum/vellum_value.h"

namespace vellum {

namespace {
pex::PexOpCode getBinaryOpCode(ast::BinaryExpression::Operator op,
                               const VellumType& type) {
  switch (op) {
    case ast::BinaryExpression::Operator::Add:
      if (type.isInt()) return pex::PexOpCode::IAdd;
      if (type.isFloat()) return pex::PexOpCode::FAdd;
      if (type.isString()) return pex::PexOpCode::StrCat;
      break;
    case ast::BinaryExpression::Operator::Subtract:
      if (type.isInt()) return pex::PexOpCode::ISub;
      if (type.isFloat()) return pex::PexOpCode::FSub;
      break;
    case ast::BinaryExpression::Operator::Multiply:
      if (type.isInt()) return pex::PexOpCode::IMul;
      if (type.isFloat()) return pex::PexOpCode::FMul;
      break;
    case ast::BinaryExpression::Operator::Divide:
      if (type.isInt()) return pex::PexOpCode::IDiv;
      if (type.isFloat()) return pex::PexOpCode::FDiv;
      break;
    case ast::BinaryExpression::Operator::Modulo:
      if (type.isInt()) return pex::PexOpCode::IMod;
      break;
    case ast::BinaryExpression::Operator::Equal:
      return pex::PexOpCode::CmpEq;
    case ast::BinaryExpression::Operator::NotEqual:
      return pex::PexOpCode::CmpEq;  // Will be followed by Not
    case ast::BinaryExpression::Operator::LessThan:
      return pex::PexOpCode::CmpLt;
    case ast::BinaryExpression::Operator::LessThanEqual:
      return pex::PexOpCode::CmpLte;
    case ast::BinaryExpression::Operator::GreaterThan:
      return pex::PexOpCode::CmpGt;
    case ast::BinaryExpression::Operator::GreaterThanEqual:
      return pex::PexOpCode::CmpGte;
    default:
      break;
  }
  return pex::PexOpCode::Invalid;
}
}  // namespace

PexFunctionCompiler::PexFunctionCompiler(
    std::shared_ptr<CompilerErrorHandler> errorHandler,
    std::shared_ptr<Resolver> resolver, pex::PexFile& file)
    : errorHandler(errorHandler), resolver(resolver), file(file) {}

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
  statement.getExpression()->compile(*this);
}

void PexFunctionCompiler::visitReturnStatement(
    ast::ReturnStatement& statement) {
  instructions.emplace_back(
      pex::PexOpCode::Return,
      std::vector<pex::PexValue>{statement.getExpression()->compile(*this)});
}

void PexFunctionCompiler::visitIfStatement(ast::IfStatement& statement) {
  assert(statement.getCondition()->getType().isBool());
  const auto condition = pex::PexIdentifier(
      makeTempVar(
          file.getString(statement.getCondition()->getType().toString()))
          .getName());
  std::vector<pex::PexValue> assign_args = {
      condition, statement.getCondition()->compile(*this)};
  instructions.emplace_back(pex::PexOpCode::Assign, std::move(assign_args));

  pex::PexValue jmp_to_else_label(int32_t(0));
  std::vector<pex::PexValue> jmp_to_else_args = {condition, jmp_to_else_label};
  size_t jmp_to_else_pos = instructions.size();
  instructions.emplace_back(pex::PexOpCode::JmpF, std::move(jmp_to_else_args));

  for (const auto& stmt : statement.getThenBlock()) {
    stmt->accept(*this);
  }

  size_t jmp_to_end_pos = instructions.size();

  if (statement.getElseBlock().has_value()) {
    pex::PexValue jmp_to_end_label(int32_t(0));
    std::vector<pex::PexValue> endJmpArgs = {jmp_to_end_label};
    instructions.emplace_back(pex::PexOpCode::Jmp, endJmpArgs);
  }

  instructions[jmp_to_else_pos].setArg(
      1, pex::PexValue(int32_t(instructions.size() - jmp_to_else_pos)));

  if (statement.getElseBlock().has_value()) {
    for (const auto& stmt : statement.getElseBlock().value()) {
      stmt->accept(*this);
    }

    instructions[jmp_to_end_pos].setArg(
        0, pex::PexValue(int32_t(instructions.size() - jmp_to_end_pos)));
  }
}

pex::PexValue PexFunctionCompiler::compile(const ast::LiteralExpression& expr) {
  return makePexValue(expr.getLiteral(), file);
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::IdentifierExpression& expr) {
  if (auto property = resolver->resolveIdentifier(expr.getIdentifier())) {
    if (property->getType() == VellumValueType::Property) {
      ast::PropertyGetExpression getExpr(
          std::make_unique<ast::IdentifierExpression>(VellumIdentifier("self")),
          expr.getIdentifier());
      getExpr.setType(property->asProperty().getType());
      return compile(getExpr);
    }
  }

  return makePexValue(expr.getIdentifier(), file);
}

pex::PexValue PexFunctionCompiler::compile(const ast::CallExpression& expr) {
  const VellumFunctionCall functionCall = expr.getFunctionCall().value();
  const auto function = resolver->resolveFunction(functionCall.getObject(),
                                                  functionCall.getFunction());
  if (!function) {
    errorHandler->errorAt(Token(),
                          std::format("Undefined function '{}'.",
                                      functionCall.getFunction().toString()));
    return pex::PexValue();
  }

  pex::PexOpCode opcode;
  std::vector<pex::PexValue> args;

  const pex::PexValue retVal =
      function->getReturnType() == VellumType::none()
          ? getNoneVar()
          : pex::PexValue(pex::PexIdentifier(
                makeTempVar(
                    file.getString(function->getReturnType().toString()))
                    .getName()));

  if (functionCall.isStatic()) {
    opcode = pex::PexOpCode::CallStatic;
    args = {
        pex::PexIdentifier(file.getString(functionCall.getObject().getValue())),
        pex::PexIdentifier(
            file.getString(functionCall.getFunction().getValue())),
        retVal};
  } else {
    opcode = pex::PexOpCode::CallMethod;
    args = {
        pex::PexIdentifier(
            file.getString(functionCall.getFunction().getValue())),
        pex::PexIdentifier(file.getString(functionCall.getObject().getValue())),
        retVal};
  }

  std::vector<pex::PexValue> variadicArgs;
  variadicArgs.reserve(expr.getArguments().size());

  for (const auto& arg : expr.getArguments()) {
    variadicArgs.push_back(arg->compile(*this));
  }

  instructions.emplace_back(opcode, std::move(args), std::move(variadicArgs));

  return retVal;
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::PropertyGetExpression& expr) {
  const VellumPropertyAccess property = expr.produceValue().asPropertyAccess();
  const pex::PexValue retVal =
      makeTempVar(file.getString(expr.getType().toString()));

  std::vector<pex::PexValue> args = {
      pex::PexIdentifier(file.getString(property.getProperty().getValue())),
      pex::PexIdentifier(file.getString(property.getObject().getValue())),
      pex::PexIdentifier(retVal.asTempVar().getName())};

  instructions.emplace_back(pex::PexOpCode::PropGet, std::move(args));

  return pex::PexIdentifier(retVal.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::PropertySetExpression& expr) {
  const pex::PexValue object = expr.getObject()->compile(*this);
  const pex::PexValue value = expr.getValue()->compile(*this);
  std::vector<pex::PexValue> args = {makePexValue(expr.getProperty(), file),
                                     object, value};
  instructions.emplace_back(pex::PexOpCode::PropSet, std::move(args));
  return value;
}

pex::PexValue PexFunctionCompiler::compile(const ast::AssignExpression& expr) {
  const pex::PexValue value = expr.getValue()->compile(*this);
  std::vector<pex::PexValue> args = {makePexValue(expr.getName(), file), value};
  instructions.emplace_back(pex::PexOpCode::Assign, std::move(args));
  return value;
}

pex::PexValue PexFunctionCompiler::compile(const ast::BinaryExpression& expr) {
  auto dest = pex::PexIdentifier(
      makeTempVar(file.getString(expr.getType().toString())).getName());

  if (expr.getOperator() == ast::BinaryExpression::Operator::And) {
    std::vector<pex::PexValue> leftArgs = {dest,
                                           expr.getLeft()->compile(*this)};
    instructions.emplace_back(pex::PexOpCode::Assign, std::move(leftArgs));

    pex::PexValue label(int32_t(0));
    std::vector<pex::PexValue> jmpArgs = {dest, label};
    size_t offset = instructions.size();
    instructions.emplace_back(pex::PexOpCode::JmpF, std::move(jmpArgs));

    std::vector<pex::PexValue> rightArgs = {dest,
                                            expr.getRight()->compile(*this)};
    instructions.emplace_back(pex::PexOpCode::Assign, std::move(rightArgs));

    pex::PexInstruction& jmpInstr = instructions[offset];
    jmpInstr.setArg(1, pex::PexValue(int32_t(instructions.size() - offset)));

  } else if (expr.getOperator() == ast::BinaryExpression::Operator::Or) {
    std::vector<pex::PexValue> leftArgs = {dest,
                                           expr.getLeft()->compile(*this)};
    instructions.emplace_back(pex::PexOpCode::Assign, std::move(leftArgs));

    pex::PexValue label(int32_t(0));
    std::vector<pex::PexValue> jmpArgs = {dest, label};
    size_t offset = instructions.size();
    instructions.emplace_back(pex::PexOpCode::JmpT, std::move(jmpArgs));

    std::vector<pex::PexValue> rightArgs = {dest,
                                            expr.getRight()->compile(*this)};
    instructions.emplace_back(pex::PexOpCode::Assign, std::move(rightArgs));

    pex::PexInstruction& jmpInstr = instructions[offset];
    jmpInstr.setArg(1, pex::PexValue(int32_t(instructions.size() - offset)));

  } else {
    std::vector<pex::PexValue> args = {dest, expr.getLeft()->compile(*this),
                                       expr.getRight()->compile(*this)};

    pex::PexOpCode code = getBinaryOpCode(expr.getOperator(), expr.getType());
    if (code == pex::PexOpCode::Invalid) {
      errorHandler->errorAt(Token(), "Unsupported binary operator");
      return dest;
    }

    instructions.emplace_back(code, std::move(args));

    // Handle NotEqual by adding a Not operation after comparison
    if (expr.getOperator() == ast::BinaryExpression::Operator::NotEqual) {
      std::vector<pex::PexValue> notArgs = {dest, dest};
      instructions.emplace_back(pex::PexOpCode::Not, std::move(notArgs));
    }
  }

  return dest;
}

pex::PexValue PexFunctionCompiler::compile(const ast::UnaryExpression& expr) {
  auto dest = pex::PexIdentifier(
      makeTempVar(file.getString(expr.getType().toString())).getName());

  switch (expr.getOperator()) {
    case ast::UnaryExpression::Operator::Negate:
      if (expr.getType().isInt()) {
        std::vector<pex::PexValue> args = {dest,
                                           expr.getOperand()->compile(*this)};
        instructions.emplace_back(pex::PexOpCode::INeg, std::move(args));

      } else if (expr.getType().isFloat()) {
        std::vector<pex::PexValue> args = {dest,
                                           expr.getOperand()->compile(*this)};
        instructions.emplace_back(pex::PexOpCode::FNeg, std::move(args));
      } else {
        assert(false && "Invalid operand type");
      }
      break;
    case ast::UnaryExpression::Operator::Not:
      assert(expr.getType().isBool());
      std::vector<pex::PexValue> args = {dest,
                                         expr.getOperand()->compile(*this)};
      instructions.emplace_back(pex::PexOpCode::Not, std::move(args));
      break;
  }

  return dest;
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
  std::array<char, PrefixLength + std::numeric_limits<uint16_t>::digits10 + 2>
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

  const std::string_view tempVarName =
      common::StringSet::insert(std::string(std::begin(buf), std::end(buf)));

  auto tempVar =
      pex::PexTemporaryVariable(file.getString(tempVarName), typeName);
  localVariables.push_back(tempVar);

  return tempVar;
}

pex::PexIdentifier PexFunctionCompiler::getNoneVar() {
  static bool isCreated = false;
  static pex::PexIdentifier noneVar(file.getString("::nonevar"));

  if (!isCreated) {
    localVariables.emplace_back(noneVar.getValue(),
                                file.getString(VellumType::none().toString()));
    isCreated = true;
  }

  return noneVar;
}
}  // namespace vellum