#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "mock/mock_lexer.h"
#include "parser/parser.h"
#include "utils.h"

using namespace vellum;
using common::makeShared;
using common::makeUnique;
using common::Shared;
using common::Unique;
using common::Vec;

TEST_CASE("ParserScriptDeclarationTest") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "ParentScript"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::ScriptDeclaration expected(
      VellumType::identifier("TestScript"), tokens[1],
      VellumType::identifier("ParentScript"), tokens[3], {});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserSuperCallInFunction") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::SUPER, 1, "super"),
                    makeToken(TokenType::DOT, 1, "."),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* call =
      dynamic_cast<const ast::CallExpression*>(exprStmt->getExpression().get());
  REQUIRE(call != nullptr);
  REQUIRE(call->getCallee()->isPropertyGetExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getObject()->isSuperExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getProperty().toString() == "foo");
}

TEST_CASE("ParserSelfCallInFunction") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::SELF, 1, "self"),
                    makeToken(TokenType::DOT, 1, "."),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* call =
      dynamic_cast<const ast::CallExpression*>(exprStmt->getExpression().get());
  REQUIRE(call != nullptr);
  REQUIRE(call->getCallee()->isPropertyGetExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getObject()->isSelfExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getProperty().toString() == "foo");
}

TEST_CASE("ParserGlobalVarDeclarationTest") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "number"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserGlobalVarDeclaration_NegativeInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "num"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::MINUS, 1, "-"),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* varDecl = dynamic_cast<const ast::GlobalVariableDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(varDecl != nullptr);
  REQUIRE(varDecl->initializer() != nullptr);
  REQUIRE(varDecl->initializer()->isLiteralExpression());
  REQUIRE(varDecl->initializer()->asLiteral().getLiteral().asInt() == -42);
}

TEST_CASE("ParserFunctionDeclaration_NoParams_NoReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{})));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_ArrayParameterType") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "squared"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::IDENTIFIER, 1, "nums"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 1);
  const auto& p = funcDecl->getParameters()[0];
  REQUIRE(p.type.isArray());
  REQUIRE(*p.type.asArraySubtype() == VellumType::unresolved("Int"));
}

TEST_CASE("ParserBreakInsideWhile") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::WHILE, 1, "while"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::BREAK, 1, "break"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* whileStmt = dynamic_cast<const ast::WhileStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(whileStmt != nullptr);
  const auto* whileBody =
      dynamic_cast<const ast::BlockStatement*>(whileStmt->getBody().get());
  REQUIRE(whileBody != nullptr);
  REQUIRE(whileBody->getStatements().size() == 1);
  REQUIRE(dynamic_cast<const ast::BreakStatement*>(
              whileBody->getStatements()[0].get()) != nullptr);
}

TEST_CASE("ParserContinueInsideWhile") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::WHILE, 1, "while"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::CONTINUE, 1, "continue"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* whileStmt = dynamic_cast<const ast::WhileStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(whileStmt != nullptr);
  const auto* whileBody =
      dynamic_cast<const ast::BlockStatement*>(whileStmt->getBody().get());
  REQUIRE(whileBody != nullptr);
  REQUIRE(whileBody->getStatements().size() == 1);
  REQUIRE(dynamic_cast<const ast::ContinueStatement*>(
              whileBody->getStatements()[0].get()) != nullptr);
}

TEST_CASE("ParserIf_OnlyThen") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IF, 1, "if"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fa"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* ifStmt = dynamic_cast<const ast::IfStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(ifStmt != nullptr);
  REQUIRE_FALSE(ifStmt->getElseBlock());
  const auto* thenBlock =
      dynamic_cast<const ast::BlockStatement*>(ifStmt->getThenBlock().get());
  REQUIRE(thenBlock != nullptr);
  REQUIRE(thenBlock->getStatements().size() == 1);
}

TEST_CASE("ParserIfElse") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IF, 1, "if"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fa"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::ELSE, 1, "else"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fb"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* ifStmt = dynamic_cast<const ast::IfStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(ifStmt != nullptr);
  REQUIRE(ifStmt->getElseBlock());
  const auto* elseBlock =
      dynamic_cast<const ast::BlockStatement*>(ifStmt->getElseBlock().get());
  REQUIRE(elseBlock != nullptr);
  REQUIRE(elseBlock->getStatements().size() == 1);
}

TEST_CASE("ParserIfElseIfElse") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IF, 1, "if"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fa"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::ELSE, 1, "else"),
                    makeToken(TokenType::IF, 1, "if"),
                    makeToken(TokenType::FALSE, 1, "false",
                              VellumLiteral(false)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fb"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::ELSE, 1, "else"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fc"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* outer = dynamic_cast<const ast::IfStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(outer != nullptr);
  REQUIRE(outer->getElseBlock());
  const auto* inner = dynamic_cast<const ast::IfStatement*>(
      outer->getElseBlock().get());
  REQUIRE(inner != nullptr);
  REQUIRE(inner->getElseBlock());
  const auto* finalElse =
      dynamic_cast<const ast::BlockStatement*>(inner->getElseBlock().get());
  REQUIRE(finalElse != nullptr);
  REQUIRE(finalElse->getStatements().size() == 1);
}

