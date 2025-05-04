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
  virtual bool equals(const Declaration& other) const = 0;
};

bool operator==(const Declaration& lhs, const Declaration& rhs);
bool operator!=(const Declaration& lhs, const Declaration& rhs);

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
  bool equals(const Declaration& other) const override;

 private:
  std::string_view scriptName_;
  std::optional<std::string_view> parentScriptName_;
};

class GlobalVariableDeclaration : public Declaration {
 public:
  GlobalVariableDeclaration(std::string_view name,
                            std::optional<VellumType> typeName,
                            std::unique_ptr<Expression> initializer)
      : name_(name),
        typeName_(typeName),
        initializer_(std::move(initializer)) {}

  std::string_view name() const { return name_; }
  std::optional<VellumType> typeName() const { return typeName_; }
  std::optional<VellumType>& typeName() { return typeName_; }
  const std::unique_ptr<Expression>& initializer() const {
    return initializer_;
  }

  VellumValue getValue() const;

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

 private:
  std::string_view name_;
  std::optional<VellumType> typeName_;
  std::unique_ptr<Expression> initializer_;
};

struct FunctionParameter {
  std::string_view name;
  std::string_view type;
};

bool operator==(const FunctionParameter& lhs, const FunctionParameter& rhs);
bool operator!=(const FunctionParameter& lhs, const FunctionParameter& rhs);

using FunctionBody = std::vector<std::unique_ptr<Statement>>;

class FunctionDeclaration : public Declaration {
 public:
  FunctionDeclaration(std::optional<std::string_view> name,
                      std::vector<FunctionParameter> parameters,
                      std::optional<std::string_view> returnTypeName,
                      FunctionBody body)
      : name(name),
        parameters(std::move(parameters)),
        returnTypeName(returnTypeName),
        body(std::move(body)) {}

  std::optional<std::string_view> getName() const { return name; }
  const std::vector<FunctionParameter>& getParameters() const {
    return parameters;
  }
  std::optional<std::string_view> getReturnTypeName() const {
    return returnTypeName;
  }
  const FunctionBody& getBody() const { return body; }

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

 private:
  std::optional<std::string_view> name;
  std::vector<FunctionParameter> parameters;
  std::optional<std::string_view> returnTypeName;
  FunctionBody body;
};

class PropertyDeclaration : public Declaration {
 public:
  PropertyDeclaration(std::string_view name, std::string_view typeName,
                      std::string_view documentationString,
                      std::optional<ast::FunctionBody> getAccessor,
                      std::optional<ast::FunctionBody> setAccessor,
                      VellumValue defaultValue)
      : name(name),
        typeName(typeName),
        documentationString(documentationString),
        getAccessor(std::move(getAccessor)),
        setAccessor(std::move(setAccessor)),
        defaultValue(defaultValue) {}

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  bool isAutoProperty() const {
    return getAccessor && setAccessor && getAccessor->empty() &&
           setAccessor->empty();
  }
  bool isReadonly() const { return !setAccessor && getAccessor; }

  std::string_view getName() const { return name; }
  std::string_view getTypeName() const { return typeName; }
  std::string_view getDocumentationString() const {
    return documentationString;
  }

  const std::optional<ast::FunctionBody>& getGetAccessor() const {
    return getAccessor;
  }
  std::optional<ast::FunctionBody> getGetAccessor() {
    return std::move(getAccessor);
  }

  const std::optional<ast::FunctionBody>& getSetAccessor() const {
    return setAccessor;
  }
  std::optional<ast::FunctionBody> getSetAccessor() {
    return std::move(setAccessor);
  }

  VellumValue getDefaultValue() const { return defaultValue; }

 private:
  std::string_view name;
  std::string_view typeName;
  std::string_view documentationString;

  std::optional<ast::FunctionBody> getAccessor;
  std::optional<ast::FunctionBody> setAccessor;

  VellumValue defaultValue;
};
}  // namespace ast
}  // namespace vellum