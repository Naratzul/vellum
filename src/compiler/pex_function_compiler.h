#pragma once

#include <memory>
#include <vector>

#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "pex/pex_file.h"
#include "pex/pex_function_parameter.h"
#include "pex/pex_instruction.h"
#include "vellum/vellum_value.h"

namespace vellum {

class CompilerErrorHandler;

namespace ast {
class CallExpression;
class FunctionDeclaration;
class Statement;
}  // namespace ast

namespace pex {
class PexFunction;
}

class PexFunctionCompiler : public ast::StatementVisitor,
                            public ast::ExpressionVisitorConst {
 public:
  PexFunctionCompiler(std::shared_ptr<CompilerErrorHandler> errorHandler,
                      pex::PexFile& file);

  pex::PexFunction compile(const ast::FunctionDeclaration& func);

  void visitExpressionStatement(ast::ExpressionStatement& statement) override;
  void visitReturnStatement(ast::ReturnStatement& statement) override;

  void visitCallExpression(const ast::CallExpression& expr) override;
  void visitGetExpression(const ast::GetExpression& expr) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;

  std::vector<pex::PexFunctionParameter> localVariables;
  std::vector<pex::PexInstruction> instructions;
  int tempVarCount = 0;

  pex::PexValue makeValueFromToken(VellumValue value);

  pex::PexTemporaryVariable makeTempVar(const pex::PexString& typeName);
};
}  // namespace vellum