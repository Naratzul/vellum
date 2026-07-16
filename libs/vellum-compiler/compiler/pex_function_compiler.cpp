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
#include "vellum/vellum_literal.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::makeUnique;
using common::Opt;
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
  if (!debugInfo) return;
  int line = currentLine;
  if (line > static_cast<int>(std::numeric_limits<uint16_t>::max()))
    line = static_cast<int>(std::numeric_limits<uint16_t>::max());
  if (!debugInfo->instructionLineMap.empty() &&
      line < static_cast<int>(debugInfo->instructionLineMap.back()))
    line = static_cast<int>(debugInfo->instructionLineMap.back());
  debugInfo->instructionLineMap.push_back(static_cast<uint16_t>(line));
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

pex::PexValue PexFunctionCompiler::emitArrayCreate(const Token& location,
                                                   const VellumType& type,
                                                   VellumLiteral length) {
  setCurrentLocation(location);
  pex::PexValue dest = makeTempVarId(type);

  Vec<pex::PexValue> args = {dest, makePexValue(length, file)};
  emitInstruction(pex::PexOpCode::ArrayCreate, std::move(args));

  return dest;
}

void PexFunctionCompiler::emitArrayIndexSet(const Token& location,
                                            const pex::PexValue& array,
                                            const pex::PexValue& index,
                                            const pex::PexValue& value) {
  setCurrentLocation(location);
  Vec<pex::PexValue> args = {array, index, value};
  emitInstruction(pex::PexOpCode::ArraySetElement, std::move(args));
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::NewArrayExpression& expr) {
  return emitArrayCreate(expr.getLocation(), expr.getType(), expr.getLength());
}

void PexFunctionCompiler::emitLogicalOr(const pex::PexValue& dest,
                                        const pex::PexValue& left,
                                        const pex::PexValue& right) {
  Vec<pex::PexValue> leftArgs = {dest, left};
  emitInstruction(pex::PexOpCode::Assign, std::move(leftArgs));

  pex::PexValue label(int32_t(0));
  Vec<pex::PexValue> jmpArgs = {dest, label};
  size_t offset = instructions.size();
  emitInstruction(pex::PexOpCode::JmpT, std::move(jmpArgs));

  Vec<pex::PexValue> rightArgs = {dest, right};
  emitInstruction(pex::PexOpCode::Assign, std::move(rightArgs));

  pex::PexInstruction& jmpInstr = instructions[offset];
  jmpInstr.setArg(1, pex::PexValue(int32_t(instructions.size() - offset)));
}

void PexFunctionCompiler::emitMatchPatternCmpEq(
    const pex::PexValue& dest, const pex::PexValue& scrutineeVal,
    const VellumType& scrutineeType, const ast::Expression& pattern,
    Opt<VellumType> promoteType, const Opt<pex::PexValue>& scrutineeFloatVal) {
  pex::PexValue lhs = scrutineeVal;
  pex::PexValue rhs = pattern.compile(*this);
  const VellumType& patternType = pattern.getType();

  if (promoteType.has_value() && promoteType->isFloat()) {
    if (scrutineeType.isInt() && patternType.isFloat() && scrutineeFloatVal) {
      lhs = *scrutineeFloatVal;
    } else if (scrutineeType.isFloat() && patternType.isInt()) {
      pex::PexValue floatTemp = makeTempVarId(*promoteType);
      setCurrentLocation(pattern.getLocation());
      Vec<pex::PexValue> castArgs = {floatTemp, rhs};
      emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
      rhs = floatTemp;
    }
  }

  setCurrentLocation(pattern.getLocation());
  Vec<pex::PexValue> cmpArgs = {dest, lhs, rhs};
  emitInstruction(pex::PexOpCode::CmpEq, std::move(cmpArgs));
}

pex::PexFunction PexFunctionCompiler::compile(
    const ast::FunctionDeclaration& func,
    pex::PexDebugFunctionInfo* debugInfo_) {
  localVariables.clear();
  instructions.clear();
  breakInstructions.clear();
  continueInstructions.clear();
  debugInfo = debugInfo_;
  currentLine = 1;
  currentReturnType = func.getReturnTypeName();

  if (func.isNative()) {
    assert(func.getBody()->IsEmpty());
  }

  func.getBody()->accept(*this);

  currentReturnType = std::nullopt;

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
                          func.isStatic(), func.isNative());
}

