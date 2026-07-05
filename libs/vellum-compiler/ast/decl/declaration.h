#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/statement/statement.h"
#include "common/types.h"
#include "lexer/token.h"
#include "vellum/vellum_modifier.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Unique;

namespace ast {

class DeclarationVisitor;

enum class DeclarationOrder {
  Import,
  Script,
  Variable,
  Function,
  Property,
  State,
};

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
  ImportDeclaration(std::string_view importName, Token importNameLocation,
                    ParsedModifiers modifiers = {})
      : importName(importName),
        importNameLocation(importNameLocation),
        modifiers_(std::move(modifiers)) {}

  std::string_view getImportName() const { return importName; }
  Token getImportNameLocation() const { return importNameLocation; }
  const ParsedModifiers& getModifiers() const { return modifiers_; }

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Import;
  }

 private:
  std::string_view importName;
  Token importNameLocation;
  ParsedModifiers modifiers_;
};

class ScriptDeclaration : public Declaration {
 public:
  ScriptDeclaration(VellumType scriptName, Token scriptNameLocation,
                    VellumType parentScriptName,
                    Opt<Token> parentScriptNameLocation,
                    Vec<Unique<ast::Declaration>> members,
                    ParsedModifiers modifiers = {})
      : scriptName(scriptName),
        parentScriptName(parentScriptName),
        scriptNameLocation(scriptNameLocation),
        parentScriptNameLocation(parentScriptNameLocation),
        members(std::move(members)),
        modifiers(std::move(modifiers)) {}

  VellumType getScriptName() const { return scriptName; }
  VellumType getParentScriptName() const { return parentScriptName; }

  Token getScriptNameLocation() const { return scriptNameLocation; }
  Opt<Token> getParentScriptNameLocation() const {
    return parentScriptNameLocation;
  }

  const Vec<Unique<ast::Declaration>>& getMemberDecls() const {
    return members;
  }

  const ParsedModifiers& getModifiers() const { return modifiers; }

  bool isHidden() const {
    return modifiersBitmask(modifiers) & VellumModifier::Hidden;
  }

  bool isConditional() const {
    return modifiersBitmask(modifiers) & VellumModifier::Conditional;
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
  ParsedModifiers modifiers;
};

class StateDeclaration : public Declaration {
 public:
  StateDeclaration(std::string_view stateName, Token stateNameLocation,
                   ParsedModifiers modifiers,
                   Vec<Unique<ast::Declaration>> members)
      : stateName(stateName),
        stateNameLocation(stateNameLocation),
        modifiers_(std::move(modifiers)),
        members(std::move(members)) {}

  std::string_view getStateName() const { return stateName; }
  Token getStateNameLocation() const { return stateNameLocation; }
  bool getIsAuto() const {
    return hasModifier(modifiers_, VellumModifier::Auto);
  }

  const ParsedModifiers& getModifiers() const { return modifiers_; }

  const Vec<Unique<ast::Declaration>>& getMemberDecls() const {
    return members;
  }

  Vec<Unique<ast::Declaration>>& getMemberDecls() { return members; }

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override { return DeclarationOrder::State; }

 private:
  std::string_view stateName;
  Token stateNameLocation;
  ParsedModifiers modifiers_;
  Vec<Unique<ast::Declaration>> members;
};

class GlobalVariableDeclaration : public Declaration {
 public:
  GlobalVariableDeclaration(std::string_view name, Opt<VellumType> typeName,
                            Unique<Expression> initializer,
                            Opt<Token> nameLocation = std::nullopt,
                            Opt<Token> typeLocation = std::nullopt,
                            ParsedModifiers modifiers = {})
      : name_(name),
        typeName_(typeName),
        initializer_(std::move(initializer)),
        nameLocation_(nameLocation),
        typeLocation_(typeLocation),
        modifiers(std::move(modifiers)) {}

  std::string_view name() const { return name_; }
  Opt<VellumType> typeName() const { return typeName_; }
  Opt<VellumType>& typeName() { return typeName_; }
  const Unique<Expression>& initializer() const { return initializer_; }
  Opt<Token> getNameLocation() const { return nameLocation_; }
  Opt<Token> getTypeLocation() const { return typeLocation_; }
  const ParsedModifiers& getModifiers() const { return modifiers; }
  bool isConditional() const {
    return modifiersBitmask(modifiers) & VellumModifier::Conditional;
  }

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
  ParsedModifiers modifiers;
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
  FunctionParameter(std::string_view name, VellumType type,
                    Opt<VellumLiteral> defaultValue,
                    Token nameLocation = Token(), Token typeLocation = Token())
      : name(name),
        type(type),
        defaultValue(std::move(defaultValue)),
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
                      VellumType returnTypeName, Unique<BlockStatement> body,
                      ParsedModifiers modifiers = {},
                      Opt<Token> nameLocation = std::nullopt,
                      Opt<Token> returnTypeLocation = std::nullopt,
                      bool isEvent = false)
      : name(name),
        parameters(std::move(parameters)),
        returnTypeName(returnTypeName),
        body(std::move(body)),
        modifiers_(std::move(modifiers)),
        isEvent_(isEvent),
        nameLocation(nameLocation),
        returnTypeLocation(returnTypeLocation) {}

