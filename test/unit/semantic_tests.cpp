#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <filesystem>

#include "analyze/declaration_collector.h"
#include "analyze/import_library.h"
#include "analyze/semantic_analyzer.h"
#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lexer/token.h"
#include "utils.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_property.h"
#include "vellum/vellum_type.h"
namespace fs = std::filesystem;

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
    importLibrary = makeShared<ImportLibrary>(Vec<std::string>{});
    auto builtinFunctions = makeShared<BuiltinFunctions>();
    resolver =
        makeShared<Resolver>(VellumObject(VellumType::identifier("testscript")),
                             errorHandler, importLibrary, builtinFunctions);
    collector =
        makeShared<DeclarationCollector>(errorHandler, resolver, "testscript");
    analyzer =
        makeShared<SemanticAnalyzer>(errorHandler, resolver, "testscript");
  }

  void addTestObject(const VellumObject& object) {
    VellumIdentifier name = object.getType().asIdentifier();
    auto module =
        makeShared<ImportModule>(name, ImportModuleType::Vellum, fs::path(""));
    auto builtinFunctions = makeShared<BuiltinFunctions>();
    auto objectResolver = makeShared<Resolver>(object, errorHandler,
                                               importLibrary, builtinFunctions);
    module->setResolver(objectResolver);
    importLibrary->addTestModule(module);
    resolver->importObject(name);
  }

 protected:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<ImportLibrary> importLibrary;
  Shared<Resolver> resolver;
  Shared<DeclarationCollector> collector;
  Shared<SemanticAnalyzer> analyzer;
};

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticGlobalVarTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  collector->collect(ast);

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
      std::nullopt, std::nullopt));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::PropertyDeclaration expected(
      "MyProperty", VellumType::literal(VellumLiteralType::String), "",
      std::nullopt, std::nullopt, std::nullopt);
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticProperty_WithIntDefaultValue") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "value", VellumType::unresolved("Int"), "", ast::FunctionBody{},
      ast::FunctionBody{}, VellumLiteral(42)));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* prop = dynamic_cast<const ast::PropertyDeclaration*>(
      result.declarations[0].get());
  REQUIRE(prop != nullptr);
  REQUIRE(prop->getDefaultValue().has_value());
  REQUIRE(prop->getDefaultValue()->getType() == VellumLiteralType::Int);
  REQUIRE(prop->getDefaultValue()->asInt() == 42);
  REQUIRE(prop->isAutoProperty());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticProperty_WithNoneDefaultValue") {
  addTestObject(VellumObject(VellumType::identifier("TestScript")));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "obj", VellumType::unresolved("TestScript"), "", ast::FunctionBody{},
      ast::FunctionBody{}, VellumLiteral()));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* prop = dynamic_cast<const ast::PropertyDeclaration*>(
      result.declarations[0].get());
  REQUIRE(prop != nullptr);
  REQUIRE(prop->getDefaultValue().has_value());
  REQUIRE(prop->getDefaultValue()->getType() == VellumLiteralType::None);
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticProperty_ReadonlyAuto_NoDefault") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "readonlyVal", VellumType::unresolved("Int"), "", ast::FunctionBody{},
      std::nullopt, std::nullopt));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* prop = dynamic_cast<const ast::PropertyDeclaration*>(
      result.declarations[0].get());
  REQUIRE(prop != nullptr);
  REQUIRE(prop->isReadonly());
  REQUIRE_FALSE(prop->getDefaultValue().has_value());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticFunctionTest") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::FunctionDeclaration expected(
      "foo", {}, VellumType::literal(VellumLiteralType::Bool), {}, false);
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_NoArgs") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(): Int
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  // test() { foo(); }
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

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
      Vec<Unique<ast::Statement>>{}, false));

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
      std::move(body), false));

  collector->collect(ast);

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
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::UndefinedFunction));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_ArgumentTypeMismatch") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false));

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
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(
      errorHandler->hasError(CompilerErrorKind::FunctionArgumentTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_TooFewArguments") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int, y: String): Bool
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("String")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false));

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
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::FunctionArgumentsCountMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_TooManyArguments") {
  Vec<Unique<ast::Declaration>> ast;
  // Define foo(x: Int): Bool
  Vec<ast::FunctionParameter> params = {{"x", VellumType::unresolved("Int")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false));

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
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::FunctionArgumentsCountMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_DefaultArgs_AllProvided") {
  Vec<Unique<ast::Declaration>> ast;
  Vec<ast::FunctionParameter> params = {
      {"a", VellumType::unresolved("Int")},
      {"b", VellumType::unresolved("String"),
       VellumLiteral(std::string_view("hi"))}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(10)));
  args.emplace_back(
      makeUnique<ast::LiteralExpression>(VellumLiteral(std::string_view("x"))));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::move(args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_DefaultArgs_Partial") {
  Vec<Unique<ast::Declaration>> ast;
  Vec<ast::FunctionParameter> params = {
      {"a", VellumType::unresolved("Int")},
      {"b", VellumType::unresolved("String"),
       VellumLiteral(std::string_view("hi"))}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(10)));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::move(args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticCall_DefaultArgs_AllOptional") {
  Vec<Unique<ast::Declaration>> ast;
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int"), VellumLiteral(5)}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticCall_DefaultArgs_TooFewRequired") {
  Vec<Unique<ast::Declaration>> ast;
  Vec<ast::FunctionParameter> params = {
      {"a", VellumType::unresolved("Int")},
      {"b", VellumType::unresolved("String"),
       VellumLiteral(std::string_view("hi"))}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::FunctionArgumentsCountMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticCall_DefaultArgs_TypeMismatch") {
  Vec<Unique<ast::Declaration>> ast;
  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int"),
       VellumLiteral(std::string_view("bad"))}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  collector->collect(ast);

  REQUIRE(errorHandler->hasError(CompilerErrorKind::VariableTypeMismatch));
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
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ReturnTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticReturn_BareReturn_InNoneFunction_Success") {
  Vec<Unique<ast::Declaration>> ast;
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(nullptr));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticReturn_BareReturn_InIntFunction_Error") {
  Vec<Unique<ast::Declaration>> ast;
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(nullptr));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ReturnTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticReturn_ExplicitNone_InNoneFunction_Success") {
  Vec<Unique<ast::Declaration>> ast;
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral())));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticReturn_WithValue_InNoneFunction_Error") {
  Vec<Unique<ast::Declaration>> ast;
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ReturnTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticReturn_WithValue_MatchingType_Success") {
  Vec<Unique<ast::Declaration>> ast;
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
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
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("number"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(100)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToProperty_Success") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with a property
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(
      VellumProperty(VellumIdentifier("myProp"),
                     VellumType::literal(VellumLiteralType::Int), false));
  addTestObject(testObject);

  // test() { TestObject.myProp = 42; }
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myProp"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticComposedAssign_ToVariable_Success") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("x"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(10)),
      ast::AssignOperator::Add, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticComposedAssign_TypeMismatch_Error") {
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "n", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));

  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("n"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(std::string_view("s"))),
      ast::AssignOperator::Add, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::AssignTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticComposedAssign_ReadonlyProperty_Error") {
  Vec<Unique<ast::Declaration>> ast;
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(
      VellumProperty(VellumIdentifier("readonlyProp"),
                     VellumType::literal(VellumLiteralType::Int), true));
  addTestObject(testObject);

  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("readonlyProp"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      ast::AssignOperator::Add, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::NotAssignable));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToFunction_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with a function
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addFunction(VellumFunction(
      VellumIdentifier("myFunc"), VellumType::literal(VellumLiteralType::Int),
      Vec<VellumVariable>{}, false));
  addTestObject(testObject);

  // test() { TestObject.myFunc = 42; }  // Error: cannot assign to function
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myFunc"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::NotAssignable));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToMethod_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with a method (function with parameters)
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addFunction(
      VellumFunction(VellumIdentifier("myMethod"), VellumType::none(),
                     Vec<VellumVariable>{VellumVariable(
                         VellumIdentifier("param"),
                         VellumType::literal(VellumLiteralType::Int))},
                     false));
  addTestObject(testObject);

  // test() { TestObject.myMethod = 42; }  // Error: cannot assign to method
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myMethod"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::NotAssignable));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_ToTypeName_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // test() { Int = 42; }  // Error: cannot assign to type name
  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("Int"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::UndefinedIdentifier));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_ToUndefinedVariable_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // test() { undefinedVar = 42; }  // Error: undefined variable
  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("undefinedVar"),
                                              Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::UndefinedIdentifier));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_ToUndefinedProperty_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object without the property we'll try to assign to
  VellumObject testObject(VellumType::identifier("TestObject"));
  addTestObject(testObject);

  // test() { TestObject.nonExistent = 42; }  // Error: undefined property
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("nonExistent"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::UndefinedProperty));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticAssign_TypeMismatch_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // var number: Int = 42
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  // test() { number = "not an int"; }  // Error: type mismatch
  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("number"), Token{}),
      makeUnique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("not an int"))),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::AssignTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_PropertyTypeMismatch_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with an Int property
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(
      VellumProperty(VellumIdentifier("myProp"),
                     VellumType::literal(VellumLiteralType::Int), false));
  addTestObject(testObject);

  // test() { TestObject.myProp = "not an int"; }  // Error: type mismatch
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myProp"),
      makeUnique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("not an int"))),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::AssignTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticMultipleScriptsDefinition_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // First script declaration
  Token script1Token = makeToken(TokenType::IDENTIFIER, 1, "testscript");
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), script1Token, VellumType::none(),
      std::nullopt, Vec<Unique<ast::Declaration>>{}));

  // Second script declaration (should trigger error)
  Token script2Token = makeToken(TokenType::IDENTIFIER, 2, "AnotherScript");
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("AnotherScript"), script2Token, VellumType::none(),
      std::nullopt, Vec<Unique<ast::Declaration>>{}));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::MultipleScriptsDefinition));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticCall_PassFunctionAsParameter_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // Define a function that takes an Int parameter
  // fun foo(fn: Int): Int
  Vec<ast::FunctionParameter> params = {{"fn", VellumType::unresolved("Int")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  // Define a function that returns Int
  // fun bar(): Int
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  // test() { foo(bar); }  // Error: cannot pass function as argument
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  Vec<Unique<ast::Expression>> call_args;
  call_args.push_back(std::move(bar_expr));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::move(call_args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: passing a function as an argument is not allowed
  // Even though bar returns Int and foo expects Int, you cannot pass the
  // function itself - only values can be passed as arguments
  REQUIRE(
      errorHandler->hasError(CompilerErrorKind::FunctionArgumentTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticCall_PassScriptTypeAsInstance_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // Create a script type for the test object
  VellumObject testScript(VellumType::identifier("TestScript"));
  addTestObject(testScript);

  // Define a function that takes a TestScript instance parameter
  // fun foo(obj: TestScript): Int
  Vec<ast::FunctionParameter> params = {
      {"obj", VellumType::unresolved("TestScript")}};
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", params, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  // test() { foo(TestScript); }  // Error: cannot pass script type as instance
  auto scriptType_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestScript"));
  Vec<Unique<ast::Expression>> call_args;
  call_args.push_back(std::move(scriptType_expr));
  auto call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::move(call_args));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(call_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: passing a script type as an argument where an instance
  // is expected is not allowed. Even though TestScript type matches TestScript
  // type, you cannot pass the type itself - only instances can be passed
  REQUIRE(
      errorHandler->hasError(CompilerErrorKind::FunctionArgumentTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_FunctionToVariable_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // var myVar: Int = 10
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "myVar", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(10))));

  // fun bar(): Int { return 42; }
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  // test() { myVar = bar; }  // Error: cannot assign function to variable
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  auto assign_expr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("myVar"), Token{}),
      std::move(bar_expr), ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot assign a function to a variable
  // Even though bar returns Int and myVar is Int, you cannot assign the
  // function itself - only values can be assigned
  REQUIRE(errorHandler->hasError(CompilerErrorKind::AssignTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticAssign_FunctionToProperty_Error") {
  Vec<Unique<ast::Declaration>> ast;
  // Create an object with an Int property
  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(
      VellumProperty(VellumIdentifier("myProperty"),
                     VellumType::literal(VellumLiteralType::Int), false));
  addTestObject(testObject);

  // fun bar(): Int { return 42; }
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  // test() { TestObject.myProperty = bar; }  // Error: cannot assign function
  // to property
  auto obj_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  auto assign_expr = makeUnique<ast::PropertySetExpression>(
      std::move(obj_expr), VellumIdentifier("myProperty"), std::move(bar_expr),
      ast::AssignOperator::Assign, Token{});
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(assign_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot assign a function to a property
  // Even though bar returns Int and myProperty is Int, you cannot assign the
  // function itself - only values can be assigned
  REQUIRE(errorHandler->hasError(CompilerErrorKind::AssignTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticReturn_ReturnFunction_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // fun foo() -> Int
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  // test() -> Int { return foo; }  // Error: cannot return function
  auto body = Vec<Unique<ast::Statement>>{};
  auto foo_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo"));
  body.emplace_back(makeUnique<ast::ReturnStatement>(std::move(foo_expr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot return a function
  // Even though foo returns Int and test expects Int, you cannot return the
  // function itself - only values can be returned
  REQUIRE(errorHandler->hasError(CompilerErrorKind::ReturnTypeMismatch));
}

// Binary operator type checking tests
TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticBinaryOp_FunctionAsOperand_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // fun bar(): Int { return 42; }
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  // test() { var result = 42 + bar; }  // Error: cannot use function as operand
  auto left_expr = makeUnique<ast::LiteralExpression>(VellumLiteral(42));
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  auto binary_expr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Add, std::move(left_expr),
      std::move(bar_expr));
  auto var_stmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("result"), std::nullopt, std::move(binary_expr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(var_stmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot use a function as an operand in binary operations
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::ArithmeticOperationTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticBinaryOp_ScriptTypeAsOperand_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // Create a script type
  VellumObject testScript(VellumType::identifier("TestScript"));
  addTestObject(testScript);

  // test() { var result = TestScript == TestScript; }  // Error: cannot use
  // script type as operand
  auto scriptType_expr1 =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestScript"));
  auto scriptType_expr2 =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestScript"));
  auto binary_expr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Equal, std::move(scriptType_expr1),
      std::move(scriptType_expr2));
  auto var_stmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("result"), std::nullopt, std::move(binary_expr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(var_stmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot use a script type as an operand in binary
  // operations
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::ArithmeticOperationTypeMismatch));
}

// Unary operator type checking tests
TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticUnaryOp_FunctionAsOperand_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // fun bar(): Int { return 42; }
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  // test() { var result = -bar; }  // Error: cannot use function as operand
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  auto unary_expr = makeUnique<ast::UnaryExpression>(
      ast::UnaryExpression::Operator::Negate, std::move(bar_expr));
  auto var_stmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("result"), std::nullopt, std::move(unary_expr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(var_stmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot use a function as an operand in unary operations
  REQUIRE(errorHandler->hasError(CompilerErrorKind::UnaryOperatorTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticUnaryOp_ScriptTypeAsOperand_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // Create a script type
  VellumObject testScript(VellumType::identifier("TestScript"));
  addTestObject(testScript);

  // test() { var result = not TestScript; }  // Error: cannot use script type
  // as operand
  auto scriptType_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestScript"));
  auto unary_expr = makeUnique<ast::UnaryExpression>(
      ast::UnaryExpression::Operator::Not, std::move(scriptType_expr));
  auto var_stmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("result"), std::nullopt, std::move(unary_expr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(var_stmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot use a script type as an operand in unary
  // operations
  REQUIRE(errorHandler->hasError(CompilerErrorKind::UnaryOperatorTypeMismatch));
}

// Variable initialization type checking tests
TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticInit_FunctionAsInitializer_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // fun bar(): Int { return 42; }
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  // test() { var myVar: Int = bar; }  // Error: cannot initialize with function
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  auto var_stmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("myVar"), VellumType::unresolved("Int"),
      std::move(bar_expr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(var_stmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot initialize a variable with a function
  // Even though bar returns Int and myVar is Int, you cannot initialize with
  // the function itself - only values can be used
  REQUIRE(errorHandler->hasError(CompilerErrorKind::VariableTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticInit_ScriptTypeAsInitializer_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // Create a script type
  VellumObject testScript(VellumType::identifier("TestScript"));
  addTestObject(testScript);

  // test() { var myVar: TestScript = TestScript; }  // Error: cannot initialize
  // with script type
  auto scriptType_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestScript"));
  auto var_stmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("myVar"), VellumType::unresolved("TestScript"),
      std::move(scriptType_expr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(var_stmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot initialize a variable with a script type
  // Even though TestScript type matches TestScript type, you cannot initialize
  // with the type itself - only instances can be used
  REQUIRE(errorHandler->hasError(CompilerErrorKind::VariableTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticGlobalInit_FunctionAsInitializer_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // fun bar(): Int { return 42; }
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  // var myVar: Int = bar  // Error: cannot initialize global with function
  auto bar_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar"));
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "myVar", VellumType::unresolved("Int"), std::move(bar_expr)));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot initialize a global variable with a function
  REQUIRE(errorHandler->hasError(CompilerErrorKind::VariableTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticGlobalInit_ScriptTypeAsInitializer_Error") {
  Vec<Unique<ast::Declaration>> ast;

  // Create a script type
  VellumObject testScript(VellumType::identifier("TestScript"));
  addTestObject(testScript);

  // var myVar: TestScript = TestScript  // Error: cannot initialize global with
  // script type
  auto scriptType_expr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestScript"));
  ast.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "myVar", VellumType::unresolved("TestScript"),
      std::move(scriptType_expr)));

  collector->collect(ast);

  const auto result = analyzer->analyze(std::move(ast));

  // This should error: cannot initialize a global variable with a script type
  REQUIRE(errorHandler->hasError(CompilerErrorKind::VariableTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayIndex_ValidIntIndex") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  auto arrayIndex = makeUnique<ast::ArrayIndexExpression>(
      std::move(propGet), std::move(indexExpr), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(arrayIndex)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  REQUIRE(funcDecl.getBody().size() == 1);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*funcDecl.getBody()[0]);
  const auto& indexExprResult =
      dynamic_cast<ast::ArrayIndexExpression&>(*stmt.getExpression());
  CHECK(indexExprResult.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticArrayIndex_NonArrayTarget_Error") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(
      VellumProperty(VellumIdentifier("valueProp"),
                     VellumType::literal(VellumLiteralType::Int), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("valueProp"), Token{});
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  auto arrayIndex = makeUnique<ast::ArrayIndexExpression>(
      std::move(propGet), std::move(indexExpr), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(arrayIndex)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ArrayIndexNotArray));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayIndex_NonIntIndex_Error") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto indexExpr = makeUnique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("not-int")), Token{});
  auto arrayIndex = makeUnique<ast::ArrayIndexExpression>(
      std::move(propGet), std::move(indexExpr), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(arrayIndex)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ArrayIndexNotInt));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticArrayIndex_AssignValidElement") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  auto valueExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(42), Token{});
  auto arraySet = makeUnique<ast::ArrayIndexSetExpression>(
      std::move(propGet), std::move(indexExpr), std::move(valueExpr),
      ast::AssignOperator::Assign, Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(arraySet)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticArrayIndex_AssignTypeMismatch_Error") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  auto valueExpr = makeUnique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("not-int")), Token{});
  auto arraySet = makeUnique<ast::ArrayIndexSetExpression>(
      std::move(propGet), std::move(indexExpr), std::move(valueExpr),
      ast::AssignOperator::Assign, Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(arraySet)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::AssignTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticInheritance_SuperCall_Resolves") {
  VellumObject parentObj(VellumType::identifier("ParentScript"));
  parentObj.addFunction(
      VellumFunction(VellumIdentifier("foo"),
                     VellumType::literal(VellumLiteralType::Int), {}, false));
  addTestObject(parentObj);

  Token scriptToken = makeToken(TokenType::IDENTIFIER, 1, "testscript");
  Token parentToken = makeToken(TokenType::IDENTIFIER, 1, "ParentScript");
  auto superExpr = makeUnique<ast::SuperExpression>(Token());
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(superExpr), VellumIdentifier("foo"), Token());
  auto callExpr = makeUnique<ast::CallExpression>(
      std::move(propGet), Vec<Unique<ast::Expression>>{}, Token());
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"), scriptToken,
      VellumType::identifier("ParentScript"), parentToken, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& scriptDecl =
      dynamic_cast<ast::ScriptDeclaration&>(*result.declarations[0]);
  REQUIRE(scriptDecl.getMemberDecls().size() == 1);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*scriptDecl.getMemberDecls()[0]);
  REQUIRE(funcDecl.getBody().size() == 1);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*funcDecl.getBody()[0]);
  const auto& call = dynamic_cast<ast::CallExpression&>(*stmt.getExpression());
  REQUIRE(call.getFunctionCall().has_value());
  REQUIRE(call.getFunctionCall()->isParentCall());
  CHECK(call.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticInheritance_SuperInScriptWithNoParent_Error") {
  auto superExpr = makeUnique<ast::SuperExpression>(Token());
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(superExpr), VellumIdentifier("foo"), Token());
  auto callExpr = makeUnique<ast::CallExpression>(
      std::move(propGet), Vec<Unique<ast::Expression>>{}, Token());
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::UndefinedFunction));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticSelfExpression_HasScriptType") {
  auto selfExpr = makeUnique<ast::SelfExpression>(Token());
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(selfExpr), VellumIdentifier("foo"), Token());
  auto callExpr = makeUnique<ast::CallExpression>(
      std::move(propGet), Vec<Unique<ast::Expression>>{}, Token());
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::literal(VellumLiteralType::Int),
      ast::FunctionBody{}, false));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& scriptDecl =
      dynamic_cast<ast::ScriptDeclaration&>(*result.declarations[0]);
  REQUIRE(scriptDecl.getMemberDecls().size() == 2);
  const auto& testFuncDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*scriptDecl.getMemberDecls()[1]);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*testFuncDecl.getBody()[0]);
  const auto& call = dynamic_cast<ast::CallExpression&>(*stmt.getExpression());
  REQUIRE(call.getCallee()->isPropertyGetExpression());
  REQUIRE(call.getCallee()->asPropertyGet().getObject()->isSelfExpression());
  CHECK(call.getCallee()->asPropertyGet().getObject()->getType() ==
        VellumType::identifier("testscript"));
  REQUIRE(call.getFunctionCall().has_value());
  REQUIRE_FALSE(call.getFunctionCall()->isStatic());
  REQUIRE_FALSE(call.getFunctionCall()->isParentCall());
  CHECK(call.getFunctionCall()->getObject().toString() == "self");
  CHECK(call.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticInheritance_PropertyShadow_Error") {
  VellumObject parentObj(VellumType::identifier("ParentScript"));
  parentObj.addProperty(
      VellumProperty(VellumIdentifier("X"),
                     VellumType::literal(VellumLiteralType::Int), false));
  addTestObject(parentObj);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::PropertyDeclaration>(
      "X", VellumType::unresolved("Int"), "", std::nullopt, std::nullopt,
      std::nullopt));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"),
      VellumType::identifier("ParentScript"),
      makeToken(TokenType::IDENTIFIER, 1, "ParentScript"), std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::CannotOverrideProperty));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticInheritance_CallInheritedFunction") {
  VellumObject parentObj(VellumType::identifier("ParentScript"));
  parentObj.addFunction(
      VellumFunction(VellumIdentifier("inherited"),
                     VellumType::literal(VellumLiteralType::Bool), {}, false));
  addTestObject(parentObj);

  auto callExpr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("inherited")),
      Vec<Unique<ast::Expression>>{}, Token());
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"),
      VellumType::identifier("ParentScript"),
      makeToken(TokenType::IDENTIFIER, 1, "ParentScript"), std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& scriptDecl =
      dynamic_cast<ast::ScriptDeclaration&>(*result.declarations[0]);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*scriptDecl.getMemberDecls()[0]);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*funcDecl.getBody()[0]);
  const auto& call = dynamic_cast<ast::CallExpression&>(*stmt.getExpression());
  CHECK(call.getType().isBool());
}

// ============================================================================
// State Tests
// ============================================================================

TEST_CASE_METHOD(SemanticTestsFixture, "State_OnlyOneAutoState") {
  Vec<Unique<ast::Declaration>> ast;

  // First auto state - should be OK
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "State1", makeToken(TokenType::IDENTIFIER, 1, "State1"), true,
      Vec<Unique<ast::Declaration>>{}));

  // Second auto state - should error
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "State2", makeToken(TokenType::IDENTIFIER, 2, "State2"), true,
      Vec<Unique<ast::Declaration>>{}));

  collector->collect(ast);

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::MultipleAutoStates));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_OnlyFunctionsAllowed") {
  Vec<Unique<ast::Declaration>> ast;

  // Try to declare a variable in a state - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::GlobalVariableDeclaration>(
      "myVar", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 1, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::ExpectDeclaration));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_PropertyNotAllowed") {
  Vec<Unique<ast::Declaration>> ast;

  // Try to declare a property in a state - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::PropertyDeclaration>(
      "MyProperty", VellumType::unresolved("String"), "", std::nullopt,
      std::nullopt, std::nullopt));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 1, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::ExpectDeclaration));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_FunctionMustBeInEmptyState") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function only in state, not in empty state - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 1, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 1, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(
      errorHandler->hasError(CompilerErrorKind::StateFunctionNotInEmptyState));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_FunctionSignatureMustMatch") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function in empty state with return type Int
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 1, "myFunc")));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same function in state with different return type - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::StateFunctionSignatureMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_FunctionParameterCountMismatch") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function in empty state with one parameter
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc",
      Vec<ast::FunctionParameter>{
          ast::FunctionParameter{"x", VellumType::unresolved("Int")}},
      VellumType::unresolved("Int"), Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same function in state with no parameters - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::StateFunctionSignatureMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_FunctionParameterTypeMismatch") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function in empty state with Int parameter
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc",
      Vec<ast::FunctionParameter>{
          ast::FunctionParameter{"x", VellumType::unresolved("Int")}},
      VellumType::unresolved("Int"), Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same function in state with Bool parameter - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc",
      Vec<ast::FunctionParameter>{
          ast::FunctionParameter{"x", VellumType::unresolved("Bool")}},
      VellumType::unresolved("Int"), Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::StateFunctionSignatureMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_FunctionDefaultValueMismatch") {
  Vec<Unique<ast::Declaration>> ast;

  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc",
      Vec<ast::FunctionParameter>{ast::FunctionParameter{
          "x", VellumType::unresolved("Int"), VellumLiteral(5)}},
      VellumType::unresolved("Int"), Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc",
      Vec<ast::FunctionParameter>{ast::FunctionParameter{
          "x", VellumType::unresolved("Int"), VellumLiteral(10)}},
      VellumType::unresolved("Int"), Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::StateFunctionSignatureMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_FunctionStaticMismatch") {
  Vec<Unique<ast::Declaration>> ast;

  // Define static function in empty state
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, true));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same function in state as non-static - should error
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::StateFunctionSignatureMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_ValidStateWithFunction") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function in empty state
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same function in state with matching signature - should be OK
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_ValidAutoState") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function in empty state
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define auto state with matching function - should be OK
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), true,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "State_MultipleStatesWithValidFunctions") {
  Vec<Unique<ast::Declaration>> ast;

  // Define function in empty state
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define first state with matching function
  Vec<Unique<ast::Declaration>> state1Members;
  state1Members.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "State1", makeToken(TokenType::IDENTIFIER, 2, "State1"), false,
      std::move(state1Members)));

  // Define second state with matching function - should be OK
  Vec<Unique<ast::Declaration>> state2Members;
  state2Members.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "myFunc", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 3, "myFunc")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "State2", makeToken(TokenType::IDENTIFIER, 3, "State2"), false,
      std::move(state2Members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 3);
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_EventAllowedInState") {
  Vec<Unique<ast::Declaration>> ast;

  // Define event in empty state
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "onActivate", Vec<ast::FunctionParameter>{}, VellumType::none(),
      Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same event in state - should be OK (events are functions)
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "onActivate", Vec<ast::FunctionParameter>{}, VellumType::none(),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "onActivate")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_EmptyStateAllowed") {
  Vec<Unique<ast::Declaration>> ast;

  // Define script
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, Vec<Unique<ast::Declaration>>{}));

  // Define state with no functions - should be OK
  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      Vec<Unique<ast::Declaration>>{}));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
}

