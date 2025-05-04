#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <memory>
#include <vector>

#include "analyze/semantic_analyzer.h"
#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"
#include "utils.h"

using namespace vellum;

TEST_CASE("SemanticTest") {
  std::vector<std::unique_ptr<ast::Declaration>> ast;
  ast.emplace_back(std::make_unique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      std::make_unique<ast::LiteralExpression>(VellumValue(42))));

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result = SemanticAnalyzer(errorHandler).analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::GlobalVariableDeclaration expected(
      "number", VellumType::literal(VellumValueType::Int),
      std::make_unique<ast::LiteralExpression>(VellumValue(42)));

  CHECK(expected == *result.declarations[0]);
}
