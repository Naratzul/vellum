#include "builtin_functions.h"

#include "vellum/vellum_identifier.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_variable.h"

namespace vellum {
using common::Opt;
using common::Vec;

BuiltinFunctions::BuiltinFunctions() {
  // GetState() -> String
  VellumFunction getState(VellumIdentifier("GetState"),
                          VellumType::literal(VellumLiteralType::String), {},
                          false);
  functions.push_back(getState);

  // GoToState(name: String)
  VellumFunction goToState(
      VellumIdentifier("GoToState"), VellumType::none(),
      {VellumVariable(VellumIdentifier("name"),
                      VellumType::literal(VellumLiteralType::String))},
      false);
  functions.push_back(goToState);
}

void BuiltinFunctions::addFunction(VellumFunction func) {
  functions.push_back(std::move(func));
}

Opt<VellumFunction> BuiltinFunctions::getFunction(VellumIdentifier name) const {
  for (const auto& func : functions) {
    if (func.getName() == name) {
      return func;
    }
  }
  return std::nullopt;
}
}  // namespace vellum
