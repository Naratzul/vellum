#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <filesystem>
#include <fstream>
#include <optional>

#include "analyze/import_library.h"
#include "analyze/import_resolver.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pex/pex_debug_info.h"
#include "pex/pex_file.h"
#include "pex/pex_function.h"
#include "pex/pex_instruction.h"
#include "pex/pex_value.h"
#include "utils.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_object.h"

namespace fs = std::filesystem;

using namespace vellum;
using common::makeShared;
using common::makeUnique;
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

TEST_CASE("CompileGlobalVarTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getVariables().size() == 1);

  pex::PexVariable expected(file.getString("number"), file.getString("Int"),
                            pex::PexUserFlag::None, pex::PexValue(42));

  const pex::PexVariable& var = file.objects()[0].getVariables()[0];
  CHECK(var == expected);
}

TEST_CASE("CompileComposedAssignTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("x"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(5)),
      ast::AssignOperator::Add, Token{});
  assign_expr->setType(VellumType::literal(VellumLiteralType::Int));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates()[0].getFunctions().size() == 1);

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  REQUIRE(instructions.size() >= 2);
  bool hasIAdd = false;
  bool hasAssign = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::IAdd) hasIAdd = true;
    if (instr.getOpCode() == pex::PexOpCode::Assign) hasAssign = true;
  }
  CHECK(hasIAdd);
  CHECK(hasAssign);
}

TEST_CASE("CompileFunctionCallTest") {
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  call_expr->setFunctionCall(VellumFunctionCall::staticCall(
      VellumType::identifier("TestScript"), VellumIdentifier("foo")));

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));

  auto foo_func = makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{}));

  auto test_func = makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body)));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(std::move(foo_func));
  ast.emplace_back(std::move(test_func));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  REQUIRE(file.objects()[0].getStates()[0].getFunctions().size() == 2);

  const auto& pex_test_func =
      file.objects()[0].getStates()[0].getFunctions()[1];
  REQUIRE(pex_test_func.getInstructions().size() == 1);

  const auto& instruction = pex_test_func.getInstructions()[0];
  CHECK(instruction.getOpCode() == pex::PexOpCode::CallStatic);
}

TEST_CASE("CompileStateReopen_MergesIntoSinglePexState") {
  auto makeIntFunc = [](std::string_view name, int line) {
    return makeUnique<ast::FunctionDeclaration>(
        name, Vec<ast::FunctionParameter>{},
        VellumType::literal(VellumLiteralType::Int),
        makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{}),
        noParsedModifiers, makeToken(TokenType::IDENTIFIER, line, name));
  };

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeIntFunc("rootFunc", 1));
  scriptMembers.emplace_back(makeIntFunc("funcA", 1));
  scriptMembers.emplace_back(makeIntFunc("funcB", 1));
  scriptMembers.emplace_back(makeIntFunc("funcC", 1));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  Vec<Unique<ast::Declaration>> mergedFirst;
  mergedFirst.emplace_back(makeIntFunc("funcA", 2));
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "Merged", makeToken(TokenType::IDENTIFIER, 2, "Merged"),
      noParsedModifiers, std::move(mergedFirst)));

  Vec<Unique<ast::Declaration>> mergedSecond;
  mergedSecond.emplace_back(makeIntFunc("funcB", 3));
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "Merged", makeToken(TokenType::IDENTIFIER, 3, "Merged"),
      noParsedModifiers, std::move(mergedSecond)));

  Vec<Unique<ast::Declaration>> otherMembers;
  otherMembers.emplace_back(makeIntFunc("funcC", 4));
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "Other", makeToken(TokenType::IDENTIFIER, 4, "Other"), noParsedModifiers,
      std::move(otherMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  const auto& obj = file.objects()[0];
  REQUIRE(obj.getStates().size() == 3);

  const pex::PexString rootName = file.getString("");
  const pex::PexString mergedName = file.getString("Merged");
  const pex::PexString otherName = file.getString("Other");
  CHECK(obj.getStates()[0].getName() == rootName);
  CHECK(obj.getStates()[1].getName() == mergedName);
  CHECK(obj.getStates()[2].getName() == otherName);

  REQUIRE(obj.getStates()[0].getFunctions().size() == 4);
  const auto& mergedState = obj.getStates()[1];
  REQUIRE(mergedState.getFunctions().size() == 2);
  REQUIRE(mergedState.getFunctions()[0].getName() == file.getString("funcA"));
  REQUIRE(mergedState.getFunctions()[1].getName() == file.getString("funcB"));
  REQUIRE(obj.getStates()[2].getFunctions().size() == 1);
  REQUIRE(obj.getStates()[2].getFunctions()[0].getName() ==
          file.getString("funcC"));
}

TEST_CASE("CompileStateReopen_SetsAutoStateNameFromLaterFragment") {
  auto makeIntFunc = [](std::string_view name, int line) {
    return makeUnique<ast::FunctionDeclaration>(
        name, Vec<ast::FunctionParameter>{},
        VellumType::literal(VellumLiteralType::Int),
        makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{}),
        noParsedModifiers, makeToken(TokenType::IDENTIFIER, line, name));
  };

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeIntFunc("myFunc", 1));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  Vec<Unique<ast::Declaration>> first;
  first.emplace_back(makeIntFunc("myFunc", 2));
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "S", makeToken(TokenType::IDENTIFIER, 2, "S"), noParsedModifiers,
      std::move(first)));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "S", makeToken(TokenType::IDENTIFIER, 3, "S"), autoParsedModifiers,
      Vec<Unique<ast::Declaration>>{}));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  CHECK(file.objects()[0].getAutoStateName() == file.getString("S"));
}

TEST_CASE("CompileArrayIndexGetTest") {
  // Build AST: var arr: Int[]; function test() { arr[0]; }
  Vec<Unique<ast::Declaration>> ast;

  // Global array declaration: var arr: Int[] = none
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "arr", VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      nullptr));

  // arr[0]
  auto arrIdent =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("arr"));
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  auto arrayIndexExpr = makeUnique<ast::ArrayIndexExpression>(
      std::move(arrIdent), std::move(indexExpr), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(arrayIndexExpr)));

  // function test() { arr[0]; }
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  REQUIRE(file.objects()[0].getStates()[0].getFunctions().size() == 1);

  const auto& pex_test_func =
      file.objects()[0].getStates()[0].getFunctions()[0];

  // Expect a single ArrayGetElement instruction
  const auto& instructions = pex_test_func.getInstructions();
  REQUIRE(instructions.size() == 1);
  const auto& instr = instructions[0];
  CHECK(instr.getOpCode() == pex::PexOpCode::ArrayGetElement);
  REQUIRE(instr.getArgs().size() == 3);
}