TEST_CASE_METHOD(SemanticTestsFixture, "State_MultipleFunctionsInState") {
  Vec<Unique<ast::Declaration>> ast;

  // Define multiple functions in empty state
  Vec<Unique<ast::Declaration>> scriptMembers;
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "func1", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false));
  scriptMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "func2", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false));

  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(scriptMembers)));

  // Define same functions in state - should be OK
  Vec<Unique<ast::Declaration>> stateMembers;
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "func1", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "func1")));
  stateMembers.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "func2", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Bool"),
      Vec<Unique<ast::Statement>>{}, false,
      makeToken(TokenType::IDENTIFIER, 2, "func2")));

  ast.emplace_back(makeUnique<ast::StateDeclaration>(
      "MyState", makeToken(TokenType::IDENTIFIER, 2, "MyState"), false,
      std::move(stateMembers)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 2);
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayLength_Valid") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto lengthGet = makeUnique<ast::PropertyGetExpression>(
      std::move(propGet), VellumIdentifier("length"), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(lengthGet)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  REQUIRE(funcDecl.getBody().size() == 1);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*funcDecl.getBody()[0]);
  const auto& lengthExpr =
      dynamic_cast<ast::PropertyGetExpression&>(*stmt.getExpression());
  CHECK(lengthExpr.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayLength_NonArray_Error") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(
      VellumProperty(VellumIdentifier("valueProp"),
                     VellumType::literal(VellumLiteralType::Int), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("valueProp"), Token{});
  auto lengthGet = makeUnique<ast::PropertyGetExpression>(
      std::move(propGet), VellumIdentifier("length"), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(lengthGet)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::UndefinedProperty));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayLength_Readonly_Error") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto valueExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(5), Token{});
  auto lengthSet = makeUnique<ast::PropertySetExpression>(
      std::move(propGet), VellumIdentifier("length"), std::move(valueExpr),
      ast::AssignOperator::Assign, Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(lengthSet)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::NotAssignable));
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayFind_Valid") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto findProp = makeUnique<ast::PropertyGetExpression>(
      std::move(propGet), VellumIdentifier("find"), Token{});
  auto valueExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(42), Token{});
  Vec<Unique<ast::Expression>> args;
  args.push_back(std::move(valueExpr));
  auto callExpr = makeUnique<ast::CallExpression>(std::move(findProp),
                                                  std::move(args), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  REQUIRE(funcDecl.getBody().size() == 1);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*funcDecl.getBody()[0]);
  const auto& call = dynamic_cast<ast::CallExpression&>(*stmt.getExpression());
  CHECK(call.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayFind_WithStartIndex") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto findProp = makeUnique<ast::PropertyGetExpression>(
      std::move(propGet), VellumIdentifier("find"), Token{});
  auto valueExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(42), Token{});
  auto indexExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(0), Token{});
  Vec<Unique<ast::Expression>> args2;
  args2.push_back(std::move(valueExpr));
  args2.push_back(std::move(indexExpr));
  auto callExpr = makeUnique<ast::CallExpression>(std::move(findProp),
                                                  std::move(args2), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayFind_WrongType_Error") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto findProp = makeUnique<ast::PropertyGetExpression>(
      std::move(propGet), VellumIdentifier("find"), Token{});
  auto valueExpr = makeUnique<ast::LiteralExpression>(
      VellumLiteral(std::string_view("wrong-type")), Token{});
  Vec<Unique<ast::Expression>> args3;
  args3.push_back(std::move(valueExpr));
  auto callExpr = makeUnique<ast::CallExpression>(std::move(findProp),
                                                  std::move(args3), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticArrayRFind_Valid") {
  Vec<Unique<ast::Declaration>> ast;

  VellumObject testObject(VellumType::identifier("TestObject"));
  testObject.addProperty(VellumProperty(
      VellumIdentifier("arrayProp"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)), false));
  addTestObject(testObject);

  auto objExpr =
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("TestObject"));
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(objExpr), VellumIdentifier("arrayProp"), Token{});
  auto rfindProp = makeUnique<ast::PropertyGetExpression>(
      std::move(propGet), VellumIdentifier("rfind"), Token{});
  auto valueExpr =
      makeUnique<ast::LiteralExpression>(VellumLiteral(42), Token{});
  Vec<Unique<ast::Expression>> args4;
  args4.push_back(std::move(valueExpr));
  auto callExpr = makeUnique<ast::CallExpression>(std::move(rfindProp),
                                                  std::move(args4), Token{});

  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  REQUIRE(funcDecl.getBody().size() == 1);
  const auto& stmt =
      dynamic_cast<ast::ExpressionStatement&>(*funcDecl.getBody()[0]);
  const auto& call = dynamic_cast<ast::CallExpression&>(*stmt.getExpression());
  CHECK(call.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "StaticContext_InstanceVariableRead_Error") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));
  auto readExpr = makeUnique<ast::IdentifierExpression>(VellumIdentifier("x"));
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(readExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "StaticContext_InstanceVariableWrite_Error") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::GlobalVariableDeclaration>(
      "x", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(0))));
  auto assignExpr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("x"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      ast::AssignOperator::Assign, Token{});
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(assignExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "StaticContext_InstancePropertyRead_Error") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::PropertyDeclaration>(
      "P", VellumType::unresolved("Int"), "", std::nullopt, std::nullopt,
      std::nullopt));
  auto readExpr = makeUnique<ast::IdentifierExpression>(VellumIdentifier("P"));
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(readExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "StaticContext_InstancePropertyWrite_Error") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::PropertyDeclaration>(
      "P", VellumType::unresolved("Int"), "", std::nullopt, std::nullopt,
      std::nullopt));
  auto assignExpr = makeUnique<ast::AssignExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("P"), Token{}),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      ast::AssignOperator::Assign, Token{});
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(assignExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "StaticContext_InstanceMethodCall_Error") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "instanceFunc", Vec<ast::FunctionParameter>{}, VellumType::none(),
      Vec<Unique<ast::Statement>>{}, false));
  auto callExpr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("instanceFunc")),
      Vec<Unique<ast::Expression>>{}, Token{});
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture, "StaticContext_Super_Error") {
  VellumObject parentObj(VellumType::identifier("ParentScript"));
  parentObj.addFunction(VellumFunction(
      VellumIdentifier("parentMethod"), VellumType::none(), {}, false));
  addTestObject(parentObj);

  Vec<Unique<ast::Declaration>> members;
  auto superGet = makeUnique<ast::PropertyGetExpression>(
      makeUnique<ast::SuperExpression>(Token()),
      VellumIdentifier("parentMethod"), Token());
  auto callExpr = makeUnique<ast::CallExpression>(
      std::move(superGet), Vec<Unique<ast::Expression>>{}, Token{});
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"),
      VellumType::identifier("ParentScript"),
      makeToken(TokenType::IDENTIFIER, 1, "ParentScript"), std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture, "StaticContext_SelfProperty_Error") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::PropertyDeclaration>(
      "P", VellumType::unresolved("Int"), "", std::nullopt, std::nullopt,
      std::nullopt));
  auto selfProp = makeUnique<ast::PropertyGetExpression>(
      makeUnique<ast::SelfExpression>(Token()), VellumIdentifier("P"),
      Token{});
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(selfProp)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture, "StaticContext_BareSelf_Error") {
  Vec<Unique<ast::Declaration>> members;
  auto selfExpr = makeUnique<ast::SelfExpression>(Token());
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(selfExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::InstanceMemberInStaticContext));
}

