#include "pex_object_compiler.h"

#include "ast/decl/declaration.h"
#include "common/string_set.h"
#include "common/types.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "pex/pex_function.h"
#include "pex/pex_object.h"
#include "pex/pex_state.h"
#include "pex_function_compiler.h"

namespace vellum {
using common::makeUnique;
using common::Opt;
using common::Vec;

PexObjectCompiler::PexObjectCompiler(Shared<CompilerErrorHandler> errorHandler,
                                     pex::PexFile& file)
    : errorHandler(errorHandler), file(file) {}

pex::PexObject PexObjectCompiler::compile(
    const Vec<Unique<ast::Declaration>>& declarations) {
  object = pex::PexObject();

  pex::PexState rootState(file.getString(""));
  object.getStates().push_back(std::move(rootState));

  for (const auto& declaration : declarations) {
    declaration->accept(*this);
  }

  return object;
}

void PexObjectCompiler::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  object.setName(file.getString(declaration.scriptName()));
  if (declaration.parentScriptName().has_value()) {
    object.setParentName(
        file.getString(declaration.parentScriptName().value()));
  }

  object.setDocumentationString(file.getString(""));
  object.setAutoStateName(file.getString(""));
}

void PexObjectCompiler::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  const pex::PexString name = file.getString(declaration.name());

  assert(declaration.typeName().has_value());
  const pex::PexString typeName =
      file.getString(declaration.typeName().value().toString());

  const pex::PexValue defaultValue = makeValueFromToken(declaration.getValue());
  object.getVariables().emplace_back(name, typeName, defaultValue);
}

void PexObjectCompiler::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  PexFunctionCompiler compiler(errorHandler, file);
  pex::PexFunction function = compiler.compile(declaration);

  object.getRootState().getFunctions().push_back(std::move(function));
}

void PexObjectCompiler::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  const pex::PexString name = file.getString(declaration.getName());
  const pex::PexString typeName =
      file.getString(declaration.getTypeName().toString());
  const pex::PexString documentationString =
      file.getString(declaration.getDocumentationString());

  Opt<pex::PexFunction> getAccessorFunc;
  Opt<pex::PexFunction> setAccessorFunc;

  if (auto getAccessor = declaration.getGetAccessor();
      !declaration.isAutoProperty()) {
    ast::FunctionBody body;
    if (!getAccessor->empty()) {
      body = std::move(getAccessor.value());
    } else {
      body.push_back(
          makeUnique<ast::ReturnStatement>(makeUnique<ast::LiteralExpression>(
              declaration.getDefaultValue().asLiteral())));
    }

    ast::FunctionDeclaration funcDecl({}, {}, declaration.getTypeName(),
                                      std::move(body), false);
    getAccessorFunc = PexFunctionCompiler(errorHandler, file).compile(funcDecl);
  }

  if (auto setAccessor = declaration.getSetAccessor()) {
    if (!setAccessor.value().empty()) {
      ast::FunctionDeclaration funcDecl({}, {}, VellumType::none(),
                                        std::move(setAccessor.value()), false);
      setAccessorFunc =
          PexFunctionCompiler(errorHandler, file).compile(funcDecl);
    }
  }

  Opt<pex::PexString> backedVariableName;
  if (declaration.isAutoProperty()) {
    const std::string_view varName = common::StringSet::insert(
        "::" + std::string(declaration.getName()) + "_var");
    pex::PexVariable backedVariable(
        file.getString(varName), typeName,
        makeValueFromToken(declaration.getDefaultValue()));
    backedVariableName = backedVariable.name();
    object.getVariables().push_back(backedVariable);
  }

  object.getProperties().emplace_back(name, typeName, documentationString,
                                      pex::PexUserFlags(), getAccessorFunc,
                                      setAccessorFunc, backedVariableName);
}

pex::PexValue PexObjectCompiler::makeValueFromToken(VellumValue value) {
  Opt<pex::PexValue> pexValue = makePexValue(value, file);
  if (!pexValue) {
    errorHandler->errorAt(Token(), "Unexpected variable initializer type.");
    return pex::PexValue();
  }
  return pexValue.value();
}

}  // namespace vellum