TEST_CASE("CompileArrayIndexSetTest") {
  // Build AST: var arr: Int[]; function test() { arr[0] = 42; }
  Vec<Unique<ast::Declaration>> ast;

  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "arr", VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      nullptr));

  // arr[0] = 42
  auto arrIdent =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("arr"));
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  auto valueExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(42), Token{});
  auto arraySetExpr = makeUnique<ast::ArrayIndexSetExpression>(
      std::move(arrIdent), std::move(indexExpr), std::move(valueExpr),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(arraySetExpr)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  REQUIRE(file.objects()[0].getStates()[0].getFunctions().size() == 1);

  const auto& pex_test_func =
      file.objects()[0].getStates()[0].getFunctions()[0];

  const auto& instructions = pex_test_func.getInstructions();
  REQUIRE(instructions.size() == 1);
  const auto& instr = instructions[0];
  CHECK(instr.getOpCode() == pex::PexOpCode::ArraySetElement);
  REQUIRE(instr.getArgs().size() == 3);
}

TEST_CASE("CompileBreakInWhile") {
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> loopBody;
  loopBody.push_back(makeUnique<ast::BreakStatement>(Token()));
  auto body = Vec<Unique<ast::Statement>>{};
  body.push_back(makeUnique<ast::WhileStatement>(
      std::move(cond), makeUnique<ast::BlockStatement>(std::move(loopBody))));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  bool hasJmp = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::Jmp) hasJmp = true;
  }
  CHECK(hasJmp);
}

TEST_CASE("CompileContinueInWhile") {
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> loopBody;
  loopBody.push_back(makeUnique<ast::ContinueStatement>(Token()));
  auto body = Vec<Unique<ast::Statement>>{};
  body.push_back(makeUnique<ast::WhileStatement>(
      std::move(cond), makeUnique<ast::BlockStatement>(std::move(loopBody))));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  bool hasJmp = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::Jmp) hasJmp = true;
  }
  CHECK(hasJmp);
}

TEST_CASE("CompileIf_NoElse_HasJmpF") {
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> thenStmts;
  thenStmts.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  auto body = Vec<Unique<ast::Statement>>{};
  body.push_back(makeUnique<ast::IfStatement>(
      std::move(cond), makeUnique<ast::BlockStatement>(std::move(thenStmts)),
      nullptr));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  bool hasJmpF = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::JmpF) hasJmpF = true;
  }
  CHECK(hasJmpF);
}

TEST_CASE("CompileIf_WithElse_HasJmpFAndJmp") {
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> thenStmts;
  thenStmts.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  Vec<Unique<ast::Statement>> elseStmts;
  elseStmts.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  auto body = Vec<Unique<ast::Statement>>{};
  body.push_back(makeUnique<ast::IfStatement>(
      std::move(cond), makeUnique<ast::BlockStatement>(std::move(thenStmts)),
      makeUnique<ast::BlockStatement>(std::move(elseStmts))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  bool hasJmpF = false;
  bool hasJmp = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::JmpF) hasJmpF = true;
    if (instr.getOpCode() == pex::PexOpCode::Jmp) hasJmp = true;
  }
  CHECK(hasJmpF);
  CHECK(hasJmp);
}

TEST_CASE("CompileIf_ElseIfChain_HasMultipleJmpF") {
  auto cond1 = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond1->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> then1;
  then1.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));

  auto cond2 = makeUnique<ast::LiteralExpression>(VellumLiteral(false));
  cond2->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> then2;
  then2.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  Vec<Unique<ast::Statement>> elseStmts;
  elseStmts.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));

  auto inner = makeUnique<ast::IfStatement>(
      std::move(cond2), makeUnique<ast::BlockStatement>(std::move(then2)),
      makeUnique<ast::BlockStatement>(std::move(elseStmts)));

  auto body = Vec<Unique<ast::Statement>>{};
  body.push_back(makeUnique<ast::IfStatement>(
      std::move(cond1), makeUnique<ast::BlockStatement>(std::move(then1)),
      std::move(inner)));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  int jmpfCount = 0;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::JmpF) ++jmpfCount;
  }
  CHECK(jmpfCount >= 2);
}

namespace {
Vec<Unique<ast::Statement>> makeMatchReturnBody(
    Unique<ast::Expression> scrutinee, Vec<ast::MatchArm> arms,
    Unique<ast::Statement> elseBody = nullptr) {
  Vec<Unique<ast::Statement>> body;
  body.push_back(makeUnique<ast::MatchStatement>(
      std::move(scrutinee), std::move(arms), std::move(elseBody),
      makeToken(TokenType::MATCH, 1, "match")));
  return body;
}

ast::MatchArm makeLiteralArm(int32_t pattern) {
  Vec<Unique<ast::Statement>> armBody;
  armBody.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  return makeMatchArm(
      makeUnique<ast::LiteralExpression>(VellumLiteral(pattern)),
      makeUnique<ast::BlockStatement>(std::move(armBody)));
}

ast::MatchArm makeOrLiteralArm(std::initializer_list<int32_t> patterns) {
  Vec<Unique<ast::Statement>> armBody;
  armBody.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  Vec<Unique<ast::Expression>> patternExprs;
  for (int32_t pattern : patterns) {
    patternExprs.push_back(
        makeUnique<ast::LiteralExpression>(VellumLiteral(pattern)));
  }
  return makeMatchArm(std::move(patternExprs),
                      makeUnique<ast::BlockStatement>(std::move(armBody)));
}

ast::MatchArm makeIntFloatOrArm(int32_t intPattern, float floatPattern) {
  Vec<Unique<ast::Statement>> armBody;
  armBody.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  Vec<Unique<ast::Expression>> patternExprs;
  patternExprs.push_back(
      makeUnique<ast::LiteralExpression>(VellumLiteral(intPattern)));
  patternExprs.push_back(
      makeUnique<ast::LiteralExpression>(VellumLiteral(floatPattern)));
  return makeMatchArm(std::move(patternExprs),
                      makeUnique<ast::BlockStatement>(std::move(armBody)));
}

int countOpCodesBefore(const Vec<pex::PexInstruction>& instructions,
                       pex::PexOpCode stopOp, pex::PexOpCode countOp) {
  int count = 0;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == stopOp) {
      break;
    }
    if (instr.getOpCode() == countOp) {
      ++count;
    }
  }
  return count;
}

int countOpCodes(const Vec<pex::PexInstruction>& instructions,
                 pex::PexOpCode opCode) {
  int count = 0;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == opCode) {
      ++count;
    }
  }
  return count;
}
}  // namespace

TEST_CASE("CompileMatch_SingleArm_HasCmpEqAndJmpF") {
  Vec<ast::MatchArm> arms;
  arms.push_back(makeLiteralArm(1));

  auto body = makeMatchReturnBody(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)), std::move(arms));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  CHECK(countOpCodes(instructions, pex::PexOpCode::Assign) >= 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::CmpEq) == 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::JmpF) == 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::Jmp) == 1);
}