TEST_CASE_METHOD(SemanticTestsFixture, "Break_OutsideLoop_Error") {
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::BreakStatement>(
      makeToken(TokenType::BREAK, 1, "break")));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::BreakOutsideLoop));
}

TEST_CASE_METHOD(SemanticTestsFixture, "Break_InsideWhile_NoError") {
  ast::FunctionBody body;
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> loopBody;
  loopBody.push_back(makeUnique<ast::BreakStatement>(
      makeToken(TokenType::BREAK, 1, "break")));
  body.push_back(makeUnique<ast::WhileStatement>(std::move(cond),
                                                  std::move(loopBody)));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
}

TEST_CASE_METHOD(SemanticTestsFixture, "Continue_OutsideLoop_Error") {
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ContinueStatement>(
      makeToken(TokenType::CONTINUE, 1, "continue")));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::ContinueOutsideLoop));
}

TEST_CASE_METHOD(SemanticTestsFixture, "Continue_InsideWhile_NoError") {
  ast::FunctionBody body;
  auto cond = makeUnique<ast::LiteralExpression>(VellumLiteral(true));
  cond->setType(VellumType::literal(VellumLiteralType::Bool));
  Vec<Unique<ast::Statement>> loopBody;
  loopBody.push_back(makeUnique<ast::ContinueStatement>(
      makeToken(TokenType::CONTINUE, 1, "continue")));
  body.push_back(makeUnique<ast::WhileStatement>(std::move(cond),
                                                  std::move(loopBody)));
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
}

