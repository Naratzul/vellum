#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <filesystem>
#include <string_view>

#include "analyze/import_library.h"
#include "analyze/semantic_analyzer.h"
#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lexer/token.h"
#include "pex/pex_file.h"
#include "pex/pex_instruction.h"
#include "pex/pex_value.h"
#include "utils.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_variable.h"

namespace fs = std::filesystem;

using namespace vellum;
using common::makeShared;
using common::makeUnique;
using common::Shared;
using common::Unique;
using common::Vec;

namespace {

constexpr std::string_view kScriptName = "testscript";
constexpr std::string_view kImportedFunc = "importedFunc";

VellumFunction makeStaticFunction(std::string_view name,
                                  VellumType returnType =
                                      VellumType::literal(VellumLiteralType::Int)) {
  return VellumFunction(VellumIdentifier(name), returnType, Vec<VellumVariable>{},
                        staticFunctionModifier);
}

Token idToken(int line, std::string_view name) {
  return makeToken(TokenType::IDENTIFIER, line, name);
}

Unique<ast::CallExpression> makeBareCall(std::string_view functionName,
                                         Token location = Token{}) {
  return makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier(functionName),
                                            location),
      Vec<Unique<ast::Expression>>{}, location);
}

Unique<ast::CallExpression> makeQualifiedCall(std::string_view importName,
                                              std::string_view functionName,
                                              Token location = Token{}) {
  auto importExpr = makeUnique<ast::IdentifierExpression>(
      VellumIdentifier(importName), location);
  auto propGet = makeUnique<ast::PropertyGetExpression>(
      std::move(importExpr), VellumIdentifier(functionName), location);
  return makeUnique<ast::CallExpression>(std::move(propGet),
                                         Vec<Unique<ast::Expression>>{},
                                         location);
}

const ast::CallExpression& firstCallInScript(
    const SemanticAnalyzeResult& result) {
  REQUIRE(result.declarations.size() >= 1);
  const auto& scriptDecl = dynamic_cast<const ast::ScriptDeclaration&>(
      *result.declarations.back());
  REQUIRE(scriptDecl.getMemberDecls().size() >= 1);
  const auto& callerDecl = dynamic_cast<const ast::FunctionDeclaration&>(
      *scriptDecl.getMemberDecls().back());
  REQUIRE(callerDecl.getBody()->getStatements().size() == 1);
  const auto& stmt = dynamic_cast<const ast::ExpressionStatement&>(
      *callerDecl.getBody()->getStatements()[0]);
  return dynamic_cast<const ast::CallExpression&>(*stmt.getExpression());
}

std::string_view pexIdentifierName(const pex::PexFile& file,
                                   const pex::PexValue& value) {
  REQUIRE(value.getType() == pex::PexValueType::Identifier);
  return file.stringTable().valueByIndex(
      static_cast<size_t>(value.asIdentifier().getValue().index()));
}