TEST_CASE("CompileMatch_WithElse_HasCmpEqJmpFAndJmp") {
  Vec<ast::MatchArm> arms;
  arms.push_back(makeLiteralArm(2));

  Vec<Unique<ast::Statement>> elseStmts;
  elseStmts.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));

  auto body = makeMatchReturnBody(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)), std::move(arms),
      makeUnique<ast::BlockStatement>(std::move(elseStmts)));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  CHECK(countOpCodes(instructions, pex::PexOpCode::CmpEq) == 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::JmpF) == 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::Jmp) == 1);
}

TEST_CASE("CompileMatch_MultipleArms_HasMultipleCmpEq") {
  Vec<ast::MatchArm> arms;
  arms.push_back(makeLiteralArm(1));
  arms.push_back(makeLiteralArm(2));

  auto body = makeMatchReturnBody(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0)), std::move(arms));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  CHECK(countOpCodes(instructions, pex::PexOpCode::CmpEq) == 2);
  CHECK(countOpCodes(instructions, pex::PexOpCode::JmpF) == 2);
  CHECK(countOpCodes(instructions, pex::PexOpCode::Jmp) == 2);
}

TEST_CASE("CompileMatch_ScrutineeEvaluatedOnce") {
  Vec<ast::MatchArm> arms;
  arms.push_back(makeLiteralArm(1));
  arms.push_back(makeLiteralArm(2));

  auto body = makeMatchReturnBody(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0)), std::move(arms));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();

  int assignBeforeFirstCmpEq = 0;
  bool seenCmpEq = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::CmpEq) {
      seenCmpEq = true;
      break;
    }
    if (instr.getOpCode() == pex::PexOpCode::Assign) {
      ++assignBeforeFirstCmpEq;
    }
  }
  REQUIRE(seenCmpEq);
  CHECK(assignBeforeFirstCmpEq == 1);
}

TEST_CASE("CompileMatch_OrPatterns_HasMultipleCmpEqPerArm") {
  Vec<ast::MatchArm> arms;
  arms.push_back(makeOrLiteralArm({1, 2}));

  auto body = makeMatchReturnBody(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0)), std::move(arms));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  CHECK(countOpCodes(instructions, pex::PexOpCode::CmpEq) == 2);
  CHECK(countOpCodes(instructions, pex::PexOpCode::JmpF) == 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::Jmp) == 1);
}

TEST_CASE("CompileMatch_IntScrutineeFloatPattern_EmitsScrutineeCastOnce") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  Vec<ast::MatchArm> arms;
  Vec<Unique<ast::Statement>> armBody;
  armBody.push_back(makeUnique<ast::ReturnStatement>(nullptr, Token{}));
  arms.push_back(makeMatchArm(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1.0f)),
      makeUnique<ast::BlockStatement>(std::move(armBody))));

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::MatchStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(int32_t(1))),
      std::move(arms), nullptr, makeToken(TokenType::MATCH, 1, "match")));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  CHECK(countOpCodes(instructions, pex::PexOpCode::Cast) == 1);
  CHECK(countOpCodesBefore(instructions, pex::PexOpCode::CmpEq,
                           pex::PexOpCode::Cast) == 1);
}

TEST_CASE("CompileMatch_IntOrFloatPatterns_ScrutineeCastOnce") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  Vec<ast::MatchArm> arms;
  arms.push_back(makeIntFloatOrArm(1, 2.2f));

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::MatchStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(int32_t(0))),
      std::move(arms), nullptr, makeToken(TokenType::MATCH, 1, "match")));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  CHECK(countOpCodes(instructions, pex::PexOpCode::Cast) == 1);
  CHECK(countOpCodes(instructions, pex::PexOpCode::CmpEq) == 2);
}

TEST_CASE("CompileDefaultArgs_EndToEnd") {
  Vec<ast::FunctionParameter> fooParams = {
      {"x", VellumType::unresolved("Int"), VellumLiteral(5)}};
  auto fooBody = Vec<Unique<ast::Statement>>{};
  fooBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("x"))));

  auto callExpr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto testBody = Vec<Unique<ast::Statement>>{};
  testBody.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", std::move(fooParams), VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(fooBody))));
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(testBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  REQUIRE(funcs.size() >= 2);

  const pex::PexFunction* testFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() &&
        file.stringTable().valueByIndex(f.getName()->index()) == "test") {
      testFunc = &f;
      break;
    }
  }
  REQUIRE(testFunc != nullptr);
  REQUIRE(testFunc->getInstructions().size() >= 1);

  const auto& instr = testFunc->getInstructions().back();
  CHECK(instr.getOpCode() == pex::PexOpCode::CallMethod);
  const auto& variadicArgs = instr.getVariadicArgs();
  REQUIRE(variadicArgs.size() == 1);
  REQUIRE(variadicArgs[0].getType() == pex::PexValueType::Integer);
  CHECK(variadicArgs[0].asInt() == 5);
}

TEST_CASE("CompileReturn_BareReturn_EmitsReturnWithNoneVar") {
  auto fooBody = Vec<Unique<ast::Statement>>{};
  fooBody.emplace_back(makeUnique<ast::ReturnStatement>(nullptr));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(fooBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  REQUIRE(funcs.size() >= 1);

  const pex::PexFunction* fooFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() &&
        file.stringTable().valueByIndex(f.getName()->index()) == "foo") {
      fooFunc = &f;
      break;
    }
  }
  REQUIRE(fooFunc != nullptr);
  const auto& instructions = fooFunc->getInstructions();
  REQUIRE(instructions.size() >= 1);

  bool hasReturn = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::Return) {
      hasReturn = true;
      const auto& args = instr.getArgs();
      REQUIRE(args.size() == 1);
      REQUIRE(args[0].getType() == pex::PexValueType::Identifier);
      CHECK(file.stringTable().valueByIndex(static_cast<size_t>(
                args[0].asIdentifier().getValue().index())) == "::nonevar");
      break;
    }
  }
  CHECK(hasReturn);
}

TEST_CASE("CompileReturn_WithValue_EmitsReturnWithValue") {
  auto fooBody = Vec<Unique<ast::Statement>>{};
  fooBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(fooBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  REQUIRE(funcs.size() >= 1);

  const pex::PexFunction* fooFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() &&
        file.stringTable().valueByIndex(f.getName()->index()) == "foo") {
      fooFunc = &f;
      break;
    }
  }
  REQUIRE(fooFunc != nullptr);
  const auto& instructions = fooFunc->getInstructions();
  REQUIRE(instructions.size() >= 1);

  bool hasReturn = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::Return) {
      hasReturn = true;
      const auto& args = instr.getArgs();
      REQUIRE(args.size() == 1);
      break;
    }
  }
  CHECK(hasReturn);
}

