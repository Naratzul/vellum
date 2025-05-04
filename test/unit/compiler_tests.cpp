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
      "number", VellumType::literal(VellumValueType::Int),
      std::make_unique<ast::LiteralExpression>(VellumValue(42))));

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