bool hasCallStatic(const pex::PexFile& file, std::string_view objectType,
                   std::string_view functionName) {
  for (const auto& obj : file.objects()) {
    for (const auto& state : obj.getStates()) {
      for (const auto& func : state.getFunctions()) {
        for (const auto& instr : func.getInstructions()) {
          if (instr.getOpCode() != pex::PexOpCode::CallStatic) {
            continue;
          }
          const auto& args = instr.getArgs();
          if (args.size() < 2) {
            continue;
          }
          if (pexIdentifierName(file, args[0]) == objectType &&
              pexIdentifierName(file, args[1]) == functionName) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

class ImportCallTestsFixture {
 public:
  ImportCallTestsFixture() {
    errorHandler = makeShared<CompilerErrorHandler>();
    importLibrary = makeShared<ImportLibrary>(Vec<fs::path>{});
    builtinFunctions = makeShared<BuiltinFunctions>();
    resolver = makeShared<Resolver>(
        VellumObject(VellumType::identifier(kScriptName)), errorHandler,
        importLibrary, builtinFunctions);
    analyzer =
        makeShared<SemanticAnalyzer>(errorHandler, resolver, kScriptName);
  }

  void addImportedScript(std::string_view name,
                         std::string_view staticFunctionName = kImportedFunc) {
    VellumObject object(VellumType::identifier(name));
    object.addFunction(makeStaticFunction(staticFunctionName));
    VellumIdentifier moduleName(name);
    auto module = makeShared<ImportModule>(moduleName, ImportModuleType::Vellum,
                                           fs::path(""));
    auto moduleResolver = makeShared<Resolver>(object, errorHandler,
                                               importLibrary, builtinFunctions);
    module->setResolver(moduleResolver);
    importLibrary->addTestModule(module);
  }

  SemanticAnalyzeResult analyze(Vec<Unique<ast::Declaration>> ast) {
    return analyzer->analyze(std::move(ast));
  }

  pex::PexFile compile(const SemanticAnalyzeResult& result) {
    return Compiler(errorHandler).compile(ScriptMetadata(),
                                          result.declarations);
  }

  Vec<Unique<ast::Declaration>> makeImportedCallScript(
      Unique<ast::CallExpression> callExpr, Vec<std::string_view> imports) {
    Vec<Unique<ast::Declaration>> ast;
    for (const auto& importName : imports) {
      ast.emplace_back(makeUnique<ast::ImportDeclaration>(
          importName, idToken(1, importName)));
    }

    ast::FunctionBody body;
    body.push_back(
        makeUnique<ast::ExpressionStatement>(std::move(callExpr)));

    Vec<Unique<ast::Declaration>> members;
    members.push_back(makeUnique<ast::FunctionDeclaration>(
        "caller", Vec<ast::FunctionParameter>{}, VellumType::none(),
        makeUnique<ast::BlockStatement>(std::move(body))));

    ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
        VellumType::identifier(kScriptName), idToken(1, kScriptName),
        VellumType::none(), std::nullopt, std::move(members)));
    return ast;
  }

  Vec<Unique<ast::Declaration>> makeLocalStaticCallScript(
      std::string_view localStaticName) {
    Vec<Unique<ast::Declaration>> members;
    members.push_back(makeUnique<ast::FunctionDeclaration>(
        localStaticName, Vec<ast::FunctionParameter>{},
        VellumType::unresolved("Int"),
        makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{}),
        staticParsedModifiers, idToken(1, localStaticName)));

    ast::FunctionBody body;
    body.push_back(makeUnique<ast::ExpressionStatement>(
        makeBareCall(localStaticName, idToken(2, localStaticName))));

    members.push_back(makeUnique<ast::FunctionDeclaration>(
        "caller", Vec<ast::FunctionParameter>{}, VellumType::none(),
        makeUnique<ast::BlockStatement>(std::move(body))));

    Vec<Unique<ast::Declaration>> ast;
    ast.emplace_back(makeUnique<ast::ScriptDeclaration>(
        VellumType::identifier(kScriptName), idToken(1, kScriptName),
        VellumType::none(), std::nullopt, std::move(members)));
    return ast;
  }

 protected:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<ImportLibrary> importLibrary;
  Shared<BuiltinFunctions> builtinFunctions;
  Shared<Resolver> resolver;
  Shared<SemanticAnalyzer> analyzer;
};

}  // namespace

TEST_CASE_METHOD(ImportCallTestsFixture,
                 "ImportCall_BareCall_ResolvesAsStaticOnImportedType") {
  addImportedScript("ImportA");

  auto result = analyze(makeImportedCallScript(makeBareCall(kImportedFunc),
                                               {"ImportA"}));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& call = firstCallInScript(result);
  REQUIRE(call.getFunctionCall().has_value());
  CHECK(call.getFunctionCall()->isStatic());
  CHECK(call.getFunctionCall()->getObjectType() ==
        VellumType::identifier("ImportA"));
  CHECK(call.getFunctionCall()->getFunction().toString() == kImportedFunc);
}

TEST_CASE_METHOD(ImportCallTestsFixture,
                 "ImportCall_QualifiedCall_ResolvesAsStaticOnImportedType") {
  addImportedScript("ImportA");

  auto result = analyze(makeImportedCallScript(
      makeQualifiedCall("ImportA", kImportedFunc), {"ImportA"}));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& call = firstCallInScript(result);
  REQUIRE(call.getFunctionCall().has_value());
  CHECK(call.getFunctionCall()->isStatic());
  CHECK(call.getFunctionCall()->getObjectType() ==
        VellumType::identifier("ImportA"));
  CHECK(call.getFunctionCall()->getFunction().toString() == kImportedFunc);
}

TEST_CASE_METHOD(ImportCallTestsFixture,
                 "ImportCall_BareCall_CompilesToCallStatic") {
  addImportedScript("ImportA");

  auto result = analyze(makeImportedCallScript(makeBareCall(kImportedFunc),
                                               {"ImportA"}));
  REQUIRE_FALSE(errorHandler->hadError());

  const auto file = compile(result);
  REQUIRE_FALSE(errorHandler->hadError());
  CHECK(hasCallStatic(file, "ImportA", kImportedFunc));
}

TEST_CASE_METHOD(ImportCallTestsFixture,
                 "ImportCall_QualifiedCall_CompilesToCallStatic") {
  addImportedScript("ImportA");

  auto result = analyze(makeImportedCallScript(
      makeQualifiedCall("ImportA", kImportedFunc), {"ImportA"}));
  REQUIRE_FALSE(errorHandler->hadError());

  const auto file = compile(result);
  REQUIRE_FALSE(errorHandler->hadError());
  CHECK(hasCallStatic(file, "ImportA", kImportedFunc));
}

TEST_CASE_METHOD(
    ImportCallTestsFixture,
    "ImportCall_LocalStaticBareCall_ResolvesAsStaticOnCurrentScript") {
  constexpr std::string_view kLocalStatic = "localHelper";

  auto result = analyze(makeLocalStaticCallScript(kLocalStatic));

  REQUIRE_FALSE(errorHandler->hadError());
  const auto& call = firstCallInScript(result);
  REQUIRE(call.getFunctionCall().has_value());
  CHECK(call.getFunctionCall()->isStatic());
  CHECK(call.getFunctionCall()->getObjectType() ==
        VellumType::identifier(kScriptName));
  CHECK(call.getFunctionCall()->getFunction().toString() == kLocalStatic);
}

TEST_CASE_METHOD(ImportCallTestsFixture,
                 "ImportCall_LocalStaticBareCall_CompilesToCallStatic") {
  constexpr std::string_view kLocalStatic = "localHelper";

  auto result = analyze(makeLocalStaticCallScript(kLocalStatic));
  REQUIRE_FALSE(errorHandler->hadError());

  const auto file = compile(result);
  REQUIRE_FALSE(errorHandler->hadError());
  CHECK(hasCallStatic(file, kScriptName, kLocalStatic));
}

TEST_CASE_METHOD(ImportCallTestsFixture,
                 "ImportCall_AmbiguousImportsBareCall_Errors") {
  addImportedScript("ImportA", "sharedFunc");
  addImportedScript("ImportB", "sharedFunc");

  analyze(makeImportedCallScript(makeBareCall("sharedFunc"),
                                 {"ImportA", "ImportB"}));

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::AmbiguousImportCall));
}