TEST_CASE("ParserMatch_LiteralAndIdentifierPatterns") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::STRING, 1, "\"a\"",
                              VellumLiteral(std::string_view("a"))),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fa"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::MINUS, 1, "-"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fc"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::IDENTIFIER, 1, "MyConst"),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "fb"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* matchStmt = dynamic_cast<const ast::MatchStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(matchStmt != nullptr);
  REQUIRE(matchStmt->getArms().size() == 3);
  REQUIRE(matchStmt->getArms()[0].pattern->isLiteralExpression());
  REQUIRE(matchStmt->getArms()[1].pattern->isLiteralExpression());
  CHECK(matchStmt->getArms()[1].pattern->asLiteral().getLiteral().asInt() ==
        -1);
  REQUIRE(matchStmt->getArms()[2].pattern->isIdentifierExpression());
}

TEST_CASE("ParserMatch_InvalidPattern") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "a"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::IDENTIFIER, 1, "fa"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ExpectMatchPattern));
}

TEST_CASE("ParserMatch_WithElse") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::ELSE, 1, "else"),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* matchStmt = dynamic_cast<const ast::MatchStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(matchStmt != nullptr);
  REQUIRE(matchStmt->getArms().size() == 1);
  REQUIRE(matchStmt->getElseBody() != nullptr);
  REQUIRE(dynamic_cast<const ast::ReturnStatement*>(
              matchStmt->getArms()[0].body.get()) != nullptr);
  REQUIRE(dynamic_cast<const ast::ReturnStatement*>(
              matchStmt->getElseBody().get()) != nullptr);
}

TEST_CASE("ParserMatch_UnbracedJumpStatements") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::BREAK, 1, "break"),
                    makeToken(TokenType::INT, 1, "3", VellumLiteral(3)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::CONTINUE, 1, "continue"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* matchStmt = dynamic_cast<const ast::MatchStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(matchStmt != nullptr);
  REQUIRE(matchStmt->getArms().size() == 3);
  REQUIRE(dynamic_cast<const ast::ReturnStatement*>(
              matchStmt->getArms()[0].body.get()) != nullptr);
  REQUIRE(dynamic_cast<const ast::BreakStatement*>(
              matchStmt->getArms()[1].body.get()) != nullptr);
  REQUIRE(dynamic_cast<const ast::ContinueStatement*>(
              matchStmt->getArms()[2].body.get()) != nullptr);
}

TEST_CASE("ParserMatch_ReturnWithValueAndNextArm") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* matchStmt = dynamic_cast<const ast::MatchStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(matchStmt != nullptr);
  REQUIRE(matchStmt->getArms().size() == 2);
  const auto* returnWithValue = dynamic_cast<const ast::ReturnStatement*>(
      matchStmt->getArms()[0].body.get());
  REQUIRE(returnWithValue != nullptr);
  REQUIRE(returnWithValue->getExpression() != nullptr);
  CHECK(
      returnWithValue->getExpression()->asLiteral().getLiteral().asInt() == 42);
  const auto* bareReturn = dynamic_cast<const ast::ReturnStatement*>(
      matchStmt->getArms()[1].body.get());
  REQUIRE(bareReturn != nullptr);
  REQUIRE(bareReturn->getExpression() == nullptr);
}

TEST_CASE("ParserMatch_ExpressionArmBody") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::IDENTIFIER, 1, "fa"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* matchStmt = dynamic_cast<const ast::MatchStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(matchStmt != nullptr);
  REQUIRE(matchStmt->getArms().size() == 1);
  REQUIRE(dynamic_cast<const ast::ExpressionStatement*>(
              matchStmt->getArms()[0].body.get()) != nullptr);
}

TEST_CASE("ParserMatch_MissingFatArrow_ExpectFatArrow") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ExpectFatArrow));
}

TEST_CASE("ParserMatch_BoolAndFloatPatterns") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::MATCH, 1, "match"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::FLOAT, 1, "1.5", VellumLiteral(1.5f)),
                    makeToken(TokenType::FAT_ARROW, 1, "=>"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* matchStmt = dynamic_cast<const ast::MatchStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(matchStmt != nullptr);
  REQUIRE(matchStmt->getArms().size() == 2);
  CHECK(matchStmt->getArms()[0].pattern->asLiteral().getLiteral().asBool() ==
        true);
  CHECK(matchStmt->getArms()[1].pattern->asLiteral().getLiteral().asFloat() ==
        1.5f);
}

TEST_CASE("ParserReturn_BareReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* returnStmt =
      dynamic_cast<const ast::ReturnStatement*>(funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(returnStmt != nullptr);
  REQUIRE(returnStmt->getExpression() == nullptr);
}

TEST_CASE("ParserReturn_WithIntLiteral") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::ARROW, 1, "->"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* returnStmt =
      dynamic_cast<const ast::ReturnStatement*>(funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(returnStmt != nullptr);
  REQUIRE(returnStmt->getExpression() != nullptr);
  REQUIRE(returnStmt->getExpression()->isLiteralExpression());
  REQUIRE(returnStmt->getExpression()->asLiteral().getLiteral().asInt() == 42);
}

TEST_CASE("ParserReturn_WithNone") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::NONE, 1, "none", VellumLiteral()),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* returnStmt =
      dynamic_cast<const ast::ReturnStatement*>(funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(returnStmt != nullptr);
  REQUIRE(returnStmt->getExpression() != nullptr);
  REQUIRE(returnStmt->getExpression()->isLiteralExpression());
  REQUIRE(returnStmt->getExpression()->asLiteral().getLiteral().getType() ==
          VellumLiteralType::None);
}