pex::PexValue PexFunctionCompiler::coerceToType(pex::PexValue value,
                                                const VellumType& srcType,
                                                const VellumType& destType,
                                                const Token& location) {
  if (srcType == destType) {
    return value;
  }

  const bool promoteIntToFloat = destType.isFloat() && srcType.isInt();
  const bool promoteScriptSubtype =
      destType.getState() == VellumTypeState::Identifier &&
      srcType.getState() == VellumTypeState::Identifier;
  if (!promoteIntToFloat && !promoteScriptSubtype) {
    return value;
  }

  pex::PexValue promotedTemp = makeTempVarId(destType);
  setCurrentLocation(location);
  Vec<pex::PexValue> castArgs = {promotedTemp, value};
  emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
  return promotedTemp;
}

void PexFunctionCompiler::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  setCurrentLocation(statement.getExpression()->getLocation());
  statement.getExpression()->compile(*this);
}

void PexFunctionCompiler::visitReturnStatement(
    ast::ReturnStatement& statement) {
  setCurrentLocation(statement.getLocation());
  pex::PexValue returnValue;
  if (statement.getExpression()) {
    returnValue = statement.getExpression()->compile(*this);
    if (currentReturnType.has_value()) {
      returnValue =
          coerceToType(std::move(returnValue),
                       statement.getExpression()->getType(), *currentReturnType,
                       statement.getExpression()->getLocation());
    }
  } else {
    returnValue = pex::PexValue(getNoneVar());
  }
  emitInstruction(pex::PexOpCode::Return,
                  Vec<pex::PexValue>{std::move(returnValue)});
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

  statement.getThenBlock()->accept(*this);

  size_t jmp_to_end_pos = instructions.size();

  if (statement.getElseBlock()) {
    setCurrentLocation(statement.getCondition()->getLocation());
    pex::PexValue jmp_to_end_label(int32_t(0));
    Vec<pex::PexValue> endJmpArgs = {jmp_to_end_label};
    emitInstruction(pex::PexOpCode::Jmp, endJmpArgs);
  }

  instructions[jmp_to_else_pos].setArg(
      1, pex::PexValue(int32_t(instructions.size() - jmp_to_else_pos)));

  if (auto& elseBlock = statement.getElseBlock()) {
    elseBlock->accept(*this);
    instructions[jmp_to_end_pos].setArg(
        0, pex::PexValue(int32_t(instructions.size() - jmp_to_end_pos)));
  }
}

