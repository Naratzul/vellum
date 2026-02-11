#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <filesystem>

#include "analyze/declaration_collector.h"
#include "analyze/import_library.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "ast/decl/declaration.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "analyze/import_resolver.h"
#include "compiler/resolver.h"
#include "pex/pex_file.h"
#include "pex/pex_function.h"
#include "utils.h"
#include "vellum/vellum_object.h"

namespace fs = std::filesystem;

using namespace vellum;
using common::makeShared;
using common::makeUnique;
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
                            pex::PexValue(42));

  const pex::PexVariable& var = file.objects()[0].getVariables()[0];
  CHECK(var == expected);
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
      Vec<Unique<ast::Statement>>{}, false);

  auto test_func = makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false);

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
      std::move(body), false));

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
      std::move(arrIdent), std::move(indexExpr), std::move(valueExpr), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(arraySetExpr)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

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
      std::move(fooBody), false));
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(testBody), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<std::string>{});
  auto importResolver =
      makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver = makeShared<Resolver>(
      VellumObject(VellumType::identifier("testscript")), errorHandler,
      importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(ast);
  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  DeclarationCollector collector(errorHandler, resolver, "testscript");
  collector.collect(ast);
  REQUIRE_FALSE(errorHandler->hadError());

  SemanticAnalyzer semantic(errorHandler, resolver, "testscript");
  auto semanticResult = semantic.analyze(std::move(ast));
  REQUIRE_FALSE(errorHandler->hadError());

  pex::PexFile file =
      Compiler(errorHandler).compile(ScriptMetadata(), semanticResult.declarations);
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