TEST_CASE("ParserForInStatement_Basic") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FOR, 1, "for"),
                    makeToken(TokenType::IDENTIFIER, 1, "i"),
                    makeToken(TokenType::IN, 1, "in"),
                    makeToken(TokenType::IDENTIFIER, 1, "nums"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::BREAK, 1, "break"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* forStmt =
      dynamic_cast<const ast::ForStatement*>(funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(forStmt != nullptr);
  REQUIRE(forStmt->getVariableName()->isIdentifierExpression());
  REQUIRE(forStmt->getVariableName()->asIdentifier().getIdentifier().toString() ==
          "i");
  REQUIRE(forStmt->getArray()->isIdentifierExpression());
  REQUIRE(forStmt->getArray()->asIdentifier().getIdentifier().toString() == "nums");
  const auto* forBodyBlock =
      dynamic_cast<const ast::BlockStatement*>(forStmt->getBody().get());
  REQUIRE(forBodyBlock != nullptr);
  REQUIRE(forBodyBlock->getStatements().size() == 1);
  REQUIRE(dynamic_cast<const ast::BreakStatement*>(
              forBodyBlock->getStatements()[0].get()) != nullptr);
}

TEST_CASE("ParserForInStatement_LoopVariableMustBeIdentifier") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FOR, 1, "for"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::IN, 1, "in"),
                    makeToken(TokenType::IDENTIFIER, 1, "nums"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("ParserForInStatement_ExpectInKeyword") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FOR, 1, "for"),
                    makeToken(TokenType::IDENTIFIER, 1, "i"),
                    makeToken(TokenType::IDENTIFIER, 1, "nums"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("ParserForInStatement_ExpectLeftBraceAfterCollection") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FOR, 1, "for"),
                    makeToken(TokenType::IDENTIFIER, 1, "i"),
                    makeToken(TokenType::IN, 1, "in"),
                    makeToken(TokenType::IDENTIFIER, 1, "nums"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("ParserPropertyDeclaration_NoInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->getName() == "x");
  REQUIRE(propDecl->getTypeName().toString() == "Int");
  REQUIRE_FALSE(propDecl->getDefaultValue().has_value());
  REQUIRE(propDecl->getGetAccessor().has_value());
  REQUIRE(propDecl->getSetAccessor().has_value());
  REQUIRE(propDecl->getGetAccessor().value()->getStatements().empty());
  REQUIRE(propDecl->getSetAccessor().value()->getStatements().empty());
}

TEST_CASE("ParserPropertyDeclaration_WithIntInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->getDefaultValue().has_value());
  REQUIRE(propDecl->getDefaultValue()->getType() == VellumLiteralType::Int);
  REQUIRE(propDecl->getDefaultValue()->asInt() == 42);
}

TEST_CASE("ParserPropertyDeclaration_WithNoneInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "obj"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "SomeScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::NONE, 1, "None", VellumLiteral()),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->getDefaultValue().has_value());
  REQUIRE(propDecl->getDefaultValue()->getType() == VellumLiteralType::None);
}

TEST_CASE("ParserPropertyDeclaration_ReadonlyAuto") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->isReadonly());
  REQUIRE(propDecl->getGetAccessor().has_value());
  REQUIRE_FALSE(propDecl->getSetAccessor().has_value());
}

TEST_CASE("ParserPropertyDeclaration_InitializerMustBeLiteral") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("ParserFunctionDeclaration_Params_NoReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "y"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Float"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<ast::FunctionParameter> params = {{"x", VellumType::unresolved("Int")},
                                        {"y", VellumType::unresolved("Float")}};

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "bar", params, VellumType::none(), makeUnique<ast::BlockStatement>(ast::FunctionBody{})));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_ParamWithDefaultValue") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "5", VellumLiteral(5)),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 1);
  REQUIRE(funcDecl->getParameters()[0].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[0].defaultValue->getType() ==
          VellumLiteralType::Int);
  REQUIRE(funcDecl->getParameters()[0].defaultValue->asInt() == 5);
}

TEST_CASE("ParserFunctionDeclaration_NegativeDefaultValue") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::MINUS, 1, "-"),
                    makeToken(TokenType::INT, 1, "5", VellumLiteral(5)),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 1);
  REQUIRE(funcDecl->getParameters()[0].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[0].defaultValue->getType() ==
          VellumLiteralType::Int);
  REQUIRE(funcDecl->getParameters()[0].defaultValue->asInt() == -5);
}

TEST_CASE("ParserFunctionDeclaration_MultipleParamsWithTrailingDefault") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "a"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "b"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "String"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::STRING, 1, "hi",
                             VellumLiteral(std::string_view("hi"))),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 2);
  REQUIRE_FALSE(funcDecl->getParameters()[0].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[1].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[1].defaultValue->getType() ==
          VellumLiteralType::String);
  REQUIRE(funcDecl->getParameters()[1].defaultValue->asString() == "hi");
}

TEST_CASE("ParserFunctionDeclaration_NoParams_WithReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "baz"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::ARROW, 1, "->"),
                    makeToken(TokenType::IDENTIFIER, 1, "String"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "baz", Vec<ast::FunctionParameter>{}, VellumType::unresolved("String"),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{})));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_Params_WithReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "qux"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "a"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Bool"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::ARROW, 1, "->"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<ast::FunctionParameter> params = {{"a", VellumType::unresolved("Bool")}};

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "qux", params, VellumType::unresolved("Int"),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{})));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_MultipleParams_MixedTypes") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "mix"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "flag"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Bool"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "name"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "String"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"flag", VellumType::unresolved("Bool")},
      {"name", VellumType::unresolved("String")}};

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "mix", params, VellumType::none(), makeUnique<ast::BlockStatement>(ast::FunctionBody{})));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserArrayIndexExpression") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "arr"),
                    makeToken(TokenType::LEFT_BRACK, 1, "["),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACK, 1, "]"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* indexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(indexExpr != nullptr);
}