void PexFunctionCompiler::visitLocalVariableStatement(
    ast::LocalVariableStatement& statement) {
  const auto pexName =
      statement.getMangledPexName().value_or(statement.getName());
  localVariables.emplace_back(file.getString(pexName.toString()),
                              file.getString(statement.getType()->toString()));

  if (statement.getInitializer()) {
    setCurrentLocation(statement.getInitializer()->getLocation());
    pex::PexFunctionParameter localVar = localVariables.back();
    pex::PexValue initVal = statement.getInitializer()->compile(*this);
    if (statement.getType().has_value()) {
      initVal = coerceToType(std::move(initVal),
                             statement.getInitializer()->getType(),
                             *statement.getType(),
                             statement.getInitializer()->getLocation());
    }
    Vec<pex::PexValue> args = {pex::PexIdentifier(localVar.getName()),
                               std::move(initVal)};
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

  statement.getBody()->accept(*this);

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

  const bool isFormList =
      statement.getCollectionKind() ==
      ast::ForStatement::CollectionKind::FormList;

  setCurrentLocation(statement.getArrayLocation());
  const pex::PexValue collectionVal = statement.getArray()->compile(*this);
  const pex::PexIdentifier collectionLength =
      makeTempVarId(VellumType::literal(VellumLiteralType::Int));

  if (isFormList) {
    Vec<pex::PexValue> lengthArgs = {
        pex::PexIdentifier(file.getString("GetSize")), collectionVal,
        collectionLength};
    emitInstruction(pex::PexOpCode::CallMethod, std::move(lengthArgs));
  } else {
    Vec<pex::PexValue> args = {collectionLength, collectionVal};
    emitInstruction(pex::PexOpCode::ArrayLength, std::move(args));
  }

  localVariables.emplace_back(file.getString(statement.getCounterMangledName()),
                              file.getString("Int"));
  const pex::PexValue counter =
      pex::PexIdentifier{file.getString(statement.getCounterMangledName())};
  Vec<pex::PexValue> args = {counter, pex::PexValue(0)};
  emitInstruction(pex::PexOpCode::Assign, std::move(args));

  const pex::PexIdentifier condition =
      makeTempVarId(VellumType::literal(VellumLiteralType::Bool));
  args = {condition, counter, collectionLength};
  size_t conditionOffset = instructions.size();
  emitInstruction(pex::PexOpCode::CmpLt, std::move(args));

  size_t jmpToEndOffset = instructions.size();
  pex::PexValue jmpToEnd(int32_t(0));

  emitInstruction(pex::PexOpCode::JmpF,
                  Vec<pex::PexValue>{condition, jmpToEnd});

  setCurrentLocation(statement.getVariableNameLocation());
  const std::string elementTypeName =
      isFormList
          ? std::string("Form")
          : std::string(
                statement.getArray()->getType().asArraySubtype()->toString());
  localVariables.emplace_back(
      file.getString(statement.getVariableName()
                         ->asIdentifier()
                         .getMangledIdentifier()
                         ->toString()),
      file.getString(elementTypeName));
  const pex::PexValue loopVariable =
      statement.getVariableName()->compile(*this);

  if (isFormList) {
    Vec<pex::PexValue> getAtArgs = {
        pex::PexIdentifier(file.getString("GetAt")), collectionVal,
        loopVariable};
    emitInstruction(pex::PexOpCode::CallMethod, std::move(getAtArgs),
                    Vec<pex::PexValue>{counter});
  } else {
    args = {loopVariable, collectionVal, counter};
    emitInstruction(pex::PexOpCode::ArrayGetElement, std::move(args));
  }

  if (statement.hasIndex()) {
    setCurrentLocation(statement.getIndexNameLocation());
    localVariables.emplace_back(
        file.getString(statement.getIndexName()
                           ->asIdentifier()
                           .getMangledIdentifier()
                           ->toString()),
        file.getString("Int"));
    const pex::PexValue indexVariable =
        statement.getIndexName()->compile(*this);
    args = {indexVariable, counter};
    emitInstruction(pex::PexOpCode::Assign, std::move(args));
  }

  args = {counter, counter, pex::PexValue(1)};
  emitInstruction(pex::PexOpCode::IAdd, std::move(args));

  statement.getBody()->accept(*this);

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

void PexFunctionCompiler::visitBlockStatement(ast::BlockStatement& statement) {
  for (auto& stmt : statement.getStatements()) {
    stmt->accept(*this);
  }
}

void PexFunctionCompiler::visitMatchStatement(ast::MatchStatement& statement) {
  setCurrentLocation(statement.getScrutinee()->getLocation());

  const VellumType scrutineeType = statement.getScrutinee()->getType();
  const Opt<VellumType> promoteType = statement.getPromoteType();

  const pex::PexIdentifier scrutinee =
      makeTempVarId(scrutineeType);
  Vec<pex::PexValue> args = {scrutinee,
                             statement.getScrutinee()->compile(*this)};
  emitInstruction(pex::PexOpCode::Assign, std::move(args));

  Opt<pex::PexValue> scrutineeFloatVal;
  if (promoteType.has_value() && promoteType->isFloat() && scrutineeType.isInt()) {
    scrutineeFloatVal = makeTempVarId(*promoteType);
    setCurrentLocation(statement.getScrutinee()->getLocation());
    Vec<pex::PexValue> castArgs = {*scrutineeFloatVal, scrutinee};
    emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
  }

  Vec<std::size_t> jumpToEndInstructions;
  jumpToEndInstructions.reserve(statement.getArms().size());

  for (const auto& arm : statement.getArms()) {
    assert(!arm.patterns.empty());

    setCurrentLocation(arm.patterns[0]->getLocation());

    pex::PexValue check =
        makeTempVarId(VellumType::literal(VellumLiteralType::Bool));

    bool firstPattern = true;
    for (const auto& pattern : arm.patterns) {
      if (firstPattern) {
        emitMatchPatternCmpEq(check, scrutinee, scrutineeType, *pattern,
                              promoteType, scrutineeFloatVal);
        firstPattern = false;
      } else {
        pex::PexValue patternResult =
            makeTempVarId(VellumType::literal(VellumLiteralType::Bool));

        emitMatchPatternCmpEq(patternResult, scrutinee, scrutineeType,
                              *pattern, promoteType, scrutineeFloatVal);
        emitLogicalOr(check, check, patternResult);
      }
    }

    args = {check, pex::PexValue(int32_t(0))};
    std::size_t jmpIndex = instructions.size();
    emitInstruction(pex::PexOpCode::JmpF, std::move(args));

    arm.body->accept(*this);

    args = {pex::PexValue(int32_t(0))};
    std::size_t jmpToEndIndex = instructions.size();
    jumpToEndInstructions.push_back(jmpToEndIndex);
    emitInstruction(pex::PexOpCode::Jmp, std::move(args));

    instructions[jmpIndex].setArg(
        1, pex::PexValue(int32_t(instructions.size() - jmpIndex)));
  }

  if (const auto& elseBody = statement.getElseBody()) {
    elseBody->accept(*this);
  }

  for (auto jumpToEndInstruction : jumpToEndInstructions) {
    pex::PexValue offset(int32_t(instructions.size() - jumpToEndInstruction));
    instructions[jumpToEndInstruction].setArg(0, offset);
  }
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
    if (i < expectedTypes.size()) {
      val = coerceToType(std::move(val), arg->getType(), expectedTypes[i],
                         arg->getLocation());
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

pex::PexValue PexFunctionCompiler::compile(const ast::TernaryExpression& expr) {
  assert(expr.getCondition()->getType().isBool());

  const VellumType resultType = expr.getType();
  pex::PexIdentifier result = makeTempVarId(resultType);

  setCurrentLocation(expr.getCondition()->getLocation());
  const pex::PexIdentifier condition =
      makeTempVarId(expr.getCondition()->getType());
  Vec<pex::PexValue> args = {condition, expr.getCondition()->compile(*this)};
  emitInstruction(pex::PexOpCode::Assign, std::move(args));

  pex::PexValue jmp_to_else_label(int32_t(0));
  args = {condition, jmp_to_else_label};
  size_t jmp_to_else_pos = instructions.size();
  emitInstruction(pex::PexOpCode::JmpF, std::move(args));

  setCurrentLocation(expr.getLeft()->getLocation());
  pex::PexValue leftResult = expr.getLeft()->compile(*this);
  if (resultType.isFloat() && expr.getLeft()->getType().isInt()) {
    pex::PexValue floatTemp = makeTempVarId(resultType);
    Vec<pex::PexValue> castArgs = {floatTemp, leftResult};
    emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
    leftResult = floatTemp;
  }
  args = {result, leftResult};
  emitInstruction(pex::PexOpCode::Assign, std::move(args));

  size_t jmp_to_end_pos = instructions.size();

  setCurrentLocation(expr.getCondition()->getLocation());
  pex::PexValue jmp_to_end_label(int32_t(0));
  args = {jmp_to_end_label};
  emitInstruction(pex::PexOpCode::Jmp, args);

  instructions[jmp_to_else_pos].setArg(
      1, pex::PexValue(int32_t(instructions.size() - jmp_to_else_pos)));

  setCurrentLocation(expr.getRight()->getLocation());
  pex::PexValue rightResult = expr.getRight()->compile(*this);
  if (resultType.isFloat() && expr.getRight()->getType().isInt()) {
    pex::PexValue floatTemp = makeTempVarId(resultType);
    Vec<pex::PexValue> castArgs = {floatTemp, rightResult};
    emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
    rightResult = floatTemp;
  }
  args = {result, rightResult};
  emitInstruction(pex::PexOpCode::Assign, std::move(args));

  instructions[jmp_to_end_pos].setArg(
      0, pex::PexValue(int32_t(instructions.size() - jmp_to_end_pos)));

  return result;
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::InterpolatedStringExpression& expr) {
  setCurrentLocation(expr.getLocation());
  const VellumType stringType = VellumType::literal(VellumLiteralType::String);

  if (expr.getParts().empty()) {
    return makePexValue(VellumLiteral(std::string_view("")), file);
  }

  Opt<pex::PexValue> result;
  for (const auto& part : expr.getParts()) {
    setCurrentLocation(part->getLocation());
    pex::PexValue partValue = part->compile(*this);
    if (!part->getType().isString()) {
      const pex::PexValue casted = makeTempVarId(stringType);
      Vec<pex::PexValue> castArgs = {casted, partValue};
      emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
      partValue = casted;
    }
    if (!result) {
      result = partValue;
    } else {
      const pex::PexValue dest = makeTempVarId(stringType);
      Vec<pex::PexValue> catArgs = {dest, *result, partValue};
      emitInstruction(pex::PexOpCode::StrCat, std::move(catArgs));
      result = dest;
    }
  }
  return *result;
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
    pex::PexValue value = coerceToType(
        expr.getValue()->compile(*this), expr.getValue()->getType(),
        expr.getType(), expr.getValue()->getLocation());
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
    pex::PexValue value = coerceToType(
        expr.getValue()->compile(*this), expr.getValue()->getType(),
        expr.getType(), expr.getValue()->getLocation());
    emitArrayIndexSet(expr.getLocation(), arrayVal, indexVal, value);
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
  emitArrayIndexSet(expr.getLocation(), arrayVal, indexVal,
                    pex::PexIdentifier(dest.asTempVar().getName()));

  return pex::PexIdentifier(dest.asTempVar().getName());
}

pex::PexValue PexFunctionCompiler::compile(const ast::AssignExpression& expr) {
  pex::PexValue name = expr.getName()->compile(*this);
  if (expr.getOperator() == ast::AssignOperator::Assign) {
    pex::PexValue value = coerceToType(
        expr.getValue()->compile(*this), expr.getValue()->getType(),
        expr.getType(), expr.getValue()->getLocation());
    setCurrentLocation(expr.getLocation());
    Vec<pex::PexValue> args = {name, value};
    emitInstruction(pex::PexOpCode::Assign, std::move(args));
    return value;
  }
  const pex::PexValue lhsVal = name;
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
      name, pex::PexIdentifier(dest.asTempVar().getName())};
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
    emitLogicalOr(dest, expr.getLeft()->compile(*this),
                  expr.getRight()->compile(*this));
  } else {
    pex::PexValue leftVal = expr.getLeft()->compile(*this);
    pex::PexValue rightVal = expr.getRight()->compile(*this);

    Opt<VellumType> promoteType;
    if (expr.getComparisonOperandType().has_value()) {
      promoteType = expr.getComparisonOperandType();
    } else if (expr.getType().isFloat()) {
      promoteType = expr.getType();
    }
    if (promoteType.has_value() && promoteType->isFloat()) {
      if (expr.getLeft()->getType().isInt()) {
        pex::PexValue floatTemp = makeTempVarId(*promoteType);
        setCurrentLocation(expr.getLeft()->getLocation());
        Vec<pex::PexValue> castArgs = {floatTemp, leftVal};
        emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
        leftVal = floatTemp;
      }
      if (expr.getRight()->getType().isInt()) {
        pex::PexValue floatTemp = makeTempVarId(*promoteType);
        setCurrentLocation(expr.getRight()->getLocation());
        Vec<pex::PexValue> castArgs = {floatTemp, rightVal};
        emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
        rightVal = floatTemp;
      }
    } else if (promoteType.has_value() &&
               promoteType->getState() == VellumTypeState::Identifier) {
      if (expr.getLeft()->getType().getState() == VellumTypeState::Identifier &&
          expr.getLeft()->getType() != *promoteType) {
        pex::PexValue objTemp = makeTempVarId(*promoteType);
        setCurrentLocation(expr.getLeft()->getLocation());
        Vec<pex::PexValue> castArgs = {objTemp, leftVal};
        emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
        leftVal = objTemp;
      }
      if (expr.getRight()->getType().getState() ==
              VellumTypeState::Identifier &&
          expr.getRight()->getType() != *promoteType) {
        pex::PexValue objTemp = makeTempVarId(*promoteType);
        setCurrentLocation(expr.getRight()->getLocation());
        Vec<pex::PexValue> castArgs = {objTemp, rightVal};
        emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));
        rightVal = objTemp;
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

pex::PexValue PexFunctionCompiler::compile(const ast::IsExpression& expr) {
  assert(expr.getTargetType().isResolved());
  setCurrentLocation(expr.getLocation());

  pex::PexValue value = expr.getExpression()->compile(*this);
  const VellumType boolType = VellumType::literal(VellumLiteralType::Bool);

  if (file.header().gameID == game::GameID::Skyrim) {
    // Desugar: (expr as Target) as Bool
    pex::PexValue castTemp = makeTempVarId(expr.getTargetType());
    Vec<pex::PexValue> castArgs = {castTemp, value};
    emitInstruction(pex::PexOpCode::Cast, std::move(castArgs));

    pex::PexValue boolTemp = makeTempVarId(boolType);
    Vec<pex::PexValue> boolArgs = {boolTemp, castTemp};
    emitInstruction(pex::PexOpCode::Cast, std::move(boolArgs));
    return boolTemp;
  }

  // Fallout 4+: dedicated Is opcode (dest, src, type)
  pex::PexValue dest = makeTempVarId(boolType);
  Vec<pex::PexValue> args = {
      dest, value,
      pex::PexIdentifier(file.getString(expr.getTargetType().toString()))};
  emitInstruction(pex::PexOpCode::Is, std::move(args));
  return dest;
}

pex::PexValue PexFunctionCompiler::compile(
    const ast::NewArrayElementsExpression& expr) {
  pex::PexValue array =
      emitArrayCreate(expr.getLocation(), expr.getType(),
                      VellumLiteral(int32_t(expr.getElementsCount())));

  for (int i = 0; i < expr.getElementsCount(); ++i) {
    const auto& element = expr.getElements()[i];
    emitArrayIndexSet(expr.getLocation(), array, pex::PexValue(int32_t(i)),
                      element->compile(*this));
  }

  return array;
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