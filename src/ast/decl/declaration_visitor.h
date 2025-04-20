#pragma once

namespace vellum {

namespace ast {

class FunctionDeclaration;
class ScriptDeclaration;
class GlobalVariableDeclaration;

class DeclarationVisitor {
 public:
  virtual void visitScriptDeclaration(ScriptDeclaration& statement) = 0;
  virtual void visitVariableDeclaration(
      GlobalVariableDeclaration& statement) = 0;
  virtual void visitFunctionDeclaration(FunctionDeclaration& statement) = 0;
};

}  // namespace ast
}  // namespace vellum