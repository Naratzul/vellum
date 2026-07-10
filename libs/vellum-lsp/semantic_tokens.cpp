#include "semantic_tokens.h"

#include <algorithm>

#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace {

struct SemanticTokenSpan {
  int line;
  int character;
  int length;
  int tokenType;
  int tokenModifiers = 0;
};

bool isKeyword(TokenType type) {
  switch (type) {
    case TokenType::AND:
    case TokenType::AS:
    case TokenType::BREAK:
    case TokenType::CONTINUE:
    case TokenType::ELSE:
    case TokenType::EVENT:
    case TokenType::FALSE:
    case TokenType::FOR:
    case TokenType::FUN:
    case TokenType::GET:
    case TokenType::IF:
    case TokenType::IN:
    case TokenType::IMPORT:
    case TokenType::NONE:
    case TokenType::OR:
    case TokenType::RETURN:
    case TokenType::SET:
    case TokenType::SCRIPT:
    case TokenType::STATE:
    case TokenType::STATIC:
    case TokenType::SUPER:
    case TokenType::SELF:
    case TokenType::TRUE:
    case TokenType::VAR:
    case TokenType::WHILE:
    case TokenType::MATCH:
    case TokenType::SCRIPTNAME:
    case TokenType::EXTENDS:
    case TokenType::HIDDEN:
    case TokenType::CONDITIONAL:
    case TokenType::FUNCTION:
    case TokenType::ENDFUNCTION:
    case TokenType::ENDEVENT:
    case TokenType::PROPERTY:
    case TokenType::ENDPROPERTY:
    case TokenType::AUTO:
    case TokenType::AUTOREADONLY:
    case TokenType::NATIVE:
    case TokenType::GLOBAL:
      return true;
    default:
      return false;
  }
}

bool isOperator(TokenType type) {
  switch (type) {
    case TokenType::LEFT_PAREN:
    case TokenType::RIGHT_PAREN:
    case TokenType::LEFT_BRACE:
    case TokenType::RIGHT_BRACE:
    case TokenType::LEFT_BRACK:
    case TokenType::RIGHT_BRACK:
    case TokenType::COMMA:
    case TokenType::DOT:
    case TokenType::MINUS:
    case TokenType::PLUS:
    case TokenType::COLON:
    case TokenType::SEMICOLON:
    case TokenType::SLASH:
    case TokenType::BACKSLASH:
    case TokenType::STAR:
    case TokenType::PERCENT:
    case TokenType::BANG:
    case TokenType::BANG_EQUAL:
    case TokenType::EQUAL:
    case TokenType::EQUAL_EQUAL:
    case TokenType::PLUS_EQUAL:
    case TokenType::MINUS_EQUAL:
    case TokenType::STAR_EQUAL:
    case TokenType::SLASH_EQUAL:
    case TokenType::PERCENT_EQUAL:
    case TokenType::GREATER:
    case TokenType::GREATER_EQUAL:
    case TokenType::LESS:
    case TokenType::LESS_EQUAL:
    case TokenType::ARROW:
    case TokenType::QUES:
      return true;
    default:
      return false;
  }
}

common::Opt<SemanticTokenLegendType> mapLexerTokenType(TokenType type) {
  if (isKeyword(type)) {
    return SemanticTokenLegendType::Keyword;
  }
  if (isOperator(type)) {
    return SemanticTokenLegendType::Operator;
  }

  switch (type) {
    case TokenType::STRING:
      return SemanticTokenLegendType::String;
    case TokenType::INT:
    case TokenType::FLOAT:
      return SemanticTokenLegendType::Number;
    default:
      return std::nullopt;
  }
}

common::Opt<SemanticTokenSpan> spanFromToken(
    const Token& token, SemanticTokenLegendType type,
    SemanticTokenLegendModifier modifiers = {}) {
  if (token.lexeme.empty()) {
    return std::nullopt;
  }

  return SemanticTokenSpan{
      .line = token.location.start.line,
      .character = token.location.start.position,
      .length = (int)token.lexeme.size(),
      .tokenType = (int)type,
      .tokenModifiers = (int)modifiers,
  };
}