TEST_CASE_METHOD(SemanticTestsFixture, "ForIn_OverArray_NoError") {
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
      std::move(forBody), tokI, tokA));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
}

TEST_CASE_METHOD(SemanticTestsFixture, "ForIn_NonArrayCollection_Error") {
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto intLit = makeUnique<ast::LiteralExpression>(VellumLiteral(0));
  intLit->setType(VellumType::literal(VellumLiteralType::Int));
  Token intTok = makeToken(TokenType::INT, 1, "0", VellumLiteral(0));

  Vec<Unique<ast::Statement>> forBody;
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      std::move(intLit), std::move(forBody), tokI, intTok));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "ForIn_BreakInsideFor_NoError") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto newArr = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  Vec<Unique<ast::Statement>> forBody;
  forBody.push_back(makeUnique<ast::BreakStatement>(
      makeToken(TokenType::BREAK, 1, "break")));

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArr), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      std::move(forBody), tokI, tokA));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
}

TEST_CASE_METHOD(SemanticTestsFixture, "ForIn_ContinueInsideFor_NoError") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto newArr = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  Vec<Unique<ast::Statement>> forBody;
  forBody.push_back(makeUnique<ast::ContinueStatement>(
      makeToken(TokenType::CONTINUE, 1, "continue")));

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArr), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      std::move(forBody), tokI, tokA));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
}

