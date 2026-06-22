#pragma once
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/expression/expression.h"
#include "common/types.h"
#include "lexer/token.h"

namespace vellum {

namespace ast {
using common::Opt;
using common::Unique;
using common::Vec;

class StatementVisitor;

class Statement {
 public:
  virtual ~Statement() = default;
  virtual void accept(StatementVisitor& visitor) = 0;
  virtual bool equals(const Statement& other) const = 0;
};

bool operator==(const Statement& lhs, const Statement& rhs);
bool operator!=(const Statement& lhs, const Statement& rhs);

class ExpressionStatement : public Statement {
 public:
  explicit ExpressionStatement(Unique<Expression> expression)
      : expression(std::move(expression)) {}

  const Unique<Expression>& getExpression() const { return expression; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> expression;
};

class ReturnStatement : public Statement {
 public:
  ReturnStatement(Unique<Expression> expression, Token returnToken = Token())
      : expression(std::move(expression)), returnToken(returnToken) {}

  const Unique<Expression>& getExpression() const { return expression; }

  Token getLocation() const {
    return expression ? expression->getLocation() : returnToken;
  }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> expression;
  Token returnToken;
};

class BlockStatement : public Statement {
 public:
  BlockStatement(Vec<Unique<Statement>> statements)
      : statements(std::move(statements)) {}

  const Vec<Unique<Statement>>& getStatements() const { return statements; }
  Vec<Unique<Statement>>& getStatements() { return statements; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Vec<Unique<Statement>> statements;
};

class IfStatement : public Statement {
 public:
  IfStatement(Unique<Expression> condition, Unique<Statement> thenBlock,
              Unique<Statement> elseBlock = nullptr)
      : condition(std::move(condition)),
        thenBlock(std::move(thenBlock)),
        elseBlock(std::move(elseBlock)) {}

  const Unique<Expression>& getCondition() const { return condition; }
  const Unique<Statement>& getThenBlock() const { return thenBlock; }
  const Unique<Statement>& getElseBlock() const { return elseBlock; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> condition;
  Unique<Statement> thenBlock;
  Unique<Statement> elseBlock;
};

class LocalVariableStatement : public Statement {
 public:
  LocalVariableStatement(VellumIdentifier name, Opt<VellumType> type,
                         Unique<Expression> initializer,
                         Token nameLocation = Token(),
                         Opt<Token> typeLocation = std::nullopt)
      : name(name),
        type(type),
        initializer(std::move(initializer)),
        nameLocation(nameLocation),
        typeLocation(typeLocation) {}

  VellumIdentifier getName() const { return name; }

  Opt<VellumType> getType() const { return type; }
  void setType(VellumType type_) { type = type_; }

  const Unique<Expression>& getInitializer() const { return initializer; }
  Token getNameLocation() const { return nameLocation; }
  Opt<Token> getTypeLocation() const { return typeLocation; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  VellumIdentifier name;
  Opt<VellumType> type;
  Unique<Expression> initializer;
  Token nameLocation;
  Opt<Token> typeLocation;
};

class WhileStatement : public Statement {
 public:
  WhileStatement(Unique<Expression> condition, Unique<Statement> body)
      : condition(std::move(condition)), body(std::move(body)) {}

  const Unique<Expression>& getCondition() const { return condition; }

  const Unique<Statement>& getBody() const { return body; }

  Unique<Statement>& getBody() { return body; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> condition;
  Unique<Statement> body;
};

class BreakStatement : public Statement {
 public:
  explicit BreakStatement(Token breakToken) : breakToken(breakToken) {}

  const Token& getLocation() const { return breakToken; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Token breakToken;
};

class ContinueStatement : public Statement {
 public:
  explicit ContinueStatement(Token continueToken)
      : continueToken(continueToken) {}

  const Token& getLocation() const { return continueToken; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Token continueToken;
};

class ForStatement : public Statement {
 public:
  ForStatement(Unique<Expression> variableName, Unique<Expression> array,
               Unique<Statement> body, Token variableNameLocation = {},
               Token arrayLocation = {})
      : variableName(std::move(variableName)),
        array(std::move(array)),
        body(std::move(body)),
        variableNameLocation(variableNameLocation),
        arrayLocation(arrayLocation) {}

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

  const Unique<Expression>& getVariableName() const { return variableName; }

  Unique<Expression>& getVariableName() { return variableName; }

  const Unique<Expression>& getArray() const { return array; }

  const Unique<Statement>& getBody() const { return body; }

  Unique<Statement>& getBody() { return body; }

  Token getVariableNameLocation() const { return variableNameLocation; }

  Token getArrayLocation() const { return arrayLocation; }

  void setCounterMangledName(std::string_view name) {
    counterMangledName = name;
  }

  const std::string& getCounterMangledName() const {
    return counterMangledName;
  }

 private:
  Unique<Expression> variableName;
  Unique<Expression> array;
  Unique<Statement> body;
  Token variableNameLocation;
  Token arrayLocation;
  std::string counterMangledName;
};

}  // namespace ast
}  // namespace vellum