TEST_CASE("CompileSelfExpression_CallMethodUsesSelf") {
  auto selfExpr = makeUnique<ast::SelfExpression>(Token());
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(selfExpr), VellumIdentifier("foo"), Token());
  auto callExpr = makeUnique<ast::CallExpression>(
      std::move(propGet), Vec<Unique<ast::Expression>>{}, Token());
  auto testBody = Vec<Unique<ast::Statement>>{};
  testBody.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{},
      VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{})));
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(testBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  REQUIRE(funcs.size() >= 2);

  const pex::PexFunction* testFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() &&
        file.stringTable().valueByIndex(f.getName()->index()) == "test") {
      testFunc = &f;
      break;
    }
  }
  REQUIRE(testFunc != nullptr);
  REQUIRE(testFunc->getInstructions().size() >= 1);

  const auto& instr = testFunc->getInstructions().back();
  CHECK(instr.getOpCode() == pex::PexOpCode::CallMethod);
  REQUIRE(instr.getArgs().size() >= 2);
  REQUIRE(instr.getArgs()[1].getType() == pex::PexValueType::Identifier);
  CHECK(file.stringTable().valueByIndex(static_cast<std::size_t>(
            instr.getArgs()[1].asIdentifier().getValue().index())) == "self");
}

TEST_CASE("CompileNegativeDefaultArgs_EndToEnd") {
  Vec<ast::FunctionParameter> fooParams = {
      {"x", VellumType::unresolved("Int"), VellumLiteral(-5)}};
  auto fooBody = Vec<Unique<ast::Statement>>{};
  fooBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("x"))));

  auto callExpr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto testBody = Vec<Unique<ast::Statement>>{};
  testBody.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", std::move(fooParams), VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(fooBody))));
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(testBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  REQUIRE(funcs.size() >= 2);

  const pex::PexFunction* testFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() &&
        file.stringTable().valueByIndex(f.getName()->index()) == "test") {
      testFunc = &f;
      break;
    }
  }
  REQUIRE(testFunc != nullptr);
  REQUIRE(testFunc->getInstructions().size() >= 1);

  const auto& instr = testFunc->getInstructions().back();
  CHECK(instr.getOpCode() == pex::PexOpCode::CallMethod);
  const auto& variadicArgs = instr.getVariadicArgs();
  REQUIRE(variadicArgs.size() == 1);
  REQUIRE(variadicArgs[0].getType() == pex::PexValueType::Integer);
  CHECK(variadicArgs[0].asInt() == -5);
}

TEST_CASE("CompileNegativeGlobalVar") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(-42))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getVariables().size() == 1);

  pex::PexVariable expected(file.getString("number"), file.getString("Int"),
                            pex::PexUserFlag::None, pex::PexValue(-42));

  const pex::PexVariable& var = file.objects()[0].getVariables()[0];
  CHECK(var == expected);
}

TEST_CASE("CompileAutoProperty_WithInitializer") {
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "value", VellumType::literal(VellumLiteralType::Int), "",
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}), VellumLiteral(42)));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getProperties().size() == 1);
  const auto& vars = file.objects()[0].getVariables();
  REQUIRE(vars.size() == 1);
  REQUIRE(vars[0].defaultValue().getType() == pex::PexValueType::Integer);
  CHECK(vars[0].defaultValue().asInt() == 42);
}

TEST_CASE("CompileFullProperty_CustomGetSet_PexAccessors") {
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "myValue_", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  ast::FunctionBody getBody;
  getBody.emplace_back(
      makeUnique<ast::ReturnStatement>(makeUnique<ast::IdentifierExpression>(
          VellumIdentifier("myValue_"), Token{})));

  ast::FunctionBody setBody;
  setBody.emplace_back(
      makeUnique<ast::ExpressionStatement>(makeUnique<ast::AssignExpression>(
          makeUnique<ast::IdentifierExpression>(VellumIdentifier("myValue_"),
                                                Token{}),
          makeUnique<ast::IdentifierExpression>(VellumIdentifier("newValue"),
                                                Token{}))));

  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "myProp", VellumType::unresolved("Int"), "",
      makeUnique<ast::BlockStatement>(std::move(getBody)),
      makeUnique<ast::BlockStatement>(std::move(setBody)), std::nullopt));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getProperties().size() == 1);
  const auto& property = file.objects()[0].getProperties()[0];

  const auto getterOpt = property.getGetAccessor();
  REQUIRE(getterOpt.has_value());
  const auto& getter = getterOpt.value();
  CHECK(getter.getReturnTypeName() == file.getString("Int"));
  CHECK(getter.getParameters().empty());

  const auto setterOpt = property.getSetAccessor();
  REQUIRE(setterOpt.has_value());
  const auto& setter = setterOpt.value();
  CHECK(setter.getReturnTypeName() == file.getString("None"));
  REQUIRE(setter.getParameters().size() == 1);
  CHECK(setter.getParameters()[0].getName() == file.getString("newValue"));
  CHECK(setter.getParameters()[0].getType() == file.getString("Int"));

  REQUIRE_FALSE(property.getBackedVariableName().has_value());
}

TEST_CASE("DebugInfo_WhenDisabled_FileHasNoDebugInfo") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  ScriptMetadata metadata;
  metadata.emitDebugInfo = false;

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file = Compiler(errorHandler).compile(metadata, std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE_FALSE(file.hasDebugInfo());
}

TEST_CASE("DebugInfo_WhenEnabled_FileHasDebugInfoAndFunctionLineMap") {
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token())));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(body))));

  ScriptMetadata metadata;
  metadata.emitDebugInfo = true;

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file = Compiler(errorHandler).compile(metadata, std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.hasDebugInfo());
  REQUIRE(file.debugInfo() != nullptr);
  REQUIRE(file.debugInfo()->functions.size() == 1);

  const auto& debugFunc = file.debugInfo()->functions[0];
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates().size() == 1);
  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  REQUIRE(funcs.size() == 1);
  REQUIRE(debugFunc.instructionLineMap.size() ==
          funcs[0].getInstructions().size());

  for (size_t i = 1; i < debugFunc.instructionLineMap.size(); ++i) {
    REQUIRE(debugFunc.instructionLineMap[i] >=
            debugFunc.instructionLineMap[i - 1]);
  }
}

TEST_CASE("DebugInfo_EmptyFunction_HasEmptyLineMap") {
  Vec<Unique<ast::Statement>> body;

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "empty", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  ScriptMetadata metadata;
  metadata.emitDebugInfo = true;

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file = Compiler(errorHandler).compile(metadata, std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.hasDebugInfo());
  REQUIRE(file.debugInfo()->functions.size() == 1);
  REQUIRE(file.debugInfo()->functions[0].instructionLineMap.empty());
  REQUIRE(file.objects()[0]
              .getStates()[0]
              .getFunctions()[0]
              .getInstructions()
              .empty());
}

