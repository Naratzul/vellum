#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <memory>
#include <vector>

#include "ast/decl/declaration.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "pex/pex_file.h"
#include "utils.h"

using namespace vellum;

TEST_CASE("CompileGlobalVarTest") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  ast.emplace_back(std::make_unique<ast::GlobalVariableDeclaration>(
      "number", VellumType::literal(VellumLiteralType::Int),
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42))));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
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
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::vector<std::unique_ptr<ast::Expression>>{});
  call_expr->setFunctionCall(VellumFunctionCall::staticCall(
      VellumType::identifier("TestScript"), VellumIdentifier("foo")));

  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));

  auto foo_func = std::make_unique<ast::FunctionDeclaration>(
      "foo", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::vector<std::unique_ptr<ast::Statement>>{});

  auto test_func = std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body));

  std::vector<std::unique_ptr<ast::Declaration>> ast;
  ast.emplace_back(std::move(foo_func));
  ast.emplace_back(std::move(test_func));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
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