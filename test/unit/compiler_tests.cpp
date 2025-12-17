#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "ast/decl/declaration.h"
#include "common/types.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "pex/pex_file.h"
#include "utils.h"

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
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(call_expr)));

  auto foo_func = makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      Vec<Unique<ast::Statement>>{});

  auto test_func = makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body));

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