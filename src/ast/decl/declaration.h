#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/statement/statement.h"
#include "common/types.h"
#include "lexer/token.h"
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

class ImportDeclaration : public Declaration {
 public:
  ImportDeclaration(std::string_view importName, Token importNameLocation)
      : importName(importName), importNameLocation(importNameLocation) {}

  std::string_view getImportName() const { return importName; }
  Token getImportNameLocation() const { return importNameLocation; }

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Import;
  }

 private:
  std::string_view importName;
  Token importNameLocation;
};

class ScriptDeclaration : public Declaration {
 public:
  ScriptDeclaration(VellumType scriptName, Token scriptNameLocation,
                    VellumType parentScriptName,
                    Opt<Token> parentScriptNameLocation,
                    Vec<Unique<ast::Declaration>> members)
      : scriptName(scriptName),
        parentScriptName(parentScriptName),
        scriptNameLocation(scriptNameLocation),
        parentScriptNameLocation(parentScriptNameLocation),
        members(std::move(members)) {}

  VellumType getScriptName() const { return scriptName; }
  VellumType getParentScriptName() const { return parentScriptName; }

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
  VellumType scriptName;
  VellumType parentScriptName;

  Token scriptNameLocation;
  Opt<Token> parentScriptNameLocation;

  Vec<Unique<ast::Declaration>> members;
};

class GlobalVariableDeclaration : public Declaration {
 public:
  GlobalVariableDeclaration(std::string_view name, Opt<VellumType> typeName,
                            Unique<Expression> initializer,
                            Opt<Token> nameLocation = std::nullopt,
                            Opt<Token> typeLocation = std::nullopt)
      : name_(name),
        typeName_(typeName),
        initializer_(std::move(initializer)),
        nameLocation_(nameLocation),
        typeLocation_(typeLocation) {}

  std::string_view name() const { return name_; }
  Opt<VellumType> typeName() const { return typeName_; }
  Opt<VellumType>& typeName() { return typeName_; }
  const Unique<Expression>& initializer() const { return initializer_; }
  Opt<Token> getNameLocation() const { return nameLocation_; }
  Opt<Token> getTypeLocation() const { return typeLocation_; }

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
  Opt<Token> nameLocation_;
  Opt<Token> typeLocation_;
};

struct FunctionParameter {
  std::string_view name;
  VellumType type;
  Opt<VellumLiteral> defaultValue;
  Token nameLocation;
  Token typeLocation;

  FunctionParameter()
      : name(""), type(VellumType::none()), nameLocation(), typeLocation() {}
  FunctionParameter(std::string_view name, VellumType type,
                    Token nameLocation = Token(), Token typeLocation = Token())
      : name(name),
        type(type),
        nameLocation(nameLocation),
        typeLocation(typeLocation) {}
};

bool operator==(const FunctionParameter& lhs, const FunctionParameter& rhs);
bool operator!=(const FunctionParameter& lhs, const FunctionParameter& rhs);

using FunctionBody = Vec<Unique<Statement>>;

class FunctionDeclaration : public Declaration {
 public:
  FunctionDeclaration(Opt<std::string_view> name,
                      Vec<FunctionParameter> parameters,
                      VellumType returnTypeName, FunctionBody body,
                      bool staticFunc, Opt<Token> nameLocation = std::nullopt,
                      Opt<Token> returnTypeLocation = std::nullopt)
      : name(name),
        parameters(std::move(parameters)),
        returnTypeName(returnTypeName),
        body(std::move(body)),
        staticFunc(staticFunc),
        nameLocation(nameLocation),
        returnTypeLocation(returnTypeLocation) {}

  Opt<std::string_view> getName() const { return name; }
  const Vec<FunctionParameter>& getParameters() const { return parameters; }
  Vec<FunctionParameter>& getParameters() { return parameters; }
  const VellumType& getReturnTypeName() const { return returnTypeName; }
  VellumType& getReturnTypeName() { return returnTypeName; }
  const FunctionBody& getBody() const { return body; }
  bool isStatic() const { return staticFunc; }
  Opt<Token> getNameLocation() const { return nameLocation; }
  Opt<Token> getReturnTypeLocation() const { return returnTypeLocation; }

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
  Opt<Token> nameLocation;
  Opt<Token> returnTypeLocation;
};

class PropertyDeclaration : public Declaration {
 public:
  PropertyDeclaration(std::string_view name, VellumType typeName,
                      std::string_view documentationString,
                      Opt<ast::FunctionBody> getAccessor,
                      Opt<ast::FunctionBody> setAccessor,
                      Opt<VellumLiteral> defaultValue,
                      Token nameLocation = Token(),
                      Token typeLocation = Token())
      : name(name),
        typeName(typeName),
        documentationString(documentationString),
        getAccessor(std::move(getAccessor)),
        setAccessor(std::move(setAccessor)),
        defaultValue(defaultValue),
        nameLocation(nameLocation),
        typeLocation(typeLocation) {}

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

  Opt<VellumLiteral> getDefaultValue() const { return defaultValue; }
  Token getNameLocation() const { return nameLocation; }
  Token getTypeLocation() const { return typeLocation; }

 private:
  std::string_view name;
  VellumType typeName;
  std::string_view documentationString;

  Opt<ast::FunctionBody> getAccessor;
  Opt<ast::FunctionBody> setAccessor;

  Opt<VellumLiteral> defaultValue;
  Token nameLocation;
  Token typeLocation;
};
}  // namespace ast
}  // namespace vellum