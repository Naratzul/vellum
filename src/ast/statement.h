#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/expression.h"
#include "vellum/vellum_value.h"

namespace vellum {

namespace ast {

class StatementVisitor;

class Statement {
 public:
  virtual ~Statement() = default;
  virtual void accept(StatementVisitor& visitor) = 0;
};

class ScriptStatement : public Statement {
 public:
  ScriptStatement(std::string_view scriptName,
                  std::optional<std::string_view> parentScriptName)
      : scriptName_(scriptName), parentScriptName_(parentScriptName) {}

  std::string_view scriptName() const { return scriptName_; }
  std::optional<std::string_view> parentScriptName() const {
    return parentScriptName_;
  }

  void accept(StatementVisitor& visitor) override;

 private:
  std::string_view scriptName_;
  std::optional<std::string_view> parentScriptName_;
};

class VariableDeclaration : public Statement {
 public:
  VariableDeclaration(std::string_view name,
                      std::optional<std::string_view> typeName,
                      std::unique_ptr<Expression> initializer)
      : name_(name),
        typeName_(typeName),
        initializer_(std::move(initializer)) {}

  std::string_view name() const { return name_; }
  std::optional<std::string_view> typeName() const { return typeName_; }
  std::optional<std::string_view>& typeName() { return typeName_; }
  const std::unique_ptr<Expression>& initializer() { return initializer_; }

  VellumValue getValue() const;

  void accept(StatementVisitor& visitor) override;

 private:
  std::string_view name_;
  std::optional<std::string_view> typeName_;
  std::unique_ptr<Expression> initializer_;
};

class FunctionDeclaration : public Statement {
 public:
  FunctionDeclaration(std::string_view name,
                      std::optional<std::string_view> returnTypeName,
                      std::vector<std::unique_ptr<Statement>> body)
      : name_(name),
        returnTypeName_(returnTypeName),
        body_(std::move(body)) {}

  std::string_view name() const { return name_; }
  std::optional<std::string_view> returnTypeName() const { return returnTypeName_; }
  const std::vector<std::unique_ptr<Statement>>& body() const { return body_; } 

  void accept(StatementVisitor& visitor) override;

 private:
  std::string_view name_;
  std::optional<std::string_view> returnTypeName_;
  std::vector<std::unique_ptr<Statement>> body_;
};
}  // namespace ast
}  // namespace vellum