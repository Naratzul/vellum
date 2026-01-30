#include "type_collector.h"

#include "ast/decl/declaration.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_type.h"

namespace vellum {

void TypeCollector::collect(Vec<Unique<ast::Declaration>>& declarations) {
  for (auto& decl : declarations) {
    decl->accept(*this);
  }
}

void TypeCollector::visitImportDeclaration(
    ast::ImportDeclaration& declaration) {
  collectTypeFromVellumType(
      VellumType::identifier(declaration.getImportName()));
}

void TypeCollector::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  if (auto parentScriptName = declaration.parentScriptName()) {
    collectTypeFromVellumType(VellumType::identifier(parentScriptName.value()));
  }

  // Recursively collect types from script members
  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void TypeCollector::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  if (auto typeName = declaration.typeName()) {
    collectTypeFromVellumType(typeName.value());
  }
}

void TypeCollector::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  collectTypeFromVellumType(declaration.getReturnTypeName());

  for (const auto& param : declaration.getParameters()) {
    collectTypeFromVellumType(param.type);
  }
}

void TypeCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  collectTypeFromVellumType(declaration.getTypeName());
}

void TypeCollector::collectTypeFromVellumType(const VellumType& type) {
  if (isLiteralType(type)) {
    return;
  }

  if (type.isArray()) {
    collectTypeFromVellumType(*type.asArraySubtype());
    return;
  }

  if (!type.isResolved()) {
    std::string_view rawType = type.asRawType();
    if (!rawType.empty()) {
      discoveredTypes.insert(VellumIdentifier(rawType));
    }
    return;
  }

  if (type.getState() == vellum::VellumTypeState::Identifier) {
    discoveredTypes.insert(type.asIdentifier());
  }
}

bool TypeCollector::isLiteralType(const VellumType& type) const {
  if (type.getState() == vellum::VellumTypeState::Literal) {
    return true;
  }

  if (!type.isResolved()) {
    std::string_view rawType = type.asRawType();
    return literalTypeFromString(rawType).has_value();
  }

  return false;
}

}  // namespace vellum