TEST_CASE_METHOD(SemanticTestsFixture, "ForIn_BodyIteratorGetsPexMangledName") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto newArr = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  Vec<Unique<ast::Statement>> forBody;
  forBody.push_back(makeUnique<ast::ExpressionStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI)));

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArr), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      std::move(forBody), tokI, tokA));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* script = dynamic_cast<const ast::ScriptDeclaration*>(
      result.declarations[0].get());
  REQUIRE(script != nullptr);
  const auto* func = dynamic_cast<const ast::FunctionDeclaration*>(
      script->getMemberDecls()[0].get());
  REQUIRE(func != nullptr);
  const auto* forStmt = dynamic_cast<const ast::ForStatement*>(
      func->getBody()[1].get());
  REQUIRE(forStmt != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      forStmt->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* idInBody =
      dynamic_cast<const ast::IdentifierExpression*>(exprStmt->getExpression().get());
  REQUIRE(idInBody != nullptr);
  REQUIRE(idInBody->getMangledIdentifier().has_value());
  CHECK(idInBody->getMangledIdentifier()->toString() == "i_1");
  CHECK(forStmt->getCounterMangledName() == "i_index_1");
}

TEST_CASE_METHOD(SemanticTestsFixture, "ForIn_NestedSameIteratorName_DistinctMangling") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokB = makeToken(TokenType::IDENTIFIER, 1, "b");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");

  auto newArrA = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});
  auto newArrB = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  Vec<Unique<ast::Statement>> innerMost;
  innerMost.push_back(makeUnique<ast::ExpressionStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI)));

  Vec<Unique<ast::Statement>> innerForWrapper;
  innerForWrapper.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("b"), tokB),
      std::move(innerMost), tokI, tokB));

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArrA), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("b"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArrB), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      std::move(innerForWrapper), tokI, tokA));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* script = dynamic_cast<const ast::ScriptDeclaration*>(
      result.declarations[0].get());
  const auto* func = dynamic_cast<const ast::FunctionDeclaration*>(
      script->getMemberDecls()[0].get());
  const auto* outerFor =
      dynamic_cast<const ast::ForStatement*>(func->getBody()[2].get());
  REQUIRE(outerFor != nullptr);
  CHECK(outerFor->getCounterMangledName() == "i_index_1");

  const auto* innerFor =
      dynamic_cast<const ast::ForStatement*>(outerFor->getBody()[0].get());
  REQUIRE(innerFor != nullptr);
  CHECK(innerFor->getCounterMangledName() == "i_index_2");

  const auto* innerExpr = dynamic_cast<const ast::ExpressionStatement*>(
      innerFor->getBody()[0].get());
  const auto* innerId = dynamic_cast<const ast::IdentifierExpression*>(
      innerExpr->getExpression().get());
  REQUIRE(innerId->getMangledIdentifier().has_value());
  CHECK(innerId->getMangledIdentifier()->toString() == "i_2");
}