void addSpan(common::Vec<SemanticTokenSpan>& spans, const Token& token,
             SemanticTokenLegendType type,
             SemanticTokenLegendModifier modifiers = {}) {
  if (const auto span = spanFromToken(token, type, modifiers)) {
    spans.push_back(*span);
  }
}

void addSpan(common::Vec<SemanticTokenSpan>& spans,
             const common::Opt<Token>& token, SemanticTokenLegendType type,
             SemanticTokenLegendModifier modifiers = {}) {
  if (token) {
    addSpan(spans, *token, type, modifiers);
  }
}

class SemanticTokensCollector : public ast::DeclarationVisitor,
                                public ast::StatementVisitor,
                                public ast::ExpressionVisitor {
 public:
  explicit SemanticTokensCollector(common::Vec<SemanticTokenSpan>& out)
      : spans(out) {}

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      decl->accept(*this);
    }
  }

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override {
    addSpan(spans, declaration.getImportNameLocation(),
            SemanticTokenLegendType::Type);
  }

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    addSpan(spans, declaration.getScriptNameLocation(),
            SemanticTokenLegendType::Class,
            SemanticTokenLegendModifierBits::Declaration);

    addSpan(spans, declaration.getParentScriptNameLocation(),
            SemanticTokenLegendType::Type);

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }

  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    addSpan(spans, declaration.getStateNameLocation(),
            SemanticTokenLegendType::Class,
            SemanticTokenLegendModifierBits::Declaration);

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
    }
  }

  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override {
    addSpan(spans, declaration.getNameLocation(),
            SemanticTokenLegendType::Variable,
            SemanticTokenLegendModifierBits::Declaration);
    addSpan(spans, declaration.getTypeLocation(),
            SemanticTokenLegendType::Type);

    if (const auto& init = declaration.initializer()) {
      init->accept(*this);
    }
  }

  void visitFunctionDeclaration(
      ast::FunctionDeclaration& declaration) override {
    SemanticTokenLegendModifier modifiers{
        SemanticTokenLegendModifierBits::Declaration};
    if (declaration.isStatic()) {
      modifiers |= SemanticTokenLegendModifierBits::Static;
    }

    addSpan(spans, declaration.getNameLocation(),
            SemanticTokenLegendType::Function, modifiers);
    addSpan(spans, declaration.getReturnTypeLocation(),
            SemanticTokenLegendType::Type);

    for (const auto& param : declaration.getParameters()) {
      addSpan(spans, param.nameLocation, SemanticTokenLegendType::Parameter,
              SemanticTokenLegendModifierBits::Declaration);
      addSpan(spans, param.typeLocation, SemanticTokenLegendType::Type);
    }

    declaration.getBody()->accept(*this);
  }

  void visitPropertyDeclaration(
      ast::PropertyDeclaration& declaration) override {
    SemanticTokenLegendModifier modifiers{
        SemanticTokenLegendModifierBits::Declaration};
    if (declaration.isReadonly()) {
      modifiers |= SemanticTokenLegendModifierBits::Readonly;
    }

    addSpan(spans, declaration.getNameLocation(),
            SemanticTokenLegendType::Property, modifiers);
    addSpan(spans, declaration.getTypeLocation(),
            SemanticTokenLegendType::Type);

    if (const auto& getBlock = declaration.getGetAccessor()) {
      getBlock.value()->accept(*this);
    }
    if (const auto& setBlock = declaration.getSetAccessor()) {
      setBlock.value()->accept(*this);
    }
  }

  void visitExpressionStatement(ast::ExpressionStatement& statement) override {
    statement.getExpression()->accept(*this);
  }

  void visitReturnStatement(ast::ReturnStatement& statement) override {
    if (statement.getExpression()) {
      statement.getExpression()->accept(*this);
    }
  }

  void visitIfStatement(ast::IfStatement& statement) override {
    statement.getCondition()->accept(*this);
    statement.getThenBlock()->accept(*this);
    if (const auto& elseBlock = statement.getElseBlock()) {
      elseBlock->accept(*this);
    }
  }

  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override {
    addSpan(spans, statement.getNameLocation(),
            SemanticTokenLegendType::Variable,
            SemanticTokenLegendModifierBits::Declaration);

    addSpan(spans, statement.getTypeLocation(), SemanticTokenLegendType::Type);

    if (const auto& init = statement.getInitializer()) {
      init->accept(*this);
    }
  }

  void visitWhileStatement(ast::WhileStatement& statement) override {
    statement.getCondition()->accept(*this);
    statement.getBody()->accept(*this);
  }

  void visitBreakStatement(ast::BreakStatement&) override {}
  void visitContinueStatement(ast::ContinueStatement&) override {}

  void visitForStatement(ast::ForStatement& statement) override {
    addSpan(spans, statement.getVariableNameLocation(),
            SemanticTokenLegendType::Variable,
            SemanticTokenLegendModifierBits::Declaration);
    statement.getArray()->accept(*this);
    statement.getBody()->accept(*this);
  }

  void visitBlockStatement(ast::BlockStatement& statement) override {
    for (const auto& stmt : statement.getStatements()) {
      stmt->accept(*this);
    }
  }

  void visitIdentifierExpression(ast::IdentifierExpression& expr) override {
    if (expr.getIdentifierType() == VellumValueType::ScriptType) {
      addSpan(spans, expr.getLocation(), SemanticTokenLegendType::Type);
    } else {
      addSpan(spans, expr.getLocation(), SemanticTokenLegendType::Variable);
    }
  }

  void visitCallExpression(ast::CallExpression& expr) override {
    const auto& callee = expr.getCallee();

    if (callee->isPropertyGetExpression()) {
      const auto& propGet = callee->asPropertyGet();
      const bool isStaticCall = expr.getFunctionCall() &&
                                expr.getFunctionCall()->isStatic();
      SemanticTokenLegendModifier functionModifiers{};
      if (isStaticCall) {
        functionModifiers |= SemanticTokenLegendModifierBits::Static;
      }

      if (isStaticCall && propGet.getObject()->isIdentifierExpression()) {
        addSpan(spans, propGet.getObject()->getLocation(),
                SemanticTokenLegendType::Type);
      } else {
        propGet.getObject()->accept(*this);
      }

      addSpan(spans, propGet.getLocation(), SemanticTokenLegendType::Function,
              functionModifiers);
    } else if (callee->isIdentifierExpression()) {
      SemanticTokenLegendModifier functionModifiers{};
      if (expr.getFunctionCall() && expr.getFunctionCall()->isStatic()) {
        functionModifiers |= SemanticTokenLegendModifierBits::Static;
      }
      addSpan(spans, callee->getLocation(), SemanticTokenLegendType::Function,
              functionModifiers);
    } else {
      callee->accept(*this);
    }

    for (const auto& arg : expr.getArguments()) {
      arg->accept(*this);
    }
  }

  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override {
    addSpan(spans, expr.getLocation(), SemanticTokenLegendType::Property);
    expr.getObject()->accept(*this);
  }

  void visitPropertySetExpression(ast::PropertySetExpression& expr) override {
    addSpan(spans, expr.getPropertyLocation(),
            SemanticTokenLegendType::Property);
    expr.getObject()->accept(*this);
    expr.getValue()->accept(*this);
  }

  void visitAssignExpression(ast::AssignExpression& expr) override {
    expr.getName()->accept(*this);
    expr.getValue()->accept(*this);
  }

  void visitBinaryExpression(ast::BinaryExpression& expr) override {
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }

  void visitUnaryExpression(ast::UnaryExpression& expr) override {
    expr.getOperand()->accept(*this);
  }

  void visitCastExpression(ast::CastExpression& expr) override {
    expr.getExpression()->accept(*this);
    addSpan(spans, expr.getTargetExpression()->getLocation(),
            SemanticTokenLegendType::Type);
  }

  void visitNewArrayExpression(ast::NewArrayExpression& expr) override {
    if (const auto& subtype = expr.getSubtype()) {
      if (const auto& subtypeLocation = expr.getSubtypeLocation()) {
        addSpan(spans, *subtypeLocation, SemanticTokenLegendType::Type);
      } else if (subtype->getState() == VellumTypeState::Identifier) {
        addSpan(spans, expr.getLocation(), SemanticTokenLegendType::Type);
      }
    }
  }

  void visitNewArrayElementsExpression(
      ast::NewArrayElementsExpression& expr) override {
    for (const auto& element : expr.getElements()) {
      element->accept(*this);
    }
  }

  void visitSelfExpression(ast::SelfExpression&) override {}
  void visitSuperExpression(ast::SuperExpression&) override {}

  void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override {
    expr.getArray()->accept(*this);
    expr.getIndex()->accept(*this);
  }

  void visitArrayIndexSetExpression(
      ast::ArrayIndexSetExpression& expr) override {
    expr.getArray()->accept(*this);
    expr.getIndex()->accept(*this);
    expr.getValue()->accept(*this);
  }

  void visitTernaryExpression(ast::TernaryExpression& expr) override {
    expr.getCondition()->accept(*this);
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
  }

 private:
  common::Vec<SemanticTokenSpan>& spans;
};