TEST_CASE("ParserArrayIndexAssignment") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "arr"),
                    makeToken(TokenType::LEFT_BRACK, 1, "["),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACK, 1, "]"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* setExpr = dynamic_cast<const ast::ArrayIndexSetExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(setExpr != nullptr);
}

TEST_CASE("ParserComposedAssign_Identifier") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::PLUS_EQUAL, 1, "+="),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* assignExpr = dynamic_cast<const ast::AssignExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(assignExpr != nullptr);
  REQUIRE(assignExpr->getOperator() == ast::AssignOperator::Add);
}

TEST_CASE("ParserComposedAssign_Property") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "obj"),
                    makeToken(TokenType::DOT, 1, "."),
                    makeToken(TokenType::IDENTIFIER, 1, "prop"),
                    makeToken(TokenType::MINUS_EQUAL, 1, "-="),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* setExpr = dynamic_cast<const ast::PropertySetExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(setExpr != nullptr);
  REQUIRE(setExpr->getOperator() == ast::AssignOperator::Subtract);
}

TEST_CASE("ParserComposedAssign_ArrayIndex") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "arr"),
                    makeToken(TokenType::LEFT_BRACK, 1, "["),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACK, 1, "]"),
                    makeToken(TokenType::STAR_EQUAL, 1, "*="),
                    makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* setExpr = dynamic_cast<const ast::ArrayIndexSetExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(setExpr != nullptr);
  REQUIRE(setExpr->getOperator() == ast::AssignOperator::Multiply);
}

TEST_CASE("ParserCall_NoArgs") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  auto expected_call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto expected_body = Vec<Unique<ast::Statement>>{};
  expected_body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(expected_call_expr)));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(expected_body))));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserCall_WithArgs") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar")));
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(42)));

  auto expected_call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::move(args));

  auto expected_body = Vec<Unique<ast::Statement>>{};
  expected_body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(expected_call_expr)));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      makeUnique<ast::BlockStatement>(std::move(expected_body))));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserArrayIndex_LiteralIndex") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "arr"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);

  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* indexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(indexExpr != nullptr);
  REQUIRE(indexExpr->getArray()->isIdentifierExpression());
  REQUIRE(indexExpr->getArray()->asIdentifier().getIdentifier().toString() ==
          "arr");
  REQUIRE(indexExpr->getIndex()->isLiteralExpression());
  REQUIRE(indexExpr->getIndex()->asLiteral().getLiteral().asInt() == 0);
}

TEST_CASE("ParserArrayIndex_Nested") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "arr"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);

  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* outerIndexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(outerIndexExpr != nullptr);

  const auto* innerIndexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      outerIndexExpr->getArray().get());
  REQUIRE(innerIndexExpr != nullptr);

  REQUIRE(innerIndexExpr->getArray()->isIdentifierExpression());
  REQUIRE(
      innerIndexExpr->getArray()->asIdentifier().getIdentifier().toString() ==
      "arr");

  REQUIRE(innerIndexExpr->getIndex()->isLiteralExpression());
  REQUIRE(innerIndexExpr->getIndex()->asLiteral().getLiteral().asInt() == 0);

  REQUIRE(outerIndexExpr->getIndex()->isLiteralExpression());
  REQUIRE(outerIndexExpr->getIndex()->asLiteral().getLiteral().asInt() == 1);
}

TEST_CASE("ParserArrayIndex_Precedence_IndexBeforePropertyAndCall") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "arr"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::DOT, 1, "."),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);

  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* callExpr =
      dynamic_cast<const ast::CallExpression*>(exprStmt->getExpression().get());
  REQUIRE(callExpr != nullptr);

  const auto* propertyExpr = dynamic_cast<const ast::PropertyGetExpression*>(
      callExpr->getCallee().get());
  REQUIRE(propertyExpr != nullptr);

  REQUIRE(propertyExpr->getProperty().toString() == "foo");

  const auto* indexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      propertyExpr->getObject().get());
  REQUIRE(indexExpr != nullptr);

  REQUIRE(indexExpr->getArray()->isIdentifierExpression());
  REQUIRE(indexExpr->getArray()->asIdentifier().getIdentifier().toString() ==
          "arr");

  REQUIRE(indexExpr->getIndex()->isLiteralExpression());
  REQUIRE(indexExpr->getIndex()->asLiteral().getLiteral().asInt() == 0);
}

TEST_CASE("ParserReturn_Ternary_Basic") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::ARROW, 1, "->"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::RETURN, 1, "return"),
      makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
      makeToken(TokenType::QUES, 1, "?"),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  const auto* returnStmt =
      dynamic_cast<const ast::ReturnStatement*>(funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(returnStmt != nullptr);
  const auto* ternary = dynamic_cast<const ast::TernaryExpression*>(
      returnStmt->getExpression().get());
  REQUIRE(ternary != nullptr);
  REQUIRE(ternary->getCondition()->isLiteralExpression());
  REQUIRE(ternary->getCondition()->asLiteral().getLiteral().asBool());
  REQUIRE(ternary->getLeft()->asLiteral().getLiteral().asInt() == 1);
  REQUIRE(ternary->getRight()->asLiteral().getLiteral().asInt() == 2);
}

