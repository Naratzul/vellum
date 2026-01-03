#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/statement/statement.h"
#include "common/types.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Unique;

namespace ast {

class DeclarationVisitor;

enum class DeclarationOrder { Import, Script, Variable, Function, Property };

class Declaration {
 public:
  virtual ~Declaration() = default;
  virtual void accept(DeclarationVisitor& visitor) = 0;
  virtual bool equals(const Declaration& other) const = 0;
  virtual DeclarationOrder getOrder() const = 0;
};

bool operator==(const Declaration& lhs, const Declaration& rhs);
bool operator!=(const Declaration& lhs, const Declaration& rhs);

class ScriptDeclaration : public Declaration {
 public:
  ScriptDeclaration(std::string_view scriptName, Token scriptNameLocation,
                    Opt<std::string_view> parentScriptName,
                    Opt<Token> parentScriptNameLocation,
                    Vec<Unique<ast::Declaration>> members)
      : scriptName_(scriptName),
        parentScriptName_(parentScriptName),
        scriptNameLocation(scriptNameLocation),
        parentScriptNameLocation(parentScriptNameLocation),
        members(std::move(members)) {}

  std::string_view scriptName() const { return scriptName_; }
  Opt<std::string_view> parentScriptName() const { return parentScriptName_; }

  Token getScriptNameLocation() const { return scriptNameLocation; }
  Opt<Token> getParentScriptNameLocation() const {
    return parentScriptNameLocation;
  }

  const Vec<Unique<ast::Declaration>>& getMemberDecls() const {
    return members;
  }

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Script;
  }

 private:
  std::string_view scriptName_;
  Opt<std::string_view> parentScriptName_;

  Token scriptNameLocation;
  Opt<Token> parentScriptNameLocation;

  Vec<Unique<ast::Declaration>> members;
};

class GlobalVariableDeclaration : public Declaration {
 public:
  GlobalVariableDeclaration(std::string_view name, Opt<VellumType> typeName,
                            Unique<Expression> initializer)
      : name_(name),
        typeName_(typeName),
        initializer_(std::move(initializer)) {}

  std::string_view name() const { return name_; }
  Opt<VellumType> typeName() const { return typeName_; }
  Opt<VellumType>& typeName() { return typeName_; }
  const Unique<Expression>& initializer() const { return initializer_; }

  VellumValue getValue() const;

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Variable;
  }

 private:
  std::string_view name_;
  Opt<VellumType> typeName_;
  Unique<Expression> initializer_;
};

struct FunctionParameter {
  std::string_view name;
  VellumType type;
};

bool operator==(const FunctionParameter& lhs, const FunctionParameter& rhs);
bool operator!=(const FunctionParameter& lhs, const FunctionParameter& rhs);

using FunctionBody = Vec<Unique<Statement>>;

class FunctionDeclaration : public Declaration {
 public:
  FunctionDeclaration(Opt<std::string_view> name,
                      Vec<FunctionParameter> parameters,
                      VellumType returnTypeName, FunctionBody body,
                      bool staticFunc)
      : name(name),
        parameters(std::move(parameters)),
        returnTypeName(returnTypeName),
        body(std::move(body)),
        staticFunc(staticFunc) {}

  Opt<std::string_view> getName() const { return name; }
  const Vec<FunctionParameter>& getParameters() const { return parameters; }
  Vec<FunctionParameter>& getParameters() { return parameters; }
  const VellumType& getReturnTypeName() const { return returnTypeName; }
  VellumType& getReturnTypeName() { return returnTypeName; }
  const FunctionBody& getBody() const { return body; }
  bool isStatic() const { return staticFunc; }

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Function;
  }

 private:
  Opt<std::string_view> name;
  Vec<FunctionParameter> parameters;
  VellumType returnTypeName;
  FunctionBody body;
  bool staticFunc;
};

class PropertyDeclaration : public Declaration {
 public:
  PropertyDeclaration(std::string_view name, VellumType typeName,
                      std::string_view documentationString,
                      Opt<ast::FunctionBody> getAccessor,
                      Opt<ast::FunctionBody> setAccessor,
                      VellumValue defaultValue)
      : name(name),
        typeName(typeName),
        documentationString(documentationString),
        getAccessor(std::move(getAccessor)),
        setAccessor(std::move(setAccessor)),
        defaultValue(defaultValue) {}

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Property;
  }

  bool isAutoProperty() const {
    return getAccessor && setAccessor && getAccessor->empty() &&
           setAccessor->empty();
  }
  bool isReadonly() const { return !setAccessor && getAccessor; }

  std::string_view getName() const { return name; }
  VellumType& getTypeName() { return typeName; }
  const VellumType& getTypeName() const { return typeName; }
  std::string_view getDocumentationString() const {
    return documentationString;
  }

  const Opt<ast::FunctionBody>& getGetAccessor() const { return getAccessor; }
  Opt<ast::FunctionBody> getGetAccessor() { return std::move(getAccessor); }

  const Opt<ast::FunctionBody>& getSetAccessor() const { return setAccessor; }
  Opt<ast::FunctionBody> getSetAccessor() { return std::move(setAccessor); }

  VellumValue getDefaultValue() const { return defaultValue; }

 private:
  std::string_view name;
  VellumType typeName;
  std::string_view documentationString;

  Opt<ast::FunctionBody> getAccessor;
  Opt<ast::FunctionBody> setAccessor;

  VellumValue defaultValue;
};
}  // namespace ast
}  // namespace vellum