common::Vec<SemanticTokenSpan> tokenizeDocument(std::string_view source,
                                                bool includeIdentifiers) {
  Lexer lexer(source);
  common::Vec<SemanticTokenSpan> spans;

  for (;;) {
    const Token token = lexer.scanToken();
    if (token.type == TokenType::END_OF_FILE) {
      break;
    }

    if (token.type == TokenType::IDENTIFIER) {
      if (!includeIdentifiers) {
        continue;
      }
      addSpan(spans, token, SemanticTokenLegendType::Variable);
      continue;
    }

    const auto tokenType = mapLexerTokenType(token.type);
    if (!tokenType) {
      continue;
    }

    if (const auto span = spanFromToken(token, *tokenType)) {
      spans.push_back(*span);
    }
  }

  return spans;
}

lsp::Array<lsp::uint> encodeSemanticTokens(
    common::Vec<SemanticTokenSpan> spans) {
  std::sort(spans.begin(), spans.end(),
            [](const SemanticTokenSpan& a, const SemanticTokenSpan& b) {
              if (a.line != b.line) {
                return a.line < b.line;
              }
              return a.character < b.character;
            });

  lsp::Array<lsp::uint> data;
  data.reserve(spans.size() * 5);

  int prevLine = 0;
  int prevChar = 0;

  for (const auto& span : spans) {
    const int deltaLine = span.line - prevLine;
    const int deltaStart =
        deltaLine == 0 ? span.character - prevChar : span.character;

    data.push_back(deltaLine);
    data.push_back(deltaStart);
    data.push_back(span.length);
    data.push_back(span.tokenType);
    data.push_back(span.tokenModifiers);

    prevLine = span.line;
    prevChar = span.character;
  }

  return data;
}

common::Vec<SemanticTokenSpan> collectSemanticSpansFromParse(
    ParserResult& parseResult, std::string_view source) {
  common::Vec<SemanticTokenSpan> spans;
  SemanticTokensCollector collector(spans);
  collector.collect(parseResult.declarations);

  auto lexerSpans = tokenizeDocument(source, false);
  spans.insert(spans.end(), lexerSpans.begin(), lexerSpans.end());
  return spans;
}

}  // namespace

lsp::SemanticTokens buildSemanticTokensFromParse(ParserResult& parseResult,
                                                 std::string_view source) {
  auto spans = collectSemanticSpansFromParse(parseResult, source);
  return lsp::SemanticTokens{.data = encodeSemanticTokens(std::move(spans))};
}

lsp::SemanticTokens buildSemanticTokensLexerFallback(std::string_view source) {
  auto spans = tokenizeDocument(source, true);
  return lsp::SemanticTokens{.data = encodeSemanticTokens(std::move(spans))};
}

}  // namespace vellum