TEST_CASE("DebugInfo_Property_GetterSetterHaveDebugEntries") {
  auto getBody = Vec<Unique<ast::Statement>>{};
  getBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token())));
  auto setBody = Vec<Unique<ast::Statement>>{};
  setBody.emplace_back(makeUnique<ast::ExpressionStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token())));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "value", VellumType::literal(VellumLiteralType::Int), "",
      makeUnique<ast::BlockStatement>(std::move(getBody)),
      makeUnique<ast::BlockStatement>(std::move(setBody)), std::nullopt));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  ScriptMetadata metadata;
  metadata.emitDebugInfo = true;
  pex::PexFile file =
      Compiler(errorHandler).compile(metadata, semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  REQUIRE(file.hasDebugInfo());
  REQUIRE(file.debugInfo()->functions.size() >= 2);

  bool hasGetter = false;
  bool hasSetter = false;
  for (const auto& f : file.debugInfo()->functions) {
    if (f.functionType == pex::PexDebugFunctionType::Getter) hasGetter = true;
    if (f.functionType == pex::PexDebugFunctionType::Setter) hasSetter = true;
  }
  REQUIRE(hasGetter);
  REQUIRE(hasSetter);
}

TEST_CASE("DebugInfo_WriteToFile_OutputDiffersWithAndWithoutDebug") {
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1), Token())));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(body))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();

  ScriptMetadata metaNoDebug;
  metaNoDebug.emitDebugInfo = false;
  pex::PexFile fileNoDebug = Compiler(errorHandler).compile(metaNoDebug, ast);

  ScriptMetadata metaWithDebug;
  metaWithDebug.emitDebugInfo = true;
  Vec<Unique<ast::Statement>> bodyCopy;
  bodyCopy.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1), Token())));
  Vec<Unique<ast::Declaration>> scriptMembersCopy;
  scriptMembersCopy.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(bodyCopy))));
  Vec<Unique<ast::Declaration>> astCopy;
  astCopy.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembersCopy)));
  pex::PexFile fileWithDebug =
      Compiler(errorHandler).compile(metaWithDebug, std::move(astCopy));

  REQUIRE_FALSE(fileNoDebug.hasDebugInfo());
  REQUIRE(fileWithDebug.hasDebugInfo());

  auto pathNo = fs::temp_directory_path() / "vellum_test_no_debug.pex";
  auto pathWith = fs::temp_directory_path() / "vellum_test_with_debug.pex";
  fileNoDebug.writeToFile(pathNo.string());
  fileWithDebug.writeToFile(pathWith.string());

  std::ifstream inNo(pathNo.string(), std::ios::binary);
  std::ifstream inWith(pathWith.string(), std::ios::binary);
  std::string bytesNo((std::istreambuf_iterator<char>(inNo)),
                      std::istreambuf_iterator<char>());
  std::string bytesWith((std::istreambuf_iterator<char>(inWith)),
                        std::istreambuf_iterator<char>());
  inNo.close();
  inWith.close();
  fs::remove(pathNo);
  fs::remove(pathWith);

  REQUIRE(bytesWith.size() > bytesNo.size());
  REQUIRE(bytesNo != bytesWith);
}

TEST_CASE("CompileCastAndIntFloatArithmetic") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  auto divExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Divide,
      makeUnique<ast::LiteralExpression>(VellumLiteral(9.0f)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(10.0f)));
  auto castExpr = makeUnique<ast::CastExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("b")),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("Int")), Token{});
  auto mulExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Multiply, std::move(divExpr),
      std::move(castExpr));
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("b"), VellumType::unresolved("Bool"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(true))));
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("f"), VellumType::unresolved("Float"),
      std::move(mulExpr)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  bool hasCast = false;
  for (const auto& instr : instructions) {
    if (instr.getOpCode() == pex::PexOpCode::Cast) hasCast = true;
  }
  CHECK(hasCast);
}

TEST_CASE("CompileComparison_IntFloat_EmitsCastBeforeCmpEq") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  auto cmpExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Equal,
      makeUnique<ast::LiteralExpression>(VellumLiteral(1.0f)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(int32_t(1))));
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("eq"), std::nullopt, std::move(cmpExpr)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  size_t cmpIdx = 0;
  for (; cmpIdx < instructions.size(); ++cmpIdx) {
    if (instructions[cmpIdx].getOpCode() == pex::PexOpCode::CmpEq) break;
  }
  REQUIRE(cmpIdx < instructions.size());
  bool castBeforeCmp = false;
  for (size_t j = 0; j < cmpIdx; ++j) {
    if (instructions[j].getOpCode() == pex::PexOpCode::Cast) {
      castBeforeCmp = true;
      break;
    }
  }
  CHECK(castBeforeCmp);
}

TEST_CASE("CompileComparison_ScriptSubtype_EmitsCastBeforeCmpEq") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  auto addModule = [&](const VellumObject& obj, Opt<VellumType> parent) {
    VellumIdentifier name = obj.getType().asIdentifier();
    auto module =
        makeShared<ImportModule>(name, ImportModuleType::Vellum, fs::path(""));
    auto modResolver = makeShared<Resolver>(obj, errorHandler, importLibrary,
                                            builtinFunctions);
    if (parent.has_value()) {
      modResolver->setParentType(std::move(*parent));
    }
    module->setResolver(modResolver);
    importLibrary->addTestModule(module);
  };

  addModule(VellumObject(VellumType::identifier("ParentPexCmp")), std::nullopt);
  addModule(VellumObject(VellumType::identifier("ChildPexCmp")),
            VellumType::identifier("ParentPexCmp"));

  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  Vec<ast::FunctionParameter> params;
  params.emplace_back("c", VellumType::unresolved("ChildPexCmp"));
  params.emplace_back("p", VellumType::unresolved("ParentPexCmp"));

  auto cmpExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Equal,
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("c"), Token{}),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("p"), Token{}));
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("eq"), std::nullopt, std::move(cmpExpr)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", std::move(params), VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  size_t cmpIdx = 0;
  for (; cmpIdx < instructions.size(); ++cmpIdx) {
    if (instructions[cmpIdx].getOpCode() == pex::PexOpCode::CmpEq) break;
  }
  REQUIRE(cmpIdx < instructions.size());
  bool castBeforeCmp = false;
  for (size_t j = 0; j < cmpIdx; ++j) {
    if (instructions[j].getOpCode() == pex::PexOpCode::Cast) {
      castBeforeCmp = true;
      break;
    }
  }
  CHECK(castBeforeCmp);
}

