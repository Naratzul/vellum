#include "pex_function_compiler.h"

#include <array>
#include <limits>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/string_set.h"
#include "common/types.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "pex/pex_identifier.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::makeUnique;
using common::Vec;

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

ast::BinaryExpression::Operator assignOpToBinaryOp(ast::AssignOperator op) {
  switch (op) {
    case ast::AssignOperator::Add:
      return ast::BinaryExpression::Operator::Add;
    case ast::AssignOperator::Subtract:
      return ast::BinaryExpression::Operator::Subtract;
    case ast::AssignOperator::Multiply:
      return ast::BinaryExpression::Operator::Multiply;
    case ast::AssignOperator::Divide:
      return ast::BinaryExpression::Operator::Divide;
    case ast::AssignOperator::Modulo:
      return ast::BinaryExpression::Operator::Modulo;
    default:
      return ast::BinaryExpression::Operator::Add;
  }
}

constexpr size_t loopSentinel = std::numeric_limits<size_t>::max();
}  // namespace

PexFunctionCompiler::PexFunctionCompiler(
    Shared<CompilerErrorHandler> errorHandler, pex::PexFile& file)
    : errorHandler(errorHandler),
      file(file),
      noneVar(file.getString("::nonevar")) {}

void PexFunctionCompiler::recordInstructionLine() {
  if (!debugInfo_) return;
  int line = currentLine_;
  if (line > static_cast<int>(std::numeric_limits<uint16_t>::max()))
    line = static_cast<int>(std::numeric_limits<uint16_t>::max());
  if (!debugInfo_->instructionLineMap.empty() &&
      line < static_cast<int>(debugInfo_->instructionLineMap.back()))
    line = static_cast<int>(debugInfo_->instructionLineMap.back());
  debugInfo_->instructionLineMap.push_back(static_cast<uint16_t>(line));
}

void PexFunctionCompiler::emitInstruction(pex::PexOpCode opcode,
                                          Vec<pex::PexValue> args) {
  instructions.emplace_back(opcode, std::move(args));
  recordInstructionLine();
}

void PexFunctionCompiler::emitInstruction(pex::PexOpCode opcode,
                                          Vec<pex::PexValue> args,
                                          Vec<pex::PexValue> variadicArgs) {
  instructions.emplace_back(opcode, std::move(args), std::move(variadicArgs));
  recordInstructionLine();
}

pex::PexFunction PexFunctionCompiler::compile(
    const ast::FunctionDeclaration& func,
    pex::PexDebugFunctionInfo* debugInfo) {
  localVariables.clear();
  instructions.clear();
  breakInstructions.clear();
  continueInstructions.clear();
  debugInfo_ = debugInfo;
  currentLine_ = 1;

  for (const auto& statement : func.getBody()) {
    statement->accept(*this);
  }

  Opt<pex::PexString> name;
  if (auto funcName = func.getName()) {
    name = file.getString(funcName.value());
  }

  pex::PexString returnTypeName =
      file.getString(func.getReturnTypeName().toString());
  pex::PexString documentationString = file.getString("");

  Vec<pex::PexFunctionParameter> parameters;
  parameters.reserve(func.getParameters().size());

  for (const auto& param : func.getParameters()) {
    assert(param.type.isResolved());
    parameters.emplace_back(file.getString(param.name),
                            file.getString(param.type.toString()));
  }

  return pex::PexFunction(name, returnTypeName, documentationString, 0,
                          parameters, localVariables, instructions,
                          func.isStatic(), false);
}

void PexFunctionCompiler::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  setCurrentLocation(statement.getExpression()->getLocation());
  statement.getExpression()->compile(*this);
}

void PexFunctionCompiler::visitReturnStatement(
    ast::ReturnStatement& statement) {
  setCurrentLocation(statement.getExpression()->getLocation());
  emitInstruction(
      pex::PexOpCode::Return,
      Vec<pex::PexValue>{statement.getExpression()->compile(*this)});
}

