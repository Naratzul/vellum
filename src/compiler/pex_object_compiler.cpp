#include "pex_object_compiler.h"

#include <cassert>

#include "ast/decl/declaration.h"
#include "common/string_set.h"
#include "common/types.h"
#include "compiler_error_handler.h"
#include "pex/pex_debug_function_info.h"
#include "pex/pex_file.h"
#include "pex/pex_function.h"
#include "pex/pex_object.h"
#include "pex/pex_state.h"
#include "pex/pex_variable.h"
#include "pex_function_compiler.h"
#include "vellum/vellum_literal.h"

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
  currentStateIndex = 0;

  for (const auto& declaration : declarations) {
    declaration->accept(*this);
  }

  return object;
}

void PexObjectCompiler::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  object.setName(file.getString(declaration.getScriptName().toString()));
  object.setParentName(
      file.getString(declaration.getParentScriptName().toString()));
  object.setDocumentationString(file.getString(""));
  object.setAutoStateName(file.getString(""));

  for (auto& member : declaration.getMemberDecls()) {
    member->accept(*this);
  }
}

void PexObjectCompiler::visitStateDeclaration(
    ast::StateDeclaration& declaration) {
  currentStateIndex = object.getStates().size();

  auto stateName = file.getString(declaration.getStateName());
  object.getStates().emplace_back(stateName);

  if (declaration.getIsAuto()) {
    object.setAutoStateName(stateName);
  }

  for (auto& member : declaration.getMemberDecls()) {
    assert(member->getOrder() == ast::DeclarationOrder::Function);
    member->accept(*this);
  }

  currentStateIndex = 0;
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
  pex::PexDebugFunctionInfo* debugFuncInfo = nullptr;
  pex::PexDebugFunctionInfo debugFuncInfoStorage;
  if (file.hasDebugInfo()) {
    debugFuncInfoStorage.objectName = object.getName();
    debugFuncInfoStorage.stateName =
        object.getStates()[currentStateIndex].getName();
    debugFuncInfoStorage.functionName =
        declaration.getName().has_value()
            ? file.getString(declaration.getName().value())
            : file.getString("");
    debugFuncInfoStorage.functionType =
        pex::PexDebugFunctionType::Normal;
    debugFuncInfo = &debugFuncInfoStorage;
  }
  pex::PexFunction function = compiler.compile(declaration, debugFuncInfo);
  if (file.hasDebugInfo()) {
    file.debugInfo()->functions.push_back(std::move(debugFuncInfoStorage));
  }
  object.getStates()[currentStateIndex].getFunctions().push_back(
      std::move(function));
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
  Opt<pex::PexString> backedVariableName;

  if (declaration.isAutoProperty()) {
    auto backedVariable = makePropertyBackedVariable(declaration);
    backedVariableName = backedVariable.name();
    object.getVariables().push_back(std::move(backedVariable));
  } else if (declaration.isAutoReadonly()) {
    VellumType type = declaration.getTypeName();
    auto defaultValueType =
        type.getState() == VellumTypeState::Literal ? type : VellumType::none();

    ast::FunctionBody body;
    body.push_back(
        makeUnique<ast::ReturnStatement>(makeUnique<ast::LiteralExpression>(
            declaration.getDefaultValue().value_or(
                makeDefaultLiteral(defaultValueType.asLiteralType())))));

    ast::FunctionDeclaration funcDecl(
        {}, {}, declaration.getTypeName(),
        makeUnique<ast::BlockStatement>(std::move(body)), false);
    pex::PexDebugFunctionInfo* getterDebugInfo = nullptr;
    pex::PexDebugFunctionInfo getterDebugInfoStorage;
    if (file.hasDebugInfo()) {
      getterDebugInfoStorage.objectName = object.getName();
      getterDebugInfoStorage.stateName = object.getRootState().getName();
      getterDebugInfoStorage.functionName = name;
      getterDebugInfoStorage.functionType = pex::PexDebugFunctionType::Getter;
      getterDebugInfo = &getterDebugInfoStorage;
    }
    getAccessorFunc =
        PexFunctionCompiler(errorHandler, file).compile(funcDecl, getterDebugInfo);
    if (file.hasDebugInfo()) {
      file.debugInfo()->functions.push_back(std::move(getterDebugInfoStorage));
    }
  } else {
    pex::PexDebugFunctionInfo* getterDebugInfo = nullptr;
    pex::PexDebugFunctionInfo getterDebugInfoStorage;
    pex::PexDebugFunctionInfo* setterDebugInfo = nullptr;
    pex::PexDebugFunctionInfo setterDebugInfoStorage;
    if (file.hasDebugInfo()) {
      getterDebugInfoStorage.objectName = object.getName();
      getterDebugInfoStorage.stateName = object.getRootState().getName();
      getterDebugInfoStorage.functionName = name;
      getterDebugInfoStorage.functionType = pex::PexDebugFunctionType::Getter;
      getterDebugInfo = &getterDebugInfoStorage;
      setterDebugInfoStorage.objectName = object.getName();
      setterDebugInfoStorage.stateName = object.getRootState().getName();
      setterDebugInfoStorage.functionName = name;
      setterDebugInfoStorage.functionType = pex::PexDebugFunctionType::Setter;
      setterDebugInfo = &setterDebugInfoStorage;
    }
    auto getBlock = declaration.releaseGetAccessor().value();
    ast::FunctionDeclaration getFuncDecl({}, {}, VellumType::none(),
                                         std::move(getBlock), false);
    getAccessorFunc = PexFunctionCompiler(errorHandler, file).compile(
        getFuncDecl, getterDebugInfo);
    if (file.hasDebugInfo()) {
      file.debugInfo()->functions.push_back(std::move(getterDebugInfoStorage));
    }

    auto setBlock = declaration.releaseSetAccessor().value();
    ast::FunctionDeclaration setFuncDecl({}, {}, VellumType::none(),
                                         std::move(setBlock), false);
    setAccessorFunc = PexFunctionCompiler(errorHandler, file).compile(
        setFuncDecl, setterDebugInfo);
    if (file.hasDebugInfo()) {
      file.debugInfo()->functions.push_back(std::move(setterDebugInfoStorage));
    }
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

pex::PexVariable PexObjectCompiler::makePropertyBackedVariable(
    const ast::PropertyDeclaration& declaration) {
  const pex::PexString typeName =
      file.getString(declaration.getTypeName().toString());
  const std::string_view varName = common::StringSet::insert(
      "::" + std::string(declaration.getName()) + "_var");

  VellumType type = declaration.getTypeName();
  auto defaultValueType =
      type.getState() == VellumTypeState::Literal ? type : VellumType::none();

  pex::PexVariable backedVariable(
      file.getString(varName), typeName,
      makeValueFromToken(declaration.getDefaultValue().value_or(
          makeDefaultLiteral(defaultValueType.asLiteralType()))));

  return backedVariable;
}

}  // namespace vellum