TEST_CASE("CompileComparison_IntInt_NoCastOpcode") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  auto cmpExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Equal,
      makeUnique<ast::LiteralExpression>(VellumLiteral(int32_t(2))),
      makeUnique<ast::LiteralExpression>(VellumLiteral(int32_t(3))));
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("eq"), std::nullopt, std::move(cmpExpr)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  for (const auto& instr : instructions) {
    CHECK(instr.getOpCode() != pex::PexOpCode::Cast);
  }
}

TEST_CASE("CompileForIn_OpcodePatternAndMangledLocals") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto newArr = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  Vec<Unique<ast::Statement>> forBody;
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArr), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      makeUnique<ast::BlockStatement>(std::move(forBody)), tokI, tokA));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  const pex::PexFunction* testFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() && file.stringTable().valueByIndex(static_cast<size_t>(
                           f.getName()->index())) == "test") {
      testFunc = &f;
      break;
    }
  }
  REQUIRE(testFunc != nullptr);

  const auto& instr = testFunc->getInstructions();
  auto hasOp = [&](pex::PexOpCode op) {
    for (const auto& i : instr)
      if (i.getOpCode() == op) return true;
    return false;
  };
  CHECK(hasOp(pex::PexOpCode::ArrayLength));
  CHECK(hasOp(pex::PexOpCode::Assign));
  CHECK(hasOp(pex::PexOpCode::CmpLt));
  CHECK(hasOp(pex::PexOpCode::JmpF));
  CHECK(hasOp(pex::PexOpCode::ArrayGetElement));
  CHECK(hasOp(pex::PexOpCode::IAdd));
  CHECK(hasOp(pex::PexOpCode::Jmp));

  const auto& st = file.stringTable();
  bool hasIter = false;
  bool hasIdx = false;
  for (const auto& lv : testFunc->getLocalVariables()) {
    std::string_view n =
        st.valueByIndex(static_cast<size_t>(lv.getName().index()));
    if (n == "i_1") hasIter = true;
    if (n == "i_index_1") hasIdx = true;
  }
  CHECK(hasIter);
  CHECK(hasIdx);
}

TEST_CASE("CompileForIn_CollectionCallEvaluatedOnce") {
  auto makeArrBody = Vec<Unique<ast::Statement>>{};
  makeArrBody.push_back(
      makeUnique<ast::ReturnStatement>(makeUnique<ast::NewArrayExpression>(
          VellumType::literal(VellumLiteralType::Int), VellumLiteral(1),
          Token{})));

  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto callMake =
      makeUnique<ast::CallExpression>(makeUnique<ast::IdentifierExpression>(
                                          VellumIdentifier("makeArr"), Token{}),
                                      Vec<Unique<ast::Expression>>{}, Token{});

  Vec<Unique<ast::Statement>> forBody;
  ast::FunctionBody testBody;
  testBody.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      std::move(callMake), makeUnique<ast::BlockStatement>(std::move(forBody)),
      tokI, Token{}));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.push_back(makeUnique<ast::FunctionDeclaration>(
      "makeArr", Vec<ast::FunctionParameter>{},
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      makeUnique<ast::BlockStatement>(std::move(makeArrBody))));
  scriptMembers.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(testBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& funcs = file.objects()[0].getStates()[0].getFunctions();
  const pex::PexFunction* testFunc = nullptr;
  for (const auto& f : funcs) {
    if (f.getName() && file.stringTable().valueByIndex(static_cast<size_t>(
                           f.getName()->index())) == "test") {
      testFunc = &f;
      break;
    }
  }
  REQUIRE(testFunc != nullptr);

  const auto& st = file.stringTable();
  size_t makeArrCalls = 0;
  for (const auto& i : testFunc->getInstructions()) {
    if (i.getOpCode() != pex::PexOpCode::CallMethod) continue;
    const auto& args = i.getArgs();
    REQUIRE(args.size() >= 1);
    REQUIRE(args[0].getType() == pex::PexValueType::Identifier);
    std::string_view name = st.valueByIndex(
        static_cast<size_t>(args[0].asIdentifier().getValue().index()));
    if (name == "makeArr") ++makeArrCalls;
  }
  CHECK(makeArrCalls == 1);
}

TEST_CASE("CompileTernary_IntInt_JmpPattern_NoCast") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(2)), loc);
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::ReturnStatement>(std::move(ternary)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  auto hasOp = [&](pex::PexOpCode op) {
    for (const auto& i : instructions)
      if (i.getOpCode() == op) return true;
    return false;
  };
  CHECK(hasOp(pex::PexOpCode::JmpF));
  CHECK(hasOp(pex::PexOpCode::Jmp));
  bool hasCast = false;
  for (const auto& i : instructions)
    if (i.getOpCode() == pex::PexOpCode::Cast) hasCast = true;
  CHECK_FALSE(hasCast);
}

TEST_CASE("CompileTernary_IntFloat_Promotion_HasCast") {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  auto analyzer =
      makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");

  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(2.5f)), loc);
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::ReturnStatement>(std::move(ternary)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Float"),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto result = analyzer->analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), result.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instructions =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  auto hasOp = [&](pex::PexOpCode op) {
    for (const auto& i : instructions)
      if (i.getOpCode() == op) return true;
    return false;
  };
  CHECK(hasOp(pex::PexOpCode::JmpF));
  CHECK(hasOp(pex::PexOpCode::Jmp));
  bool hasCast = false;
  for (const auto& i : instructions)
    if (i.getOpCode() == pex::PexOpCode::Cast) hasCast = true;
  CHECK(hasCast);
}

TEST_CASE("CompileNativeFunctionPexOutput") {
  Vec<ast::FunctionParameter> params{
      {"count", VellumType::literal(VellumLiteralType::Int)}};

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "getCount", params, VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{}),
      nativeParsedModifiers));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getStates()[0].getFunctions().size() == 1);

  const auto& pexFunc = file.objects()[0].getStates()[0].getFunctions()[0];
  CHECK(pexFunc.getName() == file.getString("getCount"));
  CHECK(pexFunc.getReturnTypeName() == file.getString("Int"));
  CHECK(pexFunc.isNative());
  CHECK_FALSE(pexFunc.isGlobal());
  CHECK(pexFunc.getInstructions().empty());
  REQUIRE(pexFunc.getParameters().size() == 1);
  CHECK(pexFunc.getParameters()[0].getName() == file.getString("count"));
  CHECK(pexFunc.getParameters()[0].getType() == file.getString("Int"));
}

TEST_CASE("CompileStaticNativeFunctionPexOutput") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "register", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{}),
      staticNativeParsedModifiers));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.objects()[0].getStates()[0].getFunctions().size() == 1);

  const auto& pexFunc = file.objects()[0].getStates()[0].getFunctions()[0];
  CHECK(pexFunc.getName() == file.getString("register"));
  CHECK(pexFunc.isNative());
  CHECK(pexFunc.isGlobal());
  CHECK(pexFunc.getInstructions().empty());
  CHECK(pexFunc.getLocalVariables().empty());
}

