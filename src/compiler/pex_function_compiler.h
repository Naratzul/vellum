#pragma once

#include <memory>
#include <vector>

#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "pex/pex_file.h"
#include "pex/pex_function_parameter.h"
#include "pex/pex_instruction.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Shared;
using common::Vec;

class CompilerErrorHandler;

namespace ast {
class CallExpression;
class FunctionDeclaration;
class Statement;
class SuperExpression;
}  // namespace ast

namespace pex {
class PexFunction;
}

class PexFunctionCompiler : public ast::StatementVisitor,
                            public ast::ExpressionCompiler {
 public:
  PexFunctionCompiler(Shared<CompilerErrorHandler> errorHandler,
                      pex::PexFile& file);

  pex::PexFunction compile(const ast::FunctionDeclaration& func);

  void visitExpressionStatement(ast::ExpressionStatement& statement) override;
  void visitReturnStatement(ast::ReturnStatement& statement) override;
  void visitIfStatement(ast::IfStatement& statement) override;
  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override;
  void visitWhileStatement(ast::WhileStatement& statement) override;

  pex::PexValue compile(const ast::LiteralExpression& expr) override;
  pex::PexValue compile(const ast::IdentifierExpression& expr) override;
  pex::PexValue compile(const ast::CallExpression& expr) override;
  pex::PexValue compile(const ast::PropertyGetExpression& expr) override;
  pex::PexValue compile(const ast::PropertySetExpression& expr) override;
  pex::PexValue compile(const ast::ArrayIndexExpression& expr) override;
  pex::PexValue compile(const ast::ArrayIndexSetExpression& expr) override;
  pex::PexValue compile(const ast::AssignExpression& expr) override;
  pex::PexValue compile(const ast::BinaryExpression& expr) override;
  pex::PexValue compile(const ast::UnaryExpression& expr) override;
  pex::PexValue compile(const ast::CastExpression& expr) override;
  pex::PexValue compile(const ast::NewArrayExpression& expr) override;
  pex::PexValue compile(const ast::SuperExpression& expr) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;

  Vec<pex::PexFunctionParameter> localVariables;
  Vec<pex::PexInstruction> instructions;
  int tempVarCount = 0;
  bool isNoneVarAdded = false;
  pex::PexIdentifier noneVar;

  pex::PexValue makeValueFromToken(VellumValue value);

  pex::PexTemporaryVariable makeTempVar(const pex::PexString& typeName);
  pex::PexIdentifier getNoneVar();
};
}  // namespace vellum