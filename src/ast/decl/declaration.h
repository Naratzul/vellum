#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/statement/statement.h"
#include "vellum/vellum_value.h"

namespace vellum {

namespace ast {

class DeclarationVisitor;

class Declaration {
 public:
  virtual ~Declaration() = default;
  virtual void accept(DeclarationVisitor& visitor) = 0;
};

class ScriptDeclaration : public Declaration {
 public:
  ScriptDeclaration(std::string_view scriptName,
                    std::optional<std::string_view> parentScriptName)
      : scriptName_(scriptName), parentScriptName_(parentScriptName) {}

  std::string_view scriptName() const { return scriptName_; }
  std::optional<std::string_view> parentScriptName() const {
    return parentScriptName_;
  }

  void accept(DeclarationVisitor& visitor) override;

 private:
  std::string_view scriptName_;
  std::optional<std::string_view> parentScriptName_;
};

class GlobalVariableDeclaration : public Declaration {
 public:
  GlobalVariableDeclaration(std::string_view name,
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

  void accept(DeclarationVisitor& visitor) override;

 private:
  std::string_view name_;
  std::optional<std::string_view> typeName_;
  std::unique_ptr<Expression> initializer_;
};

struct FunctionParameter {
  std::string_view name;
  std::string_view type;
};

class FunctionDeclaration : public Declaration {
 public:
  FunctionDeclaration(std::string_view name,
                      std::vector<FunctionParameter> parameters,
                      std::optional<std::string_view> returnTypeName,
                      std::vector<std::unique_ptr<Statement>> body)
      : name(name),
        parameters(std::move(parameters)),
        returnTypeName(returnTypeName),
        body(std::move(body)) {}

  std::string_view getName() const { return name; }
  const std::vector<FunctionParameter>& getParameters() const {
    return parameters;
  }
  std::optional<std::string_view> getReturnTypeName() const {
    return returnTypeName;
  }
  const std::vector<std::unique_ptr<Statement>>& getBody() const {
    return body;
  }

  void accept(DeclarationVisitor& visitor) override;

 private:
  std::string_view name;
  std::optional<std::string_view> returnTypeName;
  std::vector<FunctionParameter> parameters;
  std::vector<std::unique_ptr<Statement>> body;
};
}  // namespace ast
}  // namespace vellum