TEST_CASE("CompileNonNativeFunction_HasNoNativeFlag") {
  Vec<Unique<ast::Statement>> body;
  body.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{},
      VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& pexFunc = file.objects()[0].getStates()[0].getFunctions()[0];
  CHECK_FALSE(pexFunc.isNative());
  CHECK_FALSE(pexFunc.isGlobal());
  CHECK_FALSE(pexFunc.getInstructions().empty());
}

namespace {

pex::PexFile compileScriptWithSemantic(
    Vec<Unique<ast::Declaration>> scriptMembers,
    ParsedModifiers scriptModifiers = {}) {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers), std::move(scriptModifiers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  return Compiler(errorHandler)
      .compile(ScriptMetadata(), semanticResult.declarations);
}

Opt<uint8_t> findRegisteredFlagBitIndex(const pex::PexFile& file,
                                        std::string_view name) {
  for (const auto& flag : file.registeredUserFlags()) {
    if (file.stringTable().valueByIndex(flag.name.index()) == name) {
      return flag.bitIndex;
    }
  }
  return std::nullopt;
}

bool userFlagsContain(const pex::PexUserFlags& flags, pex::PexUserFlag flag) {
  return static_cast<bool>(flags & flag);
}

}  // namespace

TEST_CASE("CompileModifiers_HiddenScript") {
  pex::PexFile file = compileScriptWithSemantic({}, hiddenParsedModifiers);

  REQUIRE(file.objects().size() == 1);
  const auto& object = file.objects()[0];
  CHECK(userFlagsContain(object.getUserFlags(), pex::PexUserFlag::Hidden));
  CHECK_FALSE(userFlagsContain(object.getUserFlags(), pex::PexUserFlag::Conditional));
  REQUIRE(file.registeredUserFlags().size() == 1);
  CHECK(findRegisteredFlagBitIndex(file, "hidden") == 0);
}

TEST_CASE("CompileModifiers_ConditionalVar") {
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "stage", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0)), std::nullopt,
      std::nullopt, conditionalParsedModifiers));

  pex::PexFile file = compileScriptWithSemantic(std::move(scriptMembers),
                                                conditionalParsedModifiers);

  REQUIRE(file.objects().size() == 1);
  REQUIRE(file.objects()[0].getVariables().size() == 1);
  CHECK(userFlagsContain(file.objects()[0].getVariables()[0].userFlags(),
                         pex::PexUserFlag::Conditional));
  REQUIRE(file.registeredUserFlags().size() == 1);
  CHECK(findRegisteredFlagBitIndex(file, "conditional") == 1);
}

TEST_CASE("CompileModifiers_HiddenProperty") {
  ast::FunctionBody getBody;
  getBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "count", VellumType::literal(VellumLiteralType::Int), "",
      makeUnique<ast::BlockStatement>(std::move(getBody)), std::nullopt,
      std::nullopt, Token{}, Token{}, hiddenParsedModifiers));

  pex::PexFile file = compileScriptWithSemantic(std::move(scriptMembers));

  REQUIRE(file.objects()[0].getProperties().size() == 1);
  CHECK(userFlagsContain(file.objects()[0].getProperties()[0].getUserFlags(),
                         pex::PexUserFlag::Hidden));
  CHECK(file.objects()[0].getVariables().empty());
  REQUIRE(file.registeredUserFlags().size() == 1);
  CHECK(findRegisteredFlagBitIndex(file, "hidden") == 0);
}

TEST_CASE("CompileModifiers_ConditionalAutoProperty") {
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "flag", VellumType::literal(VellumLiteralType::Bool), "",
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}), std::nullopt,
      Token{}, Token{}, conditionalParsedModifiers));

  pex::PexFile file = compileScriptWithSemantic(std::move(scriptMembers),
                                                conditionalParsedModifiers);

  const auto& object = file.objects()[0];
  REQUIRE(object.getProperties().size() == 1);
  CHECK_FALSE(userFlagsContain(object.getProperties()[0].getUserFlags(),
                               pex::PexUserFlag::Conditional));
  REQUIRE(object.getVariables().size() == 1);
  CHECK(userFlagsContain(object.getVariables()[0].userFlags(),
                         pex::PexUserFlag::Conditional));
  REQUIRE(file.registeredUserFlags().size() == 1);
  CHECK(findRegisteredFlagBitIndex(file, "conditional") == 1);
}

TEST_CASE("CompileModifiers_HiddenConditionalAutoProperty") {
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "flag", VellumType::literal(VellumLiteralType::Bool), "",
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}), std::nullopt,
      Token{}, Token{}, hiddenConditionalParsedModifiers));

  pex::PexFile file = compileScriptWithSemantic(std::move(scriptMembers),
                                                conditionalParsedModifiers);

  const auto& object = file.objects()[0];
  REQUIRE(object.getProperties().size() == 1);
  CHECK(userFlagsContain(object.getProperties()[0].getUserFlags(),
                         pex::PexUserFlag::Hidden));
  CHECK_FALSE(userFlagsContain(object.getProperties()[0].getUserFlags(),
                               pex::PexUserFlag::Conditional));
  REQUIRE(object.getVariables().size() == 1);
  CHECK(userFlagsContain(object.getVariables()[0].userFlags(),
                         pex::PexUserFlag::Conditional));
  CHECK_FALSE(userFlagsContain(object.getVariables()[0].userFlags(),
                               pex::PexUserFlag::Hidden));
  REQUIRE(file.registeredUserFlags().size() == 2);
  CHECK(findRegisteredFlagBitIndex(file, "hidden") == 0);
  CHECK(findRegisteredFlagBitIndex(file, "conditional") == 1);
}

TEST_CASE("BuildImportGraph_SkipsCompilingScript_NoDuplicateDiagnostics") {
  const fs::path importDir =
      fs::temp_directory_path() / "vellum_self_import_test";
  const fs::path scriptPath = importDir / "SelfRefScript.vel";
  fs::create_directories(importDir);

  const std::string source = R"(script SelfRefScript {
  fun test() {
    SelfRefScript.foo()
  }
  native event lol() {}
}
)";

  {
    std::ofstream out(scriptPath);
    out << source;
  }

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{importDir});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  const std::string scriptName = "SelfRefScript";
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(scriptName)),
                           errorHandler, importLibrary, builtinFunctions);

  auto lexer = makeUnique<Lexer>(source);
  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();
  REQUIRE_FALSE(errorHandler->hadError());

  TypeCollector typeCollector;
  typeCollector.collect(parseResult.declarations);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes(),
                                   VellumIdentifier(scriptName));

  SemanticAnalyzer semantic(errorHandler, resolver, scriptName);
  semantic.analyze(std::move(parseResult.declarations));

  const auto invalidModifierCount = static_cast<int>(std::ranges::count_if(
      errorHandler->getErrors(), [](const DiagnosticMessage& diagnostic) {
        return diagnostic.errorKind == CompilerErrorKind::InvalidModifier;
      }));
  CHECK(invalidModifierCount == 1);

  fs::remove_all(importDir);
}

