#include "declaration_index.h"

#include "ast/decl/declaration.h"
#include "ast/decl/declaration_visitor.h"
#include "common/string_utils.h"
#include "common/types.h"
#include "vellum/vellum_type.h"

namespace vellum {
namespace {

bool namesMatch(VellumIdentifier a, VellumIdentifier b) {
  return common::normalizeToLower(a.toString()) ==
         common::normalizeToLower(b.toString());
}

Opt<VellumIdentifier> identifierFromType(const VellumType& type) {
  if (type.getState() != VellumTypeState::Identifier) {
    return std::nullopt;
  }
  return type.asIdentifier();
}

bool stateMatches(Opt<VellumIdentifier> queryState,
                  Opt<VellumIdentifier> declState) {
  if (!queryState && !declState) {
    return true;
  }
  if (queryState && declState) {
    return namesMatch(*queryState, *declState);
  }
  return false;
}

class DeclarationIndexVisitor : public ast::DeclarationVisitor {
 public:
  DeclarationIndexVisitor(VellumIdentifier name, MemberKind kind,
                          Opt<VellumIdentifier> state)
      : queryName(name), queryKind(kind), queryState(state) {}

  Opt<Token> result() const { return found; }

  void collect(common::Vec<common::Unique<ast::Declaration>>& declarations) {
    for (auto& decl : declarations) {
      if (found) {
        return;
      }
      decl->accept(*this);
    }
  }

  void visitImportDeclaration(ast::ImportDeclaration&) override {}

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override {
    if (queryKind == MemberKind::Script) {
      if (const auto scriptName = identifierFromType(declaration.getScriptName());
          scriptName && namesMatch(queryName, *scriptName)) {
        found = declaration.getScriptNameLocation();
        return;
      }
    }

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
      if (found) {
        return;
      }
    }
  }

  void visitStateDeclaration(ast::StateDeclaration& declaration) override {
    if (queryKind == MemberKind::State &&
        namesMatch(queryName, VellumIdentifier(declaration.getStateName()))) {
      found = declaration.getStateNameLocation();
      return;
    }

    const Opt<VellumIdentifier> savedState = currentState;
    currentState = VellumIdentifier(declaration.getStateName());

    for (const auto& memberDecl : declaration.getMemberDecls()) {
      memberDecl->accept(*this);
      if (found) {
        currentState = savedState;
        return;
      }
    }

    currentState = savedState;
  }

  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override {
    if (currentState) {
      return;
    }
    if (queryKind != MemberKind::GlobalVariable ||
        !namesMatch(queryName, VellumIdentifier(declaration.name()))) {
      return;
    }
    if (const auto loc = declaration.getNameLocation()) {
      found = *loc;
    }
  }

  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override {
    if (queryKind != MemberKind::Function || !declaration.getName()) {
      return;
    }
    if (!namesMatch(queryName, VellumIdentifier(*declaration.getName()))) {
      return;
    }
    if (!stateMatches(queryState, currentState)) {
      return;
    }
    if (const auto loc = declaration.getNameLocation()) {
      found = *loc;
    }
  }

  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override {
    if (currentState) {
      return;
    }
    if (queryKind != MemberKind::Property ||
        !namesMatch(queryName, VellumIdentifier(declaration.getName()))) {
      return;
    }
    found = declaration.getNameLocation();
  }

 private:
  VellumIdentifier queryName;
  MemberKind queryKind;
  Opt<VellumIdentifier> queryState;
  Opt<VellumIdentifier> currentState;
  Opt<Token> found;
};

}  // namespace

Opt<Token> DeclarationIndex::findMemberDeclaration(const ParserResult& ast,
                                                 VellumIdentifier name,
                                                 MemberKind kind,
                                                 Opt<VellumIdentifier> state) {
  DeclarationIndexVisitor visitor(name, kind, state);
  auto& declarations =
      const_cast<common::Vec<common::Unique<ast::Declaration>>&>(ast.declarations);
  visitor.collect(declarations);
  return visitor.result();
}

}  // namespace vellum
