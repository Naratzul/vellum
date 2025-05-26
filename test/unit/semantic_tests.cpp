#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <memory>
#include <vector>

#include "analyze/semantic_analyzer.h"
#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "utils.h"

using namespace vellum;

TEST_CASE("SemanticGlobalVarTest") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  ast.emplace_back(std::make_unique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42))));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver =
      std::make_shared<Resolver>(VellumObject(VellumIdentifier("TestScript")));
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::GlobalVariableDeclaration expected(
      "number", VellumType::literal(VellumLiteralType::Int),
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42)));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("SemanticAutoPropertyTest") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  ast.emplace_back(std::make_unique<ast::PropertyDeclaration>(
      "MyProperty", VellumType::unresolved("String"), "", std::nullopt,
      std::nullopt, VellumValue()));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver =
      std::make_shared<Resolver>(VellumObject(VellumIdentifier("TestScript")));
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::PropertyDeclaration expected(
      "MyProperty", VellumType::literal(VellumLiteralType::String), "",
      std::nullopt, std::nullopt, VellumValue());
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("SemanticFunctionTest") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", std::vector<ast::FunctionParameter>{},
      VellumType::unresolved("Bool"),
      std::vector<std::unique_ptr<ast::Statement>>{}));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver =
      std::make_shared<Resolver>(VellumObject(VellumIdentifier("TestScript")));
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::FunctionDeclaration expected(
      "foo", {}, VellumType::literal(VellumLiteralType::Bool), {});
  CHECK(expected == *result.declarations[0]);
}
