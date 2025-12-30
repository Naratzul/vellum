#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "analyze/semantic_analyzer.h"
#include "ast/decl/declaration.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "utils.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_property.h"

using namespace vellum;
using common::makeShared;
using common::makeUnique;
using common::Shared;
using common::Unique;
using common::Vec;

class SemanticTestsFixture {
 public:
  SemanticTestsFixture() {
    errorHandler = makeShared<CompilerErrorHandler>();
    resolver = makeShared<Resolver>(
        VellumObject(VellumType::identifier("TestScript")), errorHandler);
    analyzer =
        makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");
  }

 protected:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
  Shared<SemanticAnalyzer> analyzer;
};

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticGlobalVarTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::GlobalVariableDeclaration expected(
      "number", VellumType::literal(VellumLiteralType::Int),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAutoPropertyTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "MyProperty", VellumType::unresolved("String"), "", std::nullopt,
      std::nullopt, VellumValue()));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::PropertyDeclaration expected(
      "MyProperty", VellumType::literal(VellumLiteralType::String), "",
      std::nullopt, std::nullopt, VellumValue());
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticFunctionTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::FunctionDeclaration expected(
      "foo", {}, VellumType::literal(VellumLiteralType::Bool), {});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_NoArgs") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(): Int
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}));

  // test() { foo(); }
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

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

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_WithArgs") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}));

  // test() { foo(42, "hi"); }
  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(42)));
  args.emplace_back(makeUnique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("hi"))));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

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

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_UndefinedFunction") {
  Vec<Unique<ast::Declaration>> ast;
  // test() { notDefined(); }
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("notDefined")),
      Vec<Unique<ast::Expression>>{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_ArgumentTypeMismatch") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}));

  // test() { foo("not an int", 123); }
  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(makeUnique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("not an int"))));
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(123)));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_TooFewArguments") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}));

  // test() { foo(42); }  // Only one argument instead of two
  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(42)));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_TooManyArguments") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int): Bool
  Vec<ast::FunctionParameter> params = {{"x", VellumType::unresolved("Int")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}));

  // test() { foo(42, "extra"); }  // Two arguments instead of one
  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(42)));
  args.emplace_back(makeUnique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("extra"))));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(
          VellumIdentifier((std::string_view) "foo")),
      std::move(args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticFunction_MismatchedReturnType") {
  Vec<Unique<ast::Declaration>> ast;
  // fun foo(): Int { return "not an int"; }
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ReturnStatement>(makeUnique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("not an int")))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

// Assignment target validation tests
TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToVariable_Success") {
  Vec<Unique<ast::Declaration>> ast;
  // var number: Int = 42
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  // test() { number = 100; }
  auto assign_expr = makeUnique<ast::AssignExpression>(
      VellumIdentifier("number"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(100)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToProperty_Success") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with a property
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("myProp"), VellumType::literal(VellumLiteralType::Int)));
  resolver->importObject(testObject);

  // test() { TestObject.myProp = 42; }
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myProp"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToFunction_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with a function
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addFunction(VellumFunction(
      VellumIdentifier("myFunc"), VellumType::literal(VellumLiteralType::Int),
      Vec<VellumVariable>{}));
  resolver->importObject(testObject);

  // test() { TestObject.myFunc = 42; }  // Error: cannot assign to function
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myFunc"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToMethod_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with a method (function with parameters)
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addFunction(
      VellumFunction(VellumIdentifier("myMethod"), VellumType::none(),
                     Vec<VellumVariable>{VellumVariable(
                         VellumIdentifier("param"),
                         VellumType::literal(VellumLiteralType::Int))}));
  resolver->importObject(testObject);

  // test() { TestObject.myMethod = 42; }  // Error: cannot assign to method
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myMethod"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToTypeName_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // test() { Int = 42; }  // Error: cannot assign to type name
  auto assign_expr = makeUnique<ast::AssignExpression>(
      VellumIdentifier("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_ToUndefinedVariable_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // test() { undefinedVar = 42; }  // Error: undefined variable
  auto assign_expr = makeUnique<ast::AssignExpression>(
      VellumIdentifier("undefinedVar"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_ToUndefinedProperty_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object without the property we'll try to assign to
  VellumObject testObject(VellumType::identifier("TestObject"));
  resolver->importObject(testObject);

  // test() { TestObject.nonExistent = 42; }  // Error: undefined property
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("nonExistent"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)), Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_TypeMismatch_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // var number: Int = 42
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  // test() { number = "not an int"; }  // Error: type mismatch
  auto assign_expr = makeUnique<ast::AssignExpression>(
      VellumIdentifier("number"),
      makeUnique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("not an int"))),
      Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_PropertyTypeMismatch_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with an Int property
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("myProp"), VellumType::literal(VellumLiteralType::Int)));
  resolver->importObject(testObject);

  // test() { TestObject.myProp = "not an int"; }  // Error: type mismatch
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myProp"),
      makeUnique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("not an int"))),
      Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body)));

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}
