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
                            public ast::ExpressionVisitor {
 public:
  PexFunctionCompiler(std::shared_ptr<CompilerErrorHandler> errorHandler,
                      pex::PexFile& file);

  pex::PexFunction compile(const ast::FunctionDeclaration& func);

  void visitExpressionStatement(ast::ExpressionStatement& statement) override;
  void visitReturnStatement(ast::ReturnStatement& statement) override;

  void visitCallExpression(ast::CallExpression& expr) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;

  std::vector<pex::PexFunctionParameter> localVariables;
  std::vector<pex::PexInstruction> instructions;

  pex::PexValue makeValueFromToken(VellumValue value);
};
}  // namespace vellum