void PexFunctionCompiler::visitIfStatement(ast::IfStatement& statement) {
  assert(statement.getCondition()->getType().isBool());
  setCurrentLocation(statement.getCondition()->getLocation());
  const pex::PexIdentifier condition =
      makeTempVarId(statement.getCondition()->getType());
  Vec<pex::PexValue> assign_args = {condition,
                                    statement.getCondition()->compile(*this)};
  emitInstruction(pex::PexOpCode::Assign, std::move(assign_args));

  pex::PexValue jmp_to_else_label(int32_t(0));
  Vec<pex::PexValue> jmp_to_else_args = {condition, jmp_to_else_label};
  size_t jmp_to_else_pos = instructions.size();
  emitInstruction(pex::PexOpCode::JmpF, std::move(jmp_to_else_args));

  for (const auto& stmt : statement.getThenBlock()) {
    stmt->accept(*this);
  }

  size_t jmp_to_end_pos = instructions.size();

  if (statement.getElseBlock().has_value()) {
    setCurrentLocation(statement.getCondition()->getLocation());
    pex::PexValue jmp_to_end_label(int32_t(0));
    Vec<pex::PexValue> endJmpArgs = {jmp_to_end_label};
    emitInstruction(pex::PexOpCode::Jmp, endJmpArgs);
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

void PexFunctionCompiler::visitLocalVariableStatement(
    ast::LocalVariableStatement& statement) {
  localVariables.emplace_back(file.getString(statement.getName().toString()),
                              file.getString(statement.getType()->toString()));

  if (statement.getInitializer()) {
    setCurrentLocation(statement.getInitializer()->getLocation());
    pex::PexFunctionParameter localVar = localVariables.back();
    Vec<pex::PexValue> args = {pex::PexIdentifier(localVar.getName()),
                               statement.getInitializer()->compile(*this)};
    emitInstruction(pex::PexOpCode::Assign, std::move(args));
  }
}

void PexFunctionCompiler::visitWhileStatement(ast::WhileStatement& statement) {
  breakInstructions.push_back(loopSentinel);
  continueInstructions.push_back(loopSentinel);

  setCurrentLocation(statement.getCondition()->getLocation());
  const pex::PexValue condition =
      makeTempVarId(statement.getCondition()->getType());
  size_t conditionOffset = instructions.size();
  emitInstruction(
      pex::PexOpCode::Assign,
      Vec<pex::PexValue>{condition, statement.getCondition()->compile(*this)});

  size_t jmpToEndOffset = instructions.size();
  pex::PexValue jmpToEnd(int32_t(0));

  emitInstruction(pex::PexOpCode::JmpF,
                  Vec<pex::PexValue>{condition, jmpToEnd});

  for (const auto& stmt : statement.getBody()) {
    stmt->accept(*this);
  }

  setCurrentLocation(statement.getCondition()->getLocation());
  emitInstruction(pex::PexOpCode::Jmp,
                  Vec<pex::PexValue>{pex::PexValue(
                      int32_t(conditionOffset - instructions.size()))});

  auto loopBodyLength = int32_t(instructions.size() - jmpToEndOffset);
  instructions[jmpToEndOffset].setArg(1, pex::PexValue(loopBodyLength));

  while (breakInstructions.back() != loopSentinel) {
    size_t breakIndex = breakInstructions.back();
    breakInstructions.pop_back();
    instructions[breakIndex].setArg(
        0, pex::PexValue(int32_t(instructions.size() - breakIndex)));
  }

  while (continueInstructions.back() != loopSentinel) {
    size_t continueIndex = continueInstructions.back();
    continueInstructions.pop_back();
    instructions[continueIndex].setArg(
        0, pex::PexValue(int32_t(conditionOffset - continueIndex)));
  }

  continueInstructions.pop_back();
  breakInstructions.pop_back();
}

void PexFunctionCompiler::visitBreakStatement(ast::BreakStatement& statement) {
  pex::PexValue jmpToEnd(int32_t(0));
  setCurrentLocation(statement.getLocation());
  emitInstruction(pex::PexOpCode::Jmp, Vec<pex::PexValue>{jmpToEnd});
  breakInstructions.push_back(instructions.size() - 1);
}

void PexFunctionCompiler::visitContinueStatement(
    ast::ContinueStatement& statement) {
  pex::PexValue jmpToCond(int32_t(0));
  setCurrentLocation(statement.getLocation());
  emitInstruction(pex::PexOpCode::Jmp, Vec<pex::PexValue>{jmpToCond});
  continueInstructions.push_back(instructions.size() - 1);
}

void PexFunctionCompiler::visitForStatement(ast::ForStatement& statement) {
  breakInstructions.push_back(loopSentinel);
  continueInstructions.push_back(loopSentinel);

  setCurrentLocation(statement.getArrayLocation());
  const pex::PexValue arrayVal = statement.getArray()->compile(*this);
  const pex::PexIdentifier arrayLength =
      makeTempVarId(VellumType::literal(VellumLiteralType::Int));
  Vec<pex::PexValue> args = {arrayLength, arrayVal};
  emitInstruction(pex::PexOpCode::ArrayLength, std::move(args));

  localVariables.emplace_back(file.getString(statement.getCounterMangledName()),
                              file.getString("Int"));
  const pex::PexValue counter =
      pex::PexIdentifier{file.getString(statement.getCounterMangledName())};
  args = {counter, pex::PexValue(0)};
  emitInstruction(pex::PexOpCode::Assign, std::move(args));

  const pex::PexIdentifier condition =
      makeTempVarId(VellumType::literal(VellumLiteralType::Bool));
  args = {condition, counter, arrayLength};
  size_t conditionOffset = instructions.size();
  emitInstruction(pex::PexOpCode::CmpLt, std::move(args));

  size_t jmpToEndOffset = instructions.size();
  pex::PexValue jmpToEnd(int32_t(0));

  emitInstruction(pex::PexOpCode::JmpF,
                  Vec<pex::PexValue>{condition, jmpToEnd});

  setCurrentLocation(statement.getVariableNameLocation());
  localVariables.emplace_back(
      file.getString(statement.getVariableName()
                         ->asIdentifier()
                         .getMangledIdentifier()
                         ->toString()),
      file.getString(
          statement.getArray()->getType().asArraySubtype()->toString()));
  const pex::PexValue loopVariable =
      statement.getVariableName()->compile(*this);

  args = {loopVariable, arrayVal, counter};
  emitInstruction(pex::PexOpCode::ArrayGetElement, std::move(args));

  args = {counter, counter, pex::PexValue(1)};
  emitInstruction(pex::PexOpCode::IAdd, std::move(args));

  for (const auto& stmt : statement.getBody()) {
    stmt->accept(*this);
  }

  setCurrentLocation(statement.getArrayLocation());
  emitInstruction(pex::PexOpCode::Jmp,
                  Vec<pex::PexValue>{pex::PexValue(
                      int32_t(conditionOffset - instructions.size()))});

  auto loopBodyLength = int32_t(instructions.size() - jmpToEndOffset);
  instructions[jmpToEndOffset].setArg(1, pex::PexValue(loopBodyLength));

  while (breakInstructions.back() != loopSentinel) {
    size_t breakIndex = breakInstructions.back();
    breakInstructions.pop_back();
    instructions[breakIndex].setArg(
        0, pex::PexValue(int32_t(instructions.size() - breakIndex)));
  }

  while (continueInstructions.back() != loopSentinel) {
    size_t continueIndex = continueInstructions.back();
    continueInstructions.pop_back();
    instructions[continueIndex].setArg(
        0, pex::PexValue(int32_t(conditionOffset - continueIndex)));
  }

  continueInstructions.pop_back();
  breakInstructions.pop_back();
}

pex::PexValue PexFunctionCompiler::compile(const ast::LiteralExpression& expr) {
  return makePexValue(expr.getLiteral(), file);
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::IdentifierExpression& expr) {
  if (expr.getIdentifierType() == VellumValueType::Property) {
    ast::PropertyGetExpression getExpr(
        makeUnique<ast::IdentifierExpression>(VellumIdentifier("self"),
                                              expr.getLocation()),
        expr.getIdentifier(), expr.getLocation());
    getExpr.setType(expr.getType());
    return compile(getExpr);
  }

  if (auto mangledId = expr.getMangledIdentifier()) {
    return makePexValue(*mangledId, file);
  }

  return makePexValue(expr.getIdentifier(), file);
}

pex::PexValue PexFunctionCompiler::compile(const ast::CallExpression& expr) {
  const VellumFunctionCall functionCall = expr.getFunctionCall().value();
  const VellumType objectType = functionCall.getObjectType();
  const auto funcName = std::string(functionCall.getFunction().toString());

  if (objectType.isArray() && (funcName == "find" || funcName == "rfind")) {
    const auto& callee = expr.getCallee();
    assert(callee->isPropertyGetExpression());
    const auto& propGet = callee->asPropertyGet();

    const pex::PexValue arrayVal = propGet.getObject()->compile(*this);
    assert(expr.getArguments().size() >= 1);
    const pex::PexValue elementVal = expr.getArguments()[0]->compile(*this);

    pex::PexValue indexVal;
    if (expr.getArguments().size() == 2) {
      indexVal = expr.getArguments()[1]->compile(*this);
    } else {
      indexVal = pex::PexValue(int32_t(funcName == "find" ? 0 : -1));
    }

    const pex::PexValue dest = makeTempVar(file.getString("Int"));
    auto opcode = funcName == "find" ? pex::PexOpCode::ArrayFindElement
                                     : pex::PexOpCode::ArrayRFindElement;

    setCurrentLocation(expr.getLocation());
    Vec<pex::PexValue> args = {
        pex::PexIdentifier(arrayVal.asIdentifier().getValue()),
        pex::PexIdentifier(dest.asTempVar().getName()), elementVal, indexVal};
    emitInstruction(opcode, std::move(args));
    return pex::PexValue(pex::PexIdentifier(dest.asTempVar().getName()));
  }

  pex::PexOpCode opcode;
  Vec<pex::PexValue> args;

  const pex::PexValue retVal =
      expr.getType() == VellumType::none()
          ? getNoneVar()
          : pex::PexValue(makeTempVarId(expr.getType()));

  if (functionCall.isParentCall()) {
    opcode = pex::PexOpCode::CallParent;
    args = {pex::PexIdentifier(
                file.getString(functionCall.getFunction().getValue())),
            retVal};
  } else if (functionCall.isStatic()) {
    opcode = pex::PexOpCode::CallStatic;
    args = {pex::PexIdentifier(
                file.getString(functionCall.getObjectType().toString())),
            pex::PexIdentifier(
                file.getString(functionCall.getFunction().getValue())),
            retVal};
  } else {
    opcode = pex::PexOpCode::CallMethod;
    pex::PexValue objectVal;
    if (expr.getCallee()->isPropertyGetExpression()) {
      objectVal = expr.getCallee()->asPropertyGet().getObject()->compile(*this);
    } else {
      objectVal = pex::PexValue(pex::PexIdentifier(file.getString("self")));
    }
    args = {pex::PexIdentifier(
                file.getString(functionCall.getFunction().getValue())),
            objectVal, retVal};
  }

  Vec<pex::PexValue> variadicArgs;
  variadicArgs.reserve(expr.getArguments().size());
  const auto& expectedTypes = expr.getArgumentTypes();

  for (size_t i = 0; i < expr.getArguments().size(); i++) {
    const auto& arg = expr.getArguments()[i];
    pex::PexValue val = arg->compile(*this);
    if (i < expectedTypes.size() && expectedTypes[i].isFloat() &&
        arg->getType().isInt()) {
      pex::PexValue floatTemp = makeTempVarId(expectedTypes[i]);
      setCurrentLocation(arg->getLocation());
      Vec<pex::PexValue> castArgs = {floatTemp, val};
      emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
      val = floatTemp;
    }
    variadicArgs.push_back(val);
  }

  setCurrentLocation(expr.getLocation());
  emitInstruction(opcode, std::move(args), std::move(variadicArgs));

  return retVal;
}

pex::PexValue PexFunctionCompiler::compile(const ast::SelfExpression& expr) {
  (void)expr;
  return pex::PexValue(pex::PexIdentifier(file.getString("self")));
}

pex::PexValue PexFunctionCompiler::compile(const ast::SuperExpression& expr) {
  (void)expr;
  return pex::PexValue(pex::PexIdentifier(file.getString("parent")));
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::PropertyGetExpression& expr) {
  const VellumType objectType = expr.getObject()->getType();
  const auto propertyName = std::string(expr.getProperty().toString());

  if (objectType.isArray() && propertyName == "length") {
    const pex::PexValue arrayVal = expr.getObject()->compile(*this);
    const pex::PexValue dest = makeTempVar(file.getString("Int"));
    setCurrentLocation(expr.getLocation());
    Vec<pex::PexValue> args = {
        pex::PexIdentifier(dest.asTempVar().getName()),
        pex::PexIdentifier(arrayVal.asIdentifier().getValue())};
    emitInstruction(pex::PexOpCode::ArrayLength, std::move(args));
    return pex::PexIdentifier(dest.asTempVar().getName());
  }

  const pex::PexValue retVal = makeTempVar(expr.getType());

  const pex::PexValue objectVal = expr.getObject()->compile(*this);
  setCurrentLocation(expr.getLocation());
  Vec<pex::PexValue> args = {
      pex::PexIdentifier(file.getString(expr.getProperty().getValue())),
      objectVal, pex::PexIdentifier(retVal.asTempVar().getName())};

  emitInstruction(pex::PexOpCode::PropGet, std::move(args));

  return pex::PexIdentifier(retVal.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::PropertySetExpression& expr) {
  const pex::PexValue object = expr.getObject()->compile(*this);
  if (expr.getOperator() == ast::AssignOperator::Assign) {
    pex::PexValue value = expr.getValue()->compile(*this);
    if (expr.getType().isFloat() && expr.getValue()->getType().isInt()) {
      pex::PexValue floatTemp = makeTempVarId(expr.getType());
      setCurrentLocation(expr.getValue()->getLocation());
      Vec<pex::PexValue> castArgs = {floatTemp, value};
      emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
      value = floatTemp;
    }
    setCurrentLocation(expr.getLocation());
    Vec<pex::PexValue> args = {makePexValue(expr.getProperty(), file), object,
                               value};
    emitInstruction(pex::PexOpCode::PropSet, std::move(args));
    return value;
  }
  const pex::PexValue dest = makeTempVar(expr.getType());
  setCurrentLocation(expr.getLocation());
  Vec<pex::PexValue> getArgs = {makePexValue(expr.getProperty(), file), object,
                                pex::PexIdentifier(dest.asTempVar().getName())};
  emitInstruction(pex::PexOpCode::PropGet, std::move(getArgs));
  const pex::PexValue rhsVal = expr.getValue()->compile(*this);
  pex::PexOpCode code =
      getBinaryOpCode(assignOpToBinaryOp(expr.getOperator()), expr.getType());
  if (code == pex::PexOpCode::Invalid) {
    errorHandler->errorAt(expr.getLocation(), "Unsupported binary operator");
    return dest;
  }
  Vec<pex::PexValue> opArgs = {pex::PexIdentifier(dest.asTempVar().getName()),
                               pex::PexIdentifier(dest.asTempVar().getName()),
                               rhsVal};
  emitInstruction(code, std::move(opArgs));
  Vec<pex::PexValue> setArgs = {makePexValue(expr.getProperty(), file), object,
                                pex::PexIdentifier(dest.asTempVar().getName())};
  emitInstruction(pex::PexOpCode::PropSet, std::move(setArgs));
  return pex::PexIdentifier(dest.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::ArrayIndexExpression& expr) {
  const pex::PexValue retVal = makeTempVar(expr.getType());

  const pex::PexValue arrayVal = expr.getArray()->compile(*this);
  const pex::PexValue indexVal = expr.getIndex()->compile(*this);

  setCurrentLocation(expr.getLocation());
  Vec<pex::PexValue> args = {pex::PexIdentifier(retVal.asTempVar().getName()),
                             arrayVal, indexVal};

  emitInstruction(pex::PexOpCode::ArrayGetElement, std::move(args));

  return pex::PexIdentifier(retVal.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::ArrayIndexSetExpression& expr) {
  const pex::PexValue arrayVal = expr.getArray()->compile(*this);
  const pex::PexValue indexVal = expr.getIndex()->compile(*this);
  if (expr.getOperator() == ast::AssignOperator::Assign) {
    pex::PexValue value = expr.getValue()->compile(*this);
    if (expr.getType().isFloat() && expr.getValue()->getType().isInt()) {
      pex::PexValue floatTemp = makeTempVarId(expr.getType());
      setCurrentLocation(expr.getValue()->getLocation());
      Vec<pex::PexValue> castArgs = {floatTemp, value};
      emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
      value = floatTemp;
    }
    setCurrentLocation(expr.getLocation());
    Vec<pex::PexValue> args = {arrayVal, indexVal, value};
    emitInstruction(pex::PexOpCode::ArraySetElement, std::move(args));
    return value;
  }
  const pex::PexValue dest = makeTempVar(expr.getType());
  setCurrentLocation(expr.getLocation());
  Vec<pex::PexValue> getArgs = {pex::PexIdentifier(dest.asTempVar().getName()),
                                arrayVal, indexVal};
  emitInstruction(pex::PexOpCode::ArrayGetElement, std::move(getArgs));
  const pex::PexValue rhsVal = expr.getValue()->compile(*this);
  pex::PexOpCode code =
      getBinaryOpCode(assignOpToBinaryOp(expr.getOperator()), expr.getType());
  if (code == pex::PexOpCode::Invalid) {
    errorHandler->errorAt(expr.getLocation(), "Unsupported binary operator");
    return dest;
  }
  Vec<pex::PexValue> opArgs = {pex::PexIdentifier(dest.asTempVar().getName()),
                               pex::PexIdentifier(dest.asTempVar().getName()),
                               rhsVal};
  emitInstruction(code, std::move(opArgs));
  Vec<pex::PexValue> setArgs = {arrayVal, indexVal,
                                pex::PexIdentifier(dest.asTempVar().getName())};
  emitInstruction(pex::PexOpCode::ArraySetElement, std::move(setArgs));
  return pex::PexIdentifier(dest.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(const ast::AssignExpression& expr) {
  if (expr.getOperator() == ast::AssignOperator::Assign) {
    pex::PexValue value = expr.getValue()->compile(*this);
    if (expr.getType().isFloat() && expr.getValue()->getType().isInt()) {
      pex::PexValue floatTemp = makeTempVarId(expr.getType());
      setCurrentLocation(expr.getValue()->getLocation());
      Vec<pex::PexValue> castArgs = {floatTemp, value};
      emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
      value = floatTemp;
    }
    setCurrentLocation(expr.getLocation());
    Vec<pex::PexValue> args = {makePexValue(expr.getName(), file), value};
    emitInstruction(pex::PexOpCode::Assign, std::move(args));
    return value;
  }
  const pex::PexValue lhsVal = makePexValue(expr.getName(), file);
  const pex::PexValue rhsVal = expr.getValue()->compile(*this);
  const pex::PexValue dest = makeTempVar(expr.getType());
  setCurrentLocation(expr.getLocation());
  pex::PexOpCode code =
      getBinaryOpCode(assignOpToBinaryOp(expr.getOperator()), expr.getType());
  if (code == pex::PexOpCode::Invalid) {
    errorHandler->errorAt(expr.getLocation(), "Unsupported binary operator");
    return dest;
  }
  Vec<pex::PexValue> opArgs = {pex::PexIdentifier(dest.asTempVar().getName()),
                               lhsVal, rhsVal};
  emitInstruction(code, std::move(opArgs));
  Vec<pex::PexValue> assignArgs = {
      makePexValue(expr.getName(), file),
      pex::PexIdentifier(dest.asTempVar().getName())};
  emitInstruction(pex::PexOpCode::Assign, std::move(assignArgs));
  return pex::PexIdentifier(dest.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(const ast::BinaryExpression& expr) {
  pex::PexValue dest = makeTempVarId(expr.getType());

  setCurrentLocation(expr.getLocation());
  if (expr.getOperator() == ast::BinaryExpression::Operator::And) {
    Vec<pex::PexValue> leftArgs = {dest, expr.getLeft()->compile(*this)};
    emitInstruction(pex::PexOpCode::Assign, std::move(leftArgs));

    pex::PexValue label(int32_t(0));
    Vec<pex::PexValue> jmpArgs = {dest, label};
    size_t offset = instructions.size();
    emitInstruction(pex::PexOpCode::JmpF, std::move(jmpArgs));

    Vec<pex::PexValue> rightArgs = {dest, expr.getRight()->compile(*this)};
    emitInstruction(pex::PexOpCode::Assign, std::move(rightArgs));

    pex::PexInstruction& jmpInstr = instructions[offset];
    jmpInstr.setArg(1, pex::PexValue(int32_t(instructions.size() - offset)));

  } else if (expr.getOperator() == ast::BinaryExpression::Operator::Or) {
    Vec<pex::PexValue> leftArgs = {dest, expr.getLeft()->compile(*this)};
    emitInstruction(pex::PexOpCode::Assign, std::move(leftArgs));

    pex::PexValue label(int32_t(0));
    Vec<pex::PexValue> jmpArgs = {dest, label};
    size_t offset = instructions.size();
    emitInstruction(pex::PexOpCode::JmpT, std::move(jmpArgs));

    Vec<pex::PexValue> rightArgs = {dest, expr.getRight()->compile(*this)};
    emitInstruction(pex::PexOpCode::Assign, std::move(rightArgs));

    pex::PexInstruction& jmpInstr = instructions[offset];
    jmpInstr.setArg(1, pex::PexValue(int32_t(instructions.size() - offset)));

  } else {
    pex::PexValue leftVal = expr.getLeft()->compile(*this);
    pex::PexValue rightVal = expr.getRight()->compile(*this);

    if (expr.getType().isFloat()) {
      if (expr.getLeft()->getType().isInt()) {
        pex::PexValue floatTemp = makeTempVarId(expr.getType());
        setCurrentLocation(expr.getLeft()->getLocation());
        Vec<pex::PexValue> castArgs = {floatTemp, leftVal};
        emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
        leftVal = floatTemp;
      }
      if (expr.getRight()->getType().isInt()) {
        pex::PexValue floatTemp = makeTempVarId(expr.getType());
        setCurrentLocation(expr.getRight()->getLocation());
        Vec<pex::PexValue> castArgs = {floatTemp, rightVal};
        emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
        rightVal = floatTemp;
      }
    }

    pex::PexOpCode code = getBinaryOpCode(expr.getOperator(), expr.getType());
    if (code == pex::PexOpCode::Invalid) {
      errorHandler->errorAt(expr.getLocation(), "Unsupported binary operator");
      return dest;
    }

    Vec<pex::PexValue> args = {dest, leftVal, rightVal};
    emitInstruction(code, std::move(args));

    if (expr.getOperator() == ast::BinaryExpression::Operator::NotEqual) {
      Vec<pex::PexValue> notArgs = {dest, dest};
      emitInstruction(pex::PexOpCode::Not, std::move(notArgs));
    }
  }

  return dest;
}

pex::PexValue PexFunctionCompiler::compile(const ast::UnaryExpression& expr) {
  setCurrentLocation(expr.getLocation());
  pex::PexValue dest = makeTempVarId(expr.getType());

  switch (expr.getOperator()) {
    case ast::UnaryExpression::Operator::Negate:
      if (expr.getType().isInt()) {
        Vec<pex::PexValue> args = {dest, expr.getOperand()->compile(*this)};
        emitInstruction(pex::PexOpCode::INeg, std::move(args));

      } else if (expr.getType().isFloat()) {
        Vec<pex::PexValue> args = {dest, expr.getOperand()->compile(*this)};
        emitInstruction(pex::PexOpCode::FNeg, std::move(args));
      } else {
        assert(false && "Invalid operand type");
      }
      break;
    case ast::UnaryExpression::Operator::Not:
      assert(expr.getType().isBool());
      Vec<pex::PexValue> args = {dest, expr.getOperand()->compile(*this)};
      emitInstruction(pex::PexOpCode::Not, std::move(args));
      break;
  }

  return dest;
}

pex::PexValue PexFunctionCompiler::compile(const ast::CastExpression& expr) {
  assert(expr.getTargetType().isResolved());
  setCurrentLocation(expr.getLocation());

  pex::PexValue dest = makeTempVarId(expr.getTargetType());
  pex::PexValue value = expr.getExpression()->compile(*this);

  Vec<pex::PexValue> args = {dest, value};
  emitInstruction(pex::PexOpCode::Cast, std::move(args));

  return dest;
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::NewArrayExpression& expr) {
  setCurrentLocation(expr.getLocation());
  pex::PexValue dest = makeTempVarId(expr.getType());

  Vec<pex::PexValue> args = {dest, makePexValue(expr.getLength(), file)};
  emitInstruction(pex::PexOpCode::ArrayCreate, std::move(args));

  return dest;
}

pex::PexValue PexFunctionCompiler::makeValueFromToken(VellumValue value) {
  Opt<pex::PexValue> pexValue = makePexValue(value, file);
  if (!pexValue) {
    // Internal logic error - this should never happen
    errorHandler->errorAt(Token(), "Unexpected variable initializer type.");
    return pex::PexValue();
  }
  return pexValue.value();
}

pex::PexIdentifier PexFunctionCompiler::makeTempVarId(const VellumType& type) {
  return pex::PexIdentifier(makeTempVar(type).getName());
}

pex::PexTemporaryVariable PexFunctionCompiler::makeTempVar(
    const VellumType& type) {
  return makeTempVar(file.getString(type.toString()));
}

std::string_view PexFunctionCompiler::makeTempVarName() {
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
    // Internal logic error - this should never happen
    errorHandler->errorAt(
        Token(), "Failed to convert the current temp var index to a string.");
  }

  tempVarCount++;

  const std::size_t len = static_cast<std::size_t>(result.ptr - buf.data());
  const std::string_view tempVarName =
      common::StringSet::insert(std::string(buf.data(), len));

  return tempVarName;
}

pex::PexTemporaryVariable PexFunctionCompiler::makeTempVar(
    const pex::PexString& typeName) {
  auto tempVar =
      pex::PexTemporaryVariable(file.getString(makeTempVarName()), typeName);
  localVariables.push_back(tempVar);

  return tempVar;
}

pex::PexIdentifier PexFunctionCompiler::getNoneVar() {
  if (!isNoneVarAdded) {
    localVariables.emplace_back(noneVar.getValue(),
                                file.getString(VellumType::none().toString()));
    isNoneVarAdded = true;
  }

  return noneVar;
}
}  // namespace vellum