  Opt<std::string_view> getName() const { return name; }
  const Vec<FunctionParameter>& getParameters() const { return parameters; }
  Vec<FunctionParameter>& getParameters() { return parameters; }
  const VellumType& getReturnTypeName() const { return returnTypeName; }
  VellumType& getReturnTypeName() { return returnTypeName; }
  const Unique<BlockStatement>& getBody() const { return body; }
  Unique<BlockStatement>& getBody() { return body; }
  bool isStatic() const {
    return modifiersBitmask(modifiers_) & VellumModifier::Static;
  }
  bool isNative() const {
    return modifiersBitmask(modifiers_) & VellumModifier::Native;
  }
  bool isEvent() const { return isEvent_; }

  const ParsedModifiers& getModifiers() const { return modifiers_; }
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
  Unique<BlockStatement> body;
  ParsedModifiers modifiers_;
  bool isEvent_;
  Opt<Token> nameLocation;
  Opt<Token> returnTypeLocation;
};

class PropertyDeclaration : public Declaration {
 public:
  PropertyDeclaration(std::string_view name, VellumType typeName,
                      std::string_view documentationString,
                      Opt<Unique<BlockStatement>> getAccessor,
                      Opt<Unique<BlockStatement>> setAccessor,
                      Opt<VellumLiteral> defaultValue,
                      Token nameLocation = Token(),
                      Token typeLocation = Token(),
                      ParsedModifiers modifiers = {})
      : name(name),
        typeName(typeName),
        documentationString(documentationString),
        getAccessor(std::move(getAccessor)),
        setAccessor(std::move(setAccessor)),
        defaultValue(defaultValue),
        nameLocation(nameLocation),
        typeLocation(typeLocation),
        modifiers(std::move(modifiers)) {}

  void accept(DeclarationVisitor& visitor) override;
  bool equals(const Declaration& other) const override;

  DeclarationOrder getOrder() const override {
    return DeclarationOrder::Property;
  }

  bool isAutoProperty() const {
    return getAccessor && setAccessor &&
           getAccessor.value()->getStatements().empty() &&
           setAccessor.value()->getStatements().empty();
  }
  bool isAutoReadonly() const {
    return isReadonly() && getAccessor.value()->getStatements().empty();
  }
  bool isReadonly() const { return !setAccessor && getAccessor; }

  std::string_view getName() const { return name; }
  VellumType& getTypeName() { return typeName; }
  const VellumType& getTypeName() const { return typeName; }
  std::string_view getDocumentationString() const {
    return documentationString;
  }

  const Opt<Unique<BlockStatement>>& getGetAccessor() const {
    return getAccessor;
  }
  Opt<Unique<BlockStatement>> releaseGetAccessor() {
    return std::move(getAccessor);
  }

  const Opt<Unique<BlockStatement>>& getSetAccessor() const {
    return setAccessor;
  }
  Opt<Unique<BlockStatement>> releaseSetAccessor() {
    return std::move(setAccessor);
  }

  Opt<VellumLiteral> getDefaultValue() const { return defaultValue; }
  Token getNameLocation() const { return nameLocation; }
  Token getTypeLocation() const { return typeLocation; }
  const ParsedModifiers& getModifiers() const { return modifiers; }

  bool isConditional() const {
    return modifiersBitmask(modifiers) & VellumModifier::Conditional;
  }

  bool isHidden() const {
    return modifiersBitmask(modifiers) & VellumModifier::Hidden;
  }

 private:
  std::string_view name;
  VellumType typeName;
  std::string_view documentationString;

  Opt<Unique<BlockStatement>> getAccessor;
  Opt<Unique<BlockStatement>> setAccessor;

  Opt<VellumLiteral> defaultValue;
  Token nameLocation;
  Token typeLocation;
  ParsedModifiers modifiers;
};
}  // namespace ast
}  // namespace vellum