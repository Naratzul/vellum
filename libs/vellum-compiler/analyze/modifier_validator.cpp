#include "modifier_validator.h"

#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"
#include "vellum/vellum_modifier.h"

namespace vellum {
ModifierValidator::ModifierValidator(
    const Shared<CompilerErrorHandler>& errorHandler)
    : errorHandler(errorHandler) {}

void ModifierValidator::validate(Vec<Unique<ast::Declaration>>& declarations) {
  for (auto& declaration : declarations) {
    declaration->accept(*this);
  }
}

void ModifierValidator::visitImportDeclaration(
    ast::ImportDeclaration& declaration) {
  validateModifiersContext(declaration.getModifiers(),
                           VellumModifierContext::Import, *errorHandler);
}

void ModifierValidator::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  validateModifiersContext(declaration.getModifiers(),
                           VellumModifierContext::Script, *errorHandler);
  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void ModifierValidator::visitStateDeclaration(
    ast::StateDeclaration& declaration) {
  validateModifiersContext(declaration.getModifiers(),
                           VellumModifierContext::State, *errorHandler);
  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void ModifierValidator::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  validateModifiersContext(declaration.getModifiers(),
                           VellumModifierContext::Var, *errorHandler);
}

void ModifierValidator::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  const VellumModifierContext context = declaration.isEvent()
                                            ? VellumModifierContext::Event
                                            : VellumModifierContext::Function;
  validateModifiersContext(declaration.getModifiers(), context,
                           *errorHandler);
}

void ModifierValidator::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  validateModifiersContext(declaration.getModifiers(),
                           VellumModifierContext::Property, *errorHandler);
}
}  // namespace vellum
