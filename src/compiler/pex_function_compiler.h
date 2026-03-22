#pragma once

#include <memory>
#include <vector>

#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "lexer/token.h"
#include "pex/pex_debug_function_info.h"
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
class SelfExpression;
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

  pex::PexFunction compile(const ast::FunctionDeclaration& func,
                            pex::PexDebugFunctionInfo* debugInfo = nullptr);

  void visitExpressionStatement(ast::ExpressionStatement& statement) override;
  void visitReturnStatement(ast::ReturnStatement& statement) override;
  void visitIfStatement(ast::IfStatement& statement) override;
  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override;
  void visitWhileStatement(ast::WhileStatement& statement) override;
  void visitBreakStatement(ast::BreakStatement& statement) override;
  void visitContinueStatement(ast::ContinueStatement& statement) override;
  void visitForStatement(ast::ForStatement& statement) override;

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
  pex::PexValue compile(const ast::SelfExpression& expr) override;
  pex::PexValue compile(const ast::SuperExpression& expr) override;
  pex::PexValue compile(const ast::TernaryExpression& expr) override;

 private:
  void setCurrentLine(int line) { currentLine_ = line; }
  void setCurrentLocation(const Token& token) {
    currentLine_ = token.location.start.line;
  }
  void recordInstructionLine();
  void emitInstruction(pex::PexOpCode opcode, Vec<pex::PexValue> args);
  void emitInstruction(pex::PexOpCode opcode, Vec<pex::PexValue> args,
                       Vec<pex::PexValue> variadicArgs);

  Shared<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;
  pex::PexDebugFunctionInfo* debugInfo_ = nullptr;
  int currentLine_ = 1;

  Vec<pex::PexFunctionParameter> localVariables;
  Vec<pex::PexInstruction> instructions;
  int tempVarCount = 0;
  bool isNoneVarAdded = false;
  pex::PexIdentifier noneVar;

  Vec<size_t> breakInstructions;
  Vec<size_t> continueInstructions;

  pex::PexValue makeValueFromToken(VellumValue value);

  pex::PexIdentifier makeTempVarId(const VellumType& type);
  pex::PexTemporaryVariable makeTempVar(const VellumType& type);
  pex::PexTemporaryVariable makeTempVar(const pex::PexString& typeName);
  std::string_view makeTempVarName();
  pex::PexIdentifier getNoneVar();
};
}  // namespace vellum