TEST_CASE("ParserReturn_Ternary_OrPrecedence_ConditionIsOrExpression") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::ARROW, 1, "->"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::RETURN, 1, "return"),
      makeToken(TokenType::IDENTIFIER, 1, "a"),
      makeToken(TokenType::OR, 1, "||"),
      makeToken(TokenType::IDENTIFIER, 1, "b"),
      makeToken(TokenType::QUES, 1, "?"),
      makeToken(TokenType::IDENTIFIER, 1, "c"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "d"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  const auto* returnStmt =
      dynamic_cast<const ast::ReturnStatement*>(funcDecl->getBody()->getStatements()[0].get());
  const auto* ternary = dynamic_cast<const ast::TernaryExpression*>(
      returnStmt->getExpression().get());
  REQUIRE(ternary != nullptr);
  const auto* condOr = dynamic_cast<const ast::BinaryExpression*>(
      ternary->getCondition().get());
  REQUIRE(condOr != nullptr);
  REQUIRE(condOr->getOperator() == ast::BinaryExpression::Operator::Or);
  REQUIRE(condOr->getLeft()->asIdentifier().getIdentifier().toString() == "a");
  REQUIRE(condOr->getRight()->asIdentifier().getIdentifier().toString() == "b");
  REQUIRE(ternary->getLeft()->asIdentifier().getIdentifier().toString() == "c");
  REQUIRE(ternary->getRight()->asIdentifier().getIdentifier().toString() == "d");
}

TEST_CASE("ParserReturn_Ternary_RightAssociative_Nested") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::ARROW, 1, "->"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::RETURN, 1, "return"),
      makeToken(TokenType::IDENTIFIER, 1, "a"),
      makeToken(TokenType::QUES, 1, "?"),
      makeToken(TokenType::IDENTIFIER, 1, "b"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "c"),
      makeToken(TokenType::QUES, 1, "?"),
      makeToken(TokenType::IDENTIFIER, 1, "d"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "e"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  const auto* returnStmt =
      dynamic_cast<const ast::ReturnStatement*>(funcDecl->getBody()->getStatements()[0].get());
  const auto* outer = dynamic_cast<const ast::TernaryExpression*>(
      returnStmt->getExpression().get());
  REQUIRE(outer != nullptr);
  REQUIRE(outer->getCondition()->asIdentifier().getIdentifier().toString() ==
          "a");
  REQUIRE(outer->getLeft()->asIdentifier().getIdentifier().toString() == "b");
  const auto* inner = dynamic_cast<const ast::TernaryExpression*>(
      outer->getRight().get());
  REQUIRE(inner != nullptr);
  REQUIRE(inner->getCondition()->asIdentifier().getIdentifier().toString() ==
          "c");
  REQUIRE(inner->getLeft()->asIdentifier().getIdentifier().toString() == "d");
  REQUIRE(inner->getRight()->asIdentifier().getIdentifier().toString() == "e");
}

TEST_CASE("ParserReturn_Ternary_MissingColon_ExpectColon") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::ARROW, 1, "->"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::RETURN, 1, "return"),
      makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
      makeToken(TokenType::QUES, 1, "?"),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  (void)Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hasError(CompilerErrorKind::ExpectColon));
}

TEST_CASE("ParserCall_NotChainedAfterCall_OnNextLine") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::VAR, 1, "var"),
      makeToken(TokenType::IDENTIFIER, 1, "itemType"),
      makeToken(TokenType::EQUAL, 1, "="),
      makeToken(TokenType::IDENTIFIER, 1, "player"),
      makeToken(TokenType::DOT, 1, "."),
      makeToken(TokenType::IDENTIFIER, 1, "GetEquippedItemType"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_PAREN, 2, "("),
      makeToken(TokenType::IDENTIFIER, 2, "source"),
      makeToken(TokenType::AS, 2, "as"),
      makeToken(TokenType::IDENTIFIER, 2, "Weapon"),
      makeToken(TokenType::RIGHT_PAREN, 2, ")"),
      makeToken(TokenType::RIGHT_BRACE, 2, "}"),
      makeToken(TokenType::RIGHT_BRACE, 2, "}"),
      makeToken(TokenType::END_OF_FILE, 2, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 2);

  const auto* varStmt = dynamic_cast<const ast::LocalVariableStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(varStmt != nullptr);
  REQUIRE(varStmt->getName().toString() == "itemType");
  REQUIRE(varStmt->getInitializer() != nullptr);

  const auto* initCall = dynamic_cast<const ast::CallExpression*>(
      varStmt->getInitializer().get());
  REQUIRE(initCall != nullptr);
  REQUIRE(initCall->getArguments().size() == 1);
  REQUIRE(initCall->getArguments()[0]->isLiteralExpression());
  REQUIRE(initCall->getArguments()[0]->asLiteral().getLiteral().asInt() == 1);

  const auto* initCallee = dynamic_cast<const ast::PropertyGetExpression*>(
      initCall->getCallee().get());
  REQUIRE(initCallee != nullptr);
  REQUIRE(initCallee->getObject()->isIdentifierExpression());
  REQUIRE(initCallee->getObject()->asIdentifier().getIdentifier().toString() ==
          "player");
  REQUIRE(initCallee->getProperty().toString() == "GetEquippedItemType");

  const auto* castStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[1].get());
  REQUIRE(castStmt != nullptr);

  const auto* groupingExpr = dynamic_cast<const ast::GroupingExpression*>(
      castStmt->getExpression().get());
  REQUIRE(groupingExpr != nullptr);

  const auto* castExpr = dynamic_cast<const ast::CastExpression*>(
      groupingExpr->getExpression().get());
  REQUIRE(castExpr != nullptr);
  REQUIRE(castExpr->getExpression()->isIdentifierExpression());
  REQUIRE(
      castExpr->getExpression()->asIdentifier().getIdentifier().toString() ==
      "source");
  REQUIRE(castExpr->getTargetExpression()->getIdentifier().toString() ==
          "Weapon");
}