TEST_CASE("CompileArrayElements_ArrayCreateAndSet") {
  Vec<Unique<ast::Expression>> elements;
  elements.push_back(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1), Token{}));
  elements.push_back(
      makeUnique<ast::LiteralExpression>(VellumLiteral(2), Token{}));
  elements.push_back(
      makeUnique<ast::LiteralExpression>(VellumLiteral(3), Token{}));
  auto arrayLiteral =
      makeUnique<ast::NewArrayElementsExpression>(std::move(elements), Token{});

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"), std::nullopt, std::move(arrayLiteral), Token{},
      std::nullopt));

  Vec<Unique<ast::Declaration>> ast;
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(members)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instr =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  auto countOp = [&](pex::PexOpCode op) {
    int n = 0;
    for (const auto& i : instr) {
      if (i.getOpCode() == op) {
        ++n;
      }
    }
    return n;
  };
  CHECK(countOp(pex::PexOpCode::ArrayCreate) == 1);
  CHECK(countOp(pex::PexOpCode::ArraySetElement) == 3);
}

TEST_CASE("CompileEmptyArray_ReturnAndLocalInit") {
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("nums"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      makeUnique<ast::NewArrayExpression>(std::nullopt, VellumLiteral(0),
                                          Token{}),
      Token{}, makeToken(TokenType::IDENTIFIER, 1, "Int")));
  body.push_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::NewArrayExpression>(std::nullopt, VellumLiteral(0),
                                          Token{})));

  Vec<Unique<ast::Declaration>> ast;
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "makeArray", Vec<ast::FunctionParameter>{},
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      makeUnique<ast::BlockStatement>(std::move(body))));
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(members)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& instr =
      file.objects()[0].getStates()[0].getFunctions()[0].getInstructions();
  auto countOp = [&](pex::PexOpCode op) {
    int n = 0;
    for (const auto& i : instr) {
      if (i.getOpCode() == op) {
        ++n;
      }
    }
    return n;
  };
  CHECK(countOp(pex::PexOpCode::ArrayCreate) == 2);
  CHECK(countOp(pex::PexOpCode::ArraySetElement) == 0);
}

TEST_CASE("CompileEmptyArray_AssignAndCall") {
  Vec<ast::FunctionParameter> fooParams = {
      {"arr", VellumType::array(VellumType::literal(VellumLiteralType::Int))}};

  ast::FunctionBody testBody;
  testBody.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("nums"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      makeUnique<ast::NewArrayExpression>(std::nullopt, VellumLiteral(0),
                                          Token{}),
      Token{}, makeToken(TokenType::IDENTIFIER, 1, "Int")));
  testBody.push_back(makeUnique<ast::ExpressionStatement>(
      makeUnique<ast::AssignExpression>(
          makeUnique<ast::IdentifierExpression>(VellumIdentifier("nums"),
                                                Token{}),
          makeUnique<ast::NewArrayExpression>(std::nullopt, VellumLiteral(0),
                                              Token{}),
          ast::AssignOperator::Assign, Token{})));
  Vec<Unique<ast::Expression>> callArgs;
  callArgs.push_back(makeUnique<ast::NewArrayExpression>(
      std::nullopt, VellumLiteral(0), Token{}));
  testBody.push_back(makeUnique<ast::ExpressionStatement>(
      makeUnique<ast::CallExpression>(
          makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
          std::move(callArgs))));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "foo", fooParams, VellumType::none(),
      makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{})));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(testBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(members)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler)
          .compile(ScriptMetadata(), semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& testInstr =
      file.objects()[0].getStates()[0].getFunctions()[1].getInstructions();
  auto countOp = [&](pex::PexOpCode op) {
    int n = 0;
    for (const auto& i : testInstr) {
      if (i.getOpCode() == op) {
        ++n;
      }
    }
    return n;
  };
  CHECK(countOp(pex::PexOpCode::ArrayCreate) == 3);
  CHECK(countOp(pex::PexOpCode::ArraySetElement) == 0);
}

TEST_CASE("CompileLocalVarShadowing_InIfBlock_EmitsDistinctPexLocals") {
  Vec<Unique<ast::Statement>> thenBody;
  thenBody.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("num"), std::nullopt,
      makeUnique<ast::LiteralExpression>(VellumLiteral(56))));

  Vec<Unique<ast::Statement>> body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("num"), std::nullopt,
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  body.push_back(makeUnique<ast::IfStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::BlockStatement>(std::move(thenBody)), nullptr));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file = Compiler(errorHandler).compile(ScriptMetadata(),
                                                     semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& locals =
      file.objects()[0].getStates()[0].getFunctions()[0].getLocalVariables();
  const auto& st = file.stringTable();
  bool hasOuter = false;
  bool hasInner = false;
  for (const auto& lv : locals) {
    const std::string_view name =
        st.valueByIndex(static_cast<size_t>(lv.getName().index()));
    if (name == "num") {
      hasOuter = true;
    }
    if (name == "num_3") {
      hasInner = true;
    }
  }
  CHECK(hasOuter);
  CHECK(hasInner);
}

TEST_CASE("CompileLocalVarShadowing_InWhileBlock_EmitsDistinctPexLocals") {
  Vec<Unique<ast::Statement>> whileBody;
  whileBody.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("num"), std::nullopt,
      makeUnique<ast::LiteralExpression>(VellumLiteral(99))));

  Vec<Unique<ast::Statement>> body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("num"), std::nullopt,
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  body.push_back(makeUnique<ast::WhileStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(false)),
      makeUnique<ast::BlockStatement>(std::move(whileBody))));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(body))));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                           errorHandler, importLibrary, builtinFunctions);
  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file = Compiler(errorHandler).compile(ScriptMetadata(),
                                                     semanticResult.declarations);
  REQUIRE_FALSE(errorHandler->hadError());

  const auto& locals =
      file.objects()[0].getStates()[0].getFunctions()[0].getLocalVariables();
  const auto& st = file.stringTable();
  bool hasOuter = false;
  bool hasInner = false;
  for (const auto& lv : locals) {
    const std::string_view name =
        st.valueByIndex(static_cast<size_t>(lv.getName().index()));
    if (name == "num") {
      hasOuter = true;
    }
    if (name == "num_3") {
      hasInner = true;
    }
  }
  CHECK(hasOuter);
  CHECK(hasInner);
}