TEST_CASE_METHOD(SemanticTestsFixture, "Break_OutsideLoop_AfterEmptyFor_Error") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto newArr = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArr), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      Vec<Unique<ast::Statement>>{}, tokI, tokA));
  body.push_back(makeUnique<ast::BreakStatement>(
      makeToken(TokenType::BREAK, 1, "break")));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::BreakOutsideLoop));
}

TEST_CASE_METHOD(SemanticTestsFixture, "Continue_OutsideLoop_AfterEmptyFor_Error") {
  Token tokA = makeToken(TokenType::IDENTIFIER, 1, "a");
  Token tokI = makeToken(TokenType::IDENTIFIER, 1, "i");
  auto newArr = makeUnique<ast::NewArrayExpression>(
      VellumType::literal(VellumLiteralType::Int), VellumLiteral(1), Token{});

  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("a"),
      VellumType::array(VellumType::literal(VellumLiteralType::Int)),
      std::move(newArr), Token{}, std::nullopt));
  body.push_back(makeUnique<ast::ForStatement>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("i"), tokI),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("a"), tokA),
      Vec<Unique<ast::Statement>>{}, tokI, tokA));
  body.push_back(makeUnique<ast::ContinueStatement>(
      makeToken(TokenType::CONTINUE, 1, "continue")));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::ContinueOutsideLoop));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "StaticContext_LocalsAndStaticCall_NoError") {
  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticHelper", Vec<ast::FunctionParameter>{}, VellumType::none(),
      Vec<Unique<ast::Statement>>{}, true));
  auto localInit = makeUnique<ast::LiteralExpression>(VellumLiteral(42));
  ast::FunctionBody body;
  body.push_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("x"), std::nullopt, std::move(localInit), Token{},
      std::nullopt));
  auto callExpr = makeUnique<ast::CallExpression>(
      makeUnique<ast::PropertyGetExpression>(
          makeUnique<ast::IdentifierExpression>(
              VellumIdentifier("testscript")),
          VellumIdentifier("StaticHelper"), Token()),
      Vec<Unique<ast::Expression>>{}, Token{});
  body.push_back(makeUnique<ast::ExpressionStatement>(std::move(callExpr)));
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "StaticTest", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), true));

  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
      VellumType::identifier("testscript"),
      makeToken(TokenType::IDENTIFIER, 1, "testscript"), VellumType::none(),
      std::nullopt, std::move(members)));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
}