TEST_CASE("ParserCall_NotChainedAfterGroupedCall_OnNextLine") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::VAR, 1, "var"),
      makeToken(TokenType::IDENTIFIER, 1, "itemType"),
      makeToken(TokenType::EQUAL, 1, "="),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::IDENTIFIER, 1, "player"),
      makeToken(TokenType::DOT, 1, "."),
      makeToken(TokenType::IDENTIFIER, 1, "GetEquippedItemType"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_PAREN, 2, "("),
      makeToken(TokenType::IDENTIFIER, 2, "source"),
      makeToken(TokenType::AS, 2, "as"),
      makeToken(TokenType::IDENTIFIER, 2, "Weapon"),
      makeToken(TokenType::RIGHT_PAREN, 2, ")"),
      makeToken(TokenType::RIGHT_BRACE, 2, "}"),
      makeToken(TokenType::RIGHT_BRACE, 2, "}"),
      makeToken(TokenType::END_OF_FILE, 2, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 2);

  const auto* varStmt = dynamic_cast<const ast::LocalVariableStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(varStmt != nullptr);

  const auto* initGrouping = dynamic_cast<const ast::GroupingExpression*>(
      varStmt->getInitializer().get());
  REQUIRE(initGrouping != nullptr);

  const auto* initCall = dynamic_cast<const ast::CallExpression*>(
      initGrouping->getExpression().get());
  REQUIRE(initCall != nullptr);
  REQUIRE(initCall->getCallee()->isPropertyGetExpression());

  const auto* castStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[1].get());
  REQUIRE(castStmt != nullptr);
  REQUIRE(dynamic_cast<const ast::CastExpression*>(
              dynamic_cast<const ast::GroupingExpression*>(
                  castStmt->getExpression().get())
                  ->getExpression()
                  .get()) != nullptr);
}

TEST_CASE("ParserCall_ChainedMethodCall") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "obj"),
      makeToken(TokenType::DOT, 1, "."),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::DOT, 1, "."),
      makeToken(TokenType::IDENTIFIER, 1, "bar"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody()->getStatements().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* outerCall = dynamic_cast<const ast::CallExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(outerCall != nullptr);
  REQUIRE(outerCall->getCallee()->isPropertyGetExpression());
  REQUIRE(outerCall->getCallee()->asPropertyGet().getProperty().toString() ==
          "bar");

  const auto* innerCall = dynamic_cast<const ast::CallExpression*>(
      outerCall->getCallee()->asPropertyGet().getObject().get());
  REQUIRE(innerCall != nullptr);
  REQUIRE(innerCall->getCallee()->isPropertyGetExpression());
  REQUIRE(innerCall->getCallee()->asPropertyGet().getProperty().toString() ==
          "foo");
  REQUIRE(
      innerCall->getCallee()->asPropertyGet().getObject()->isIdentifierExpression());
  REQUIRE(innerCall->getCallee()
              ->asPropertyGet()
              .getObject()
              ->asIdentifier()
              .getIdentifier()
              .toString() == "obj");
}

namespace {
const ast::FunctionDeclaration* getScriptFunction(
    const ParserResult& result, size_t memberIndex = 0) {
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() > memberIndex);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[memberIndex].get());
  REQUIRE(funcDecl != nullptr);
  return funcDecl;
}
}  // namespace

TEST_CASE("ParserFunctionModifiers_StaticFun") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::STATIC, 1, "static"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* funcDecl = getScriptFunction(result);
  CHECK(funcDecl->isStatic());
  CHECK_FALSE(funcDecl->isNative());
  CHECK(modifiersBitmask(funcDecl->getModifiers()) == staticFunctionModifier);
}

TEST_CASE("ParserFunctionModifiers_NativeFun_NoBody") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::NATIVE, 1, "native"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "getPlayer"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::ARROW, 1, "->"),
                    makeToken(TokenType::IDENTIFIER, 1, "Actor"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* funcDecl = getScriptFunction(result);
  CHECK(funcDecl->isNative());
  CHECK_FALSE(funcDecl->isStatic());
  CHECK(modifiersBitmask(funcDecl->getModifiers()) == nativeFunctionModifier);
  CHECK(funcDecl->getBody()->getStatements().empty());
}

TEST_CASE("ParserFunctionModifiers_StaticNativeFun") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::NATIVE, 1, "native"),
                    makeToken(TokenType::STATIC, 1, "static"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* funcDecl = getScriptFunction(result);
  CHECK(funcDecl->isStatic());
  CHECK(funcDecl->isNative());
  CHECK(modifiersBitmask(funcDecl->getModifiers()) ==
        staticNativeFunctionModifier);
}

TEST_CASE("ParserFunctionModifiers_DuplicateStatic") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::STATIC, 1, "static"),
                    makeToken(TokenType::STATIC, 1, "static"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hadError());
  REQUIRE(errorHandler->hasError(CompilerErrorKind::DuplicateModifier));
  const auto* funcDecl = getScriptFunction(result);
  CHECK(funcDecl->isStatic());
  CHECK(modifiersBitmask(funcDecl->getModifiers()) == staticFunctionModifier);
}

TEST_CASE("ParserFunctionModifiers_NativeFunWithBody_ParsesNextMember") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::NATIVE, 1, "native"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 2);

  const auto* nativeFunc = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(nativeFunc != nullptr);
  CHECK(nativeFunc->isNative());
  REQUIRE(nativeFunc->getBody()->getStatements().size() == 1);
  CHECK(dynamic_cast<const ast::ReturnStatement*>(
            nativeFunc->getBody()->getStatements()[0].get()) != nullptr);

  const auto* barFunc = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[1].get());
  REQUIRE(barFunc != nullptr);
  CHECK(barFunc->getName() == "bar");
  CHECK_FALSE(barFunc->isNative());
}

