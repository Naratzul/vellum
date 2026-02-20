#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <fstream>
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
#include "pex/pex_debug_info.h"
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

TEST_CASE("CompileComposedAssignTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  auto assign_expr = makeUnique<ast::AssignExpression>(
      VellumIdentifier("x"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(5)),
      ast::AssignOperator::Add, Token{});
  assign_expr->setType(VellumType::literal(VellumLiteralType::Int));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

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
      std::move(arrIdent), std::move(indexExpr), std::move(valueExpr),
      ast::AssignOperator::Assign, Token{});
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

TEST_CASE("CompileBreakInWhile") {
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> loopBody;
  loopBody.push_back(makeUnique<ast::BreakStatement>(Token()));
  auto body = Vec<Unique<ast::Statement>>{};
  body.push_back(makeUnique<ast::WhileStatement>(std::move(cond),
                                                 std::move(loopBody)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

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
      "foo", Vec<ast::FunctionParameter>{}, VellumType::literal(VellumLiteralType::Int),
      Vec<Unique<ast::Statement>>{}, false));
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

  pex::PexFile file = Compiler(errorHandler).compile(
      ScriptMetadata(), semanticResult.declarations);
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
  CHECK(file.stringTable().valueByIndex(
            static_cast<std::size_t>(
                instr.getArgs()[1].asIdentifier().getValue().index())) ==
        "self");
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
                            pex::PexValue(-42));

  const pex::PexVariable& var = file.objects()[0].getVariables()[0];
  CHECK(var == expected);
}

TEST_CASE("CompileAutoProperty_WithInitializer") {
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "value", VellumType::literal(VellumLiteralType::Int), "",
      ast::FunctionBody{}, ast::FunctionBody{}, VellumLiteral(42)));

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
  REQUIRE(file.objects()[0].getProperties().size() == 1);
  const auto& vars = file.objects()[0].getVariables();
  REQUIRE(vars.size() == 1);
  REQUIRE(vars[0].defaultValue().getType() == pex::PexValueType::Integer);
  CHECK(vars[0].defaultValue().asInt() == 42);
}

TEST_CASE("DebugInfo_WhenDisabled_FileHasNoDebugInfo") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  ScriptMetadata metadata;
  metadata.emitDebugInfo = false;

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file =
      Compiler(errorHandler).compile(metadata, std::move(ast));

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
      std::move(body), false));

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
  REQUIRE(debugFunc.instructionLineMap.size() == funcs[0].getInstructions().size());

  for (size_t i = 1; i < debugFunc.instructionLineMap.size(); ++i) {
    REQUIRE(debugFunc.instructionLineMap[i] >= debugFunc.instructionLineMap[i - 1]);
  }
}

TEST_CASE("DebugInfo_EmptyFunction_HasEmptyLineMap") {
  Vec<Unique<ast::Statement>> body;

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "empty", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  ScriptMetadata metadata;
  metadata.emitDebugInfo = true;

  auto errorHandler = makeShared<CompilerErrorHandler>();
  pex::PexFile file = Compiler(errorHandler).compile(metadata, std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(file.hasDebugInfo());
  REQUIRE(file.debugInfo()->functions.size() == 1);
  REQUIRE(file.debugInfo()->functions[0].instructionLineMap.empty());
  REQUIRE(file.objects()[0].getStates()[0].getFunctions()[0].getInstructions().empty());
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
      std::move(getBody), std::move(setBody), std::nullopt));

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
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), Token{}, VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  auto errorHandler = makeShared<CompilerErrorHandler>();

  ScriptMetadata metaNoDebug;
  metaNoDebug.emitDebugInfo = false;
  pex::PexFile fileNoDebug =
      Compiler(errorHandler).compile(metaNoDebug, ast);

  ScriptMetadata metaWithDebug;
  metaWithDebug.emitDebugInfo = true;
  Vec<Unique<ast::Statement>> bodyCopy;
  bodyCopy.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(1), Token())));
  Vec<Unique<ast::Declaration>> scriptMembersCopy;
  scriptMembersCopy.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(bodyCopy), false));
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
  auto importLibrary = makeShared<ImportLibrary>(Vec<std::string>{});
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver = makeShared<Resolver>(
      VellumObject(VellumType::identifier("testscript")), errorHandler,
      importLibrary, builtinFunctions);
  auto collector = makeShared<DeclarationCollector>(errorHandler, resolver,
                                                    "testscript");
  auto analyzer = makeShared<SemanticAnalyzer>(errorHandler, resolver,
                                               "testscript");

  auto divExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Divide,
      makeUnique<ast::LiteralExpression>(VellumLiteral(9.0f)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(10.0f)));
  auto castExpr = makeUnique<ast::CastExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("b")),
      VellumType::unresolved("Int"), Token{});
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
      std::move(body), false));

  collector->collect(ast);
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