#pragma once

namespace vellum {

namespace ast {

class FunctionDeclaration;
class GlobalVariableDeclaration;
class ScriptDeclaration;
class PropertyDeclaration;

class DeclarationVisitor {
 public:
  virtual void visitScriptDeclaration(ScriptDeclaration& declaration) = 0;
  virtual void visitVariableDeclaration(
      GlobalVariableDeclaration& declaration) = 0;
  virtual void visitFunctionDeclaration(FunctionDeclaration& declaration) = 0;
  virtual void visitPropertyDeclaration(PropertyDeclaration& declaration) = 0;
};

}  // namespace ast
}  // namespace vellum