TEST_CASE("ParserFunctionModifiers_NativeFun_EmptyBraces") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::NATIVE, 1, "native"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* funcDecl = getScriptFunction(result);
  CHECK(funcDecl->isNative());
  CHECK(funcDecl->getBody()->getStatements().empty());
}

TEST_CASE("ParserFunctionModifiers_StaticEventParses") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::STATIC, 1, "static"),
                    makeToken(TokenType::EVENT, 1, "event"),
                    makeToken(TokenType::IDENTIFIER, 1, "onActivate"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* funcDecl = getScriptFunction(result);
  CHECK(funcDecl->isStatic());
  CHECK(funcDecl->isEvent());
}

namespace {
const ast::ScriptDeclaration* getScriptDeclaration(const ParserResult& result) {
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  return scriptDecl;
}

const ast::GlobalVariableDeclaration* getScriptVariable(
    const ParserResult& result, size_t memberIndex = 0) {
  const auto* scriptDecl = getScriptDeclaration(result);
  REQUIRE(scriptDecl->getMemberDecls().size() > memberIndex);
  const auto* varDecl = dynamic_cast<const ast::GlobalVariableDeclaration*>(
      scriptDecl->getMemberDecls()[memberIndex].get());
  REQUIRE(varDecl != nullptr);
  return varDecl;
}

const ast::PropertyDeclaration* getScriptProperty(
    const ParserResult& result, size_t memberIndex = 0) {
  const auto* scriptDecl = getScriptDeclaration(result);
  REQUIRE(scriptDecl->getMemberDecls().size() > memberIndex);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[memberIndex].get());
  REQUIRE(propDecl != nullptr);
  return propDecl;
}
}  // namespace

TEST_CASE("ParserModifiers_HiddenScript") {
  Vec<Token> tokens{makeToken(TokenType::HIDDEN, 1, "hidden"),
                    makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl = getScriptDeclaration(result);
  CHECK(scriptDecl->isHidden());
  CHECK_FALSE(scriptDecl->isConditional());
  CHECK(modifiersBitmask(scriptDecl->getModifiers()) ==
        VellumModifiers{VellumModifier::Hidden});
}

TEST_CASE("ParserModifiers_ConditionalVar") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::CONDITIONAL, 1, "conditional"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "stage"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* varDecl = getScriptVariable(result);
  CHECK(modifiersBitmask(varDecl->getModifiers()) ==
        VellumModifiers{VellumModifier::Conditional});
}

TEST_CASE("ParserModifiers_HiddenAndConditionalScript") {
  Vec<Token> tokens{makeToken(TokenType::HIDDEN, 1, "hidden"),
                    makeToken(TokenType::CONDITIONAL, 1, "conditional"),
                    makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl = getScriptDeclaration(result);
  CHECK(scriptDecl->isHidden());
  CHECK(scriptDecl->isConditional());
  CHECK(modifiersBitmask(scriptDecl->getModifiers()) ==
        (VellumModifiers{VellumModifier::Hidden} |
         VellumModifiers{VellumModifier::Conditional}));
}

TEST_CASE("ParserModifiers_HiddenProperty") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::HIDDEN, 1, "hidden"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "count"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RETURN, 1, "return"),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* propDecl = getScriptProperty(result);
  CHECK(propDecl->isHidden());
  CHECK_FALSE(propDecl->isConditional());
}

TEST_CASE("ParserModifiers_ConditionalAutoProperty") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::CONDITIONAL, 1, "conditional"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "flag"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Bool"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* propDecl = getScriptProperty(result);
  CHECK(propDecl->isConditional());
  CHECK(propDecl->isAutoProperty());
}

TEST_CASE("ParserModifiers_DuplicateHidden") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::HIDDEN, 1, "hidden"),
                    makeToken(TokenType::HIDDEN, 1, "hidden"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hasError(CompilerErrorKind::DuplicateModifier));
  const auto* varDecl = getScriptVariable(result);
  CHECK(modifiersBitmask(varDecl->getModifiers()) ==
        VellumModifiers{VellumModifier::Hidden});
}

TEST_CASE("ParserModifiers_HiddenVarParses") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::HIDDEN, 1, "hidden"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* varDecl = getScriptVariable(result);
  CHECK(modifiersBitmask(varDecl->getModifiers()) ==
        VellumModifiers{VellumModifier::Hidden});
}

namespace {

const ast::Expression* firstFunctionExpression(const ParserResult& result) {
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()->getStatements()[0].get());
  REQUIRE(exprStmt != nullptr);
  return exprStmt->getExpression().get();
}

Vec<Token> wrapInTestFunction(Vec<Token> exprTokens) {
  Vec<Token> tokens;
  tokens.push_back(makeToken(TokenType::SCRIPT, 1, "script"));
  tokens.push_back(makeToken(TokenType::IDENTIFIER, 1, "TestScript"));
  tokens.push_back(makeToken(TokenType::LEFT_BRACE, 1, "{"));
  tokens.push_back(makeToken(TokenType::FUN, 1, "fun"));
  tokens.push_back(makeToken(TokenType::IDENTIFIER, 1, "test"));
  tokens.push_back(makeToken(TokenType::LEFT_PAREN, 1, "("));
  tokens.push_back(makeToken(TokenType::RIGHT_PAREN, 1, ")"));
  tokens.push_back(makeToken(TokenType::LEFT_BRACE, 1, "{"));
  for (auto& token : exprTokens) {
    tokens.push_back(std::move(token));
  }
  tokens.push_back(makeToken(TokenType::RIGHT_BRACE, 1, "}"));
  tokens.push_back(makeToken(TokenType::RIGHT_BRACE, 1, "}"));
  tokens.push_back(makeToken(TokenType::END_OF_FILE, 1, ""));
  return tokens;
}

}  // namespace

