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
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
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
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
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
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::FunctionDeclaration expected(
      "foo", {}, VellumType::literal(VellumLiteralType::Bool), {});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("SemanticCall_NoArgs") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // Define foo(): Int
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", std::vector<ast::FunctionParameter>{},
      VellumType::unresolved("Int"),
      std::vector<std::unique_ptr<ast::Statement>>{}));

  // test() { foo(); }
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::vector<std::unique_ptr<ast::Expression>>{});
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
  const auto& test_func =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[1]);
  REQUIRE(test_func.getBody().size() == 1);
  const auto& expr_stmt =
      dynamic_cast<ast::ExpressionStatement&>(*test_func.getBody()[0]);
  const auto& call =
      dynamic_cast<ast::CallExpression&>(*expr_stmt.getExpression());
  CHECK(call.getType().isInt());
}

TEST_CASE("SemanticCall_WithArgs") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  std::vector<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      std::vector<std::unique_ptr<ast::Statement>>{}));

  // test() { foo(42, "hi"); }
  auto args = std::vector<std::unique_ptr<ast::Expression>>{};
  args.emplace_back(
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42)));
  args.emplace_back(std::make_unique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("hi"))));
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
  const auto& test_func =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[1]);
  REQUIRE(test_func.getBody().size() == 1);
  const auto& expr_stmt =
      dynamic_cast<ast::ExpressionStatement&>(*test_func.getBody()[0]);
  const auto& call =
      dynamic_cast<ast::CallExpression&>(*expr_stmt.getExpression());
  CHECK(call.getType().isBool());
}

TEST_CASE("SemanticCall_UndefinedFunction") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // test() { notDefined(); }
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(
          VellumIdentifier("notDefined")),
      std::vector<std::unique_ptr<ast::Expression>>{});
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("SemanticCall_ArgumentTypeMismatch") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  std::vector<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      std::vector<std::unique_ptr<ast::Statement>>{}));

  // test() { foo("not an int", 123); }
  auto args = std::vector<std::unique_ptr<ast::Expression>>{};
  args.emplace_back(std::make_unique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("not an int"))));
  args.emplace_back(
      std::make_unique<ast::LiteralExpression>(VellumLiteral(123)));
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("SemanticCall_TooFewArguments") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  std::vector<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      std::vector<std::unique_ptr<ast::Statement>>{}));

  // test() { foo(42); }  // Only one argument instead of two
  auto args = std::vector<std::unique_ptr<ast::Expression>>{};
  args.emplace_back(
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42)));
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("SemanticCall_TooManyArguments") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // Define foo(x: Int): Bool
  std::vector<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")}};
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      std::vector<std::unique_ptr<ast::Statement>>{}));

  // test() { foo(42, "extra"); }  // Two arguments instead of one
  auto args = std::vector<std::unique_ptr<ast::Expression>>{};
  args.emplace_back(
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42)));
  args.emplace_back(std::make_unique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("extra"))));
  auto call_expr = std::make_unique<ast::CallExpression>(
      std::make_unique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(
      std::make_unique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "test", std::vector<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("SemanticFunction_MismatchedReturnType") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  // fun foo(): Int { return "not an int"; }
  auto body = std::vector<std::unique_ptr<ast::Statement>>{};
  body.emplace_back(std::make_unique<ast::ReturnStatement>(
      std::make_unique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("not an int")))));
  ast.emplace_back(std::make_unique<ast::FunctionDeclaration>(
      "foo", std::vector<ast::FunctionParameter>{},
      VellumType::unresolved("Int"), std::move(body)));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  auto resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier("TestScript")), errorHandler);
  const auto result =
      SemanticAnalyzer(errorHandler, resolver).analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}