TEST_CASE_METHOD(SemanticTestsFixture, "Cast_ValidBoolToInt") {
  Vec<Unique<ast::Declaration>> ast;
  auto castExpr = makeUnique<ast::CastExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      VellumType::unresolved("Int"), Token{});
  auto varStmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("x"), std::nullopt, std::move(castExpr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(varStmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  const auto& localStmt =
      dynamic_cast<ast::LocalVariableStatement&>(*funcDecl.getBody()[0]);
  REQUIRE(localStmt.getInitializer()->getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture, "Cast_ToArray_Invalid_Skyrim") {
  Vec<Unique<ast::Declaration>> ast;
  auto targetArrayType =
      VellumType::array(VellumType::unresolved("Int"));
  auto castExpr = makeUnique<ast::CastExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      std::move(targetArrayType), Token{});
  auto varStmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("x"), std::nullopt, std::move(castExpr));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(varStmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::InvalidCast));
}

TEST_CASE_METHOD(SemanticTestsFixture, "ImplicitIntToFloat_Assignment") {
  Vec<Unique<ast::Declaration>> ast;
  auto varStmt = makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("f"), VellumType::unresolved("Float"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(std::move(varStmt));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture, "Arithmetic_BoolNotPromoted_Error") {
  Vec<Unique<ast::Declaration>> ast;
  auto divExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Divide,
      makeUnique<ast::LiteralExpression>(VellumLiteral(9.0f)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(10.0f)));
  auto bIdent = makeUnique<ast::IdentifierExpression>(VellumIdentifier("b"));
  auto mulExpr = makeUnique<ast::BinaryExpression>(
      ast::BinaryExpression::Operator::Multiply, std::move(divExpr),
      std::move(bIdent));
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("b"), VellumType::unresolved("Bool"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(true))));
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("f"), std::nullopt, std::move(mulExpr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(
      CompilerErrorKind::ArithmeticOperationTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture, "Arithmetic_IntFloatPromotesToFloat") {
  Vec<Unique<ast::Declaration>> ast;
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
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("b"), VellumType::unresolved("Bool"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(true))));
  body.emplace_back(makeUnique<ast::LocalVariableStatement>(
      VellumIdentifier("f"), VellumType::unresolved("Float"),
      std::move(mulExpr)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  const auto& localStmt =
      dynamic_cast<ast::LocalVariableStatement&>(*funcDecl.getBody()[1]);
  REQUIRE(localStmt.getInitializer()->getType().isFloat());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticTernary_IntInt_ResultInt") {
  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(2)), loc);
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ReturnStatement>(std::move(ternary)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  const auto& ret =
      dynamic_cast<ast::ReturnStatement&>(*funcDecl.getBody()[0]);
  const auto& tern =
      dynamic_cast<ast::TernaryExpression&>(*ret.getExpression());
  REQUIRE(tern.getType().isInt());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticTernary_IntFloat_ResultFloat") {
  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(3.5f)), loc);
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ReturnStatement>(std::move(ternary)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Float"),
      std::move(body), false));

  collector->collect(ast);
  const auto result = analyzer->analyze(std::move(ast));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& funcDecl =
      dynamic_cast<ast::FunctionDeclaration&>(*result.declarations[0]);
  const auto& ret =
      dynamic_cast<ast::ReturnStatement&>(*funcDecl.getBody()[0]);
  const auto& tern =
      dynamic_cast<ast::TernaryExpression&>(*ret.getExpression());
  REQUIRE(tern.getType().isFloat());
}