TEST_CASE("ParserArrayLiteral_IntElements") {
  Vec<Token> exprTokens{
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::COMMA, 1, ","),
      makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
      makeToken(TokenType::COMMA, 1, ","),
      makeToken(TokenType::INT, 1, "3", VellumLiteral(3)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result = Parser(makeUnique<LexerMock>(wrapInTestFunction(std::move(exprTokens))),
                             errorHandler)
                          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* arrayLiteral =
      dynamic_cast<const ast::NewArrayElementsExpression*>(
          firstFunctionExpression(result));
  REQUIRE(arrayLiteral != nullptr);
  REQUIRE(arrayLiteral->getElementsCount() == 3);
  CHECK(arrayLiteral->getElements()[0]->isLiteralExpression());
  CHECK(arrayLiteral->getElements()[0]->asLiteral().getLiteral().asInt() == 1);
  CHECK(arrayLiteral->getElements()[2]->asLiteral().getLiteral().asInt() == 3);
}

TEST_CASE("ParserArraySizedCreate_IntSemicolon4") {
  Vec<Token> exprTokens{
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::SEMICOLON, 1, ";"),
      makeToken(TokenType::INT, 1, "4", VellumLiteral(4)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result = Parser(makeUnique<LexerMock>(wrapInTestFunction(std::move(exprTokens))),
                             errorHandler)
                          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* sized =
      dynamic_cast<const ast::NewArrayExpression*>(firstFunctionExpression(result));
  REQUIRE(sized != nullptr);
  REQUIRE(sized->getSubtype().has_value());
  CHECK(sized->getSubtype()->asRawType() == "Int");
  CHECK(sized->getLength().asInt() == 4);
  REQUIRE(sized->getSubtypeLocation().has_value());
  CHECK(sized->getSubtypeLocation()->lexeme == "Int");
}

TEST_CASE("ParserArrayEmpty") {
  Vec<Token> exprTokens{
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result = Parser(makeUnique<LexerMock>(wrapInTestFunction(std::move(exprTokens))),
                             errorHandler)
                          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* empty =
      dynamic_cast<const ast::NewArrayExpression*>(firstFunctionExpression(result));
  REQUIRE(empty != nullptr);
  CHECK_FALSE(empty->getSubtype().has_value());
  CHECK(empty->getLength().asInt() == 0);
}

TEST_CASE("ParserArrayLiteral_IdentifierElement") {
  Vec<Token> exprTokens{
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result = Parser(makeUnique<LexerMock>(wrapInTestFunction(std::move(exprTokens))),
                             errorHandler)
                          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* arrayLiteral =
      dynamic_cast<const ast::NewArrayElementsExpression*>(
          firstFunctionExpression(result));
  REQUIRE(arrayLiteral != nullptr);
  REQUIRE(arrayLiteral->getElementsCount() == 1);
  REQUIRE(arrayLiteral->getElements()[0]->isIdentifierExpression());
  CHECK(arrayLiteral->getElements()[0]->asIdentifier().getIdentifier().toString() ==
        "foo");
}

TEST_CASE("ParserArrayLiteral_CommaDisambiguation") {
  Vec<Token> exprTokens{
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::COMMA, 1, ","),
      makeToken(TokenType::INT, 1, "4", VellumLiteral(4)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result = Parser(makeUnique<LexerMock>(wrapInTestFunction(std::move(exprTokens))),
                             errorHandler)
                          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* arrayLiteral =
      dynamic_cast<const ast::NewArrayElementsExpression*>(
          firstFunctionExpression(result));
  REQUIRE(arrayLiteral != nullptr);
  REQUIRE(arrayLiteral->getElementsCount() == 2);
  CHECK(arrayLiteral->getElements()[0]->isIdentifierExpression());
  CHECK(arrayLiteral->getElements()[1]->isLiteralExpression());
}

TEST_CASE("ParserTrailingDotWithoutPropertyName") {
  constexpr const char* kSource = R"(script TestScript {
  fun test() {
    makeArray().
  }
}
)";

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser parser(makeUnique<Lexer>(kSource), errorHandler);
  const auto result = parser.parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* script =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(script != nullptr);
  REQUIRE(script->getMemberDecls().size() == 1);

  const auto* function = dynamic_cast<const ast::FunctionDeclaration*>(
      script->getMemberDecls()[0].get());
  REQUIRE(function != nullptr);
  REQUIRE(function->getBody());
  REQUIRE(function->getBody()->getStatements().size() == 1);

  const auto* callStmt = dynamic_cast<const ast::ExpressionStatement*>(
      function->getBody()->getStatements()[0].get());
  REQUIRE(callStmt != nullptr);
  REQUIRE(dynamic_cast<const ast::CallExpression*>(
              callStmt->getExpression().get()) != nullptr);
}

TEST_CASE("ParserTrailingDotAfterIdentifier") {
  constexpr const char* kSource = R"(script TestScript {
  fun test() {
    nums.
  }
}
)";

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser parser(makeUnique<Lexer>(kSource), errorHandler);
  const auto result = parser.parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* script =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  const auto* function = dynamic_cast<const ast::FunctionDeclaration*>(
      script->getMemberDecls()[0].get());
  const auto* numsStmt = dynamic_cast<const ast::ExpressionStatement*>(
      function->getBody()->getStatements()[0].get());
  REQUIRE(numsStmt != nullptr);
  REQUIRE(numsStmt->getExpression()->isIdentifierExpression());
}