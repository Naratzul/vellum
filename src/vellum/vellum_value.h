#pragma once

#include <cassert>
#include <optional>
#include <ostream>
#include <string_view>
#include <variant>

#include "pex/pex_value.h"
#include "vellum_function.h"
#include "vellum_identifier.h"
#include "vellum_literal.h"
#include "vellum_property.h"
#include "vellum_variable.h"

namespace vellum {

namespace pex {
class PexFile;
}

enum class VellumValueType {
  None,
  Function,
  FunctionCall,
  Identifier,
  Literal,
  Property,
  PropertyAccess,
  Variable,
  ScriptType
};

class VellumValue {
 public:
  VellumValue() = default;
  VellumValue(VellumFunction value)
      : value(value), type(VellumValueType::Function) {}
  VellumValue(VellumFunctionCall value)
      : value(value), type(VellumValueType::FunctionCall) {}
  VellumValue(VellumLiteral value)
      : value(value), type(VellumValueType::Literal) {}
  VellumValue(VellumIdentifier value)
      : value(value), type(VellumValueType::Identifier) {}
  VellumValue(VellumProperty value)
      : value(value), type(VellumValueType::Property) {}
  VellumValue(VellumPropertyAccess value)
      : value(value), type(VellumValueType::PropertyAccess) {}
  VellumValue(VellumVariable value)
      : value(value), type(VellumValueType::Variable) {}
  VellumValue(VellumType value)
      : value(value.asIdentifier()), type(VellumValueType::ScriptType) {}

  VellumValueType getType() const { return type; }

  VellumFunction asFunction() const {
    assert(type == VellumValueType::Function);
    return std::get<VellumFunction>(value);
  }

  VellumFunctionCall asFunctionCall() const {
    assert(type == VellumValueType::FunctionCall);
    return std::get<VellumFunctionCall>(value);
  }

  VellumLiteral asLiteral() const {
    assert(type == VellumValueType::Literal);
    return std::get<VellumLiteral>(value);
  }

  VellumIdentifier asIdentifier() const {
    assert(type == VellumValueType::Identifier);
    return std::get<VellumIdentifier>(value);
  }

  VellumProperty asProperty() const {
    assert(type == VellumValueType::Property);
    return std::get<VellumProperty>(value);
  }

  VellumPropertyAccess asPropertyAccess() const {
    assert(type == VellumValueType::PropertyAccess);
    return std::get<VellumPropertyAccess>(value);
  }

  VellumVariable asVariable() const {
    assert(type == VellumValueType::Variable);
    return std::get<VellumVariable>(value);
  }

  VellumType asScriptType() const {
    assert(type == VellumValueType::ScriptType);
    return VellumType::identifier(std::get<VellumIdentifier>(value));
  }

 private:
  std::variant<std::monostate, VellumFunction, VellumFunctionCall,
               VellumLiteral, VellumIdentifier, VellumProperty,
               VellumPropertyAccess, VellumVariable>
      value;
  VellumValueType type = VellumValueType::None;
};

pex::PexValue makePexValue(VellumValue value, pex::PexFile& file);

bool operator==(const VellumValue& lhs, const VellumValue& rhs);
bool operator!=(const VellumValue& lhs, const VellumValue& rhs);
std::ostream& operator<<(std::ostream& os, const VellumValue& value);
}  // namespace vellum