TEST_CASE_METHOD(SemanticTestsFixture, "SemanticTernary_NonBoolCondition_Error") {
  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(2)), loc);
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ReturnStatement>(std::move(ternary)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body), false));

  collector->collect(ast);
  (void)analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hadError());
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticTernary_IncompatibleBranches_TernaryTypeMismatch") {
  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)),
      makeUnique<ast::LiteralExpression>(
          VellumLiteral(std::string_view("no"))),
      loc);
  auto body = Vec<Unique<ast::Statement>>{};
  body.emplace_back(
      makeUnique<ast::ReturnStatement>(std::move(ternary)));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(body), false));

  collector->collect(ast);
  (void)analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::TernaryTypeMismatch));
}

TEST_CASE_METHOD(SemanticTestsFixture,
                 "SemanticTernary_FunctionAsBranch_TernaryTypeMismatch") {
  auto barBody = Vec<Unique<ast::Statement>>{};
  barBody.emplace_back(makeUnique<ast::ReturnStatement>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));
  Vec<Unique<ast::Declaration>> ast;
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "bar", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(barBody), false));

  Token loc{};
  auto ternary = makeUnique<ast::TernaryExpression>(
      makeUnique<ast::LiteralExpression>(VellumLiteral(true)),
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar")),
      makeUnique<ast::LiteralExpression>(VellumLiteral(1)), loc);
  auto testBody = Vec<Unique<ast::Statement>>{};
  testBody.emplace_back(
      makeUnique<ast::ReturnStatement>(std::move(ternary)));
  ast.emplace_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::unresolved("Int"),
      std::move(testBody), false));

  collector->collect(ast);
  (void)analyzer->analyze(std::move(ast));

  REQUIRE(errorHandler->hasError(CompilerErrorKind::TernaryTypeMismatch));
}
