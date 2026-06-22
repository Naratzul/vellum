#pragma once

#include <optional>
#include <vector>

#include "common/types.h"
#include "vellum/vellum_type.h"
#include "vellum_function.h"
#include "vellum_identifier.h"
#include "vellum_property.h"
#include "vellum_state.h"
#include "vellum_value.h"
#include "vellum_variable.h"

namespace vellum {
using common::Opt;
using common::Vec;
class VellumObject {
 public:
  explicit VellumObject(VellumType type) : type(type) {}

  VellumType getType() const { return type; }

  const Vec<VellumFunction>& getFunctions() const { return functions; }
  void addFunction(VellumFunction function) {
    functions.push_back(std::move(function));
  }

  Opt<VellumFunction> getFunction(VellumIdentifier id) const;

  const Vec<VellumProperty>& getProperties() const { return properties; }
  void addProperty(VellumProperty property) {
    properties.push_back(std::move(property));
  }

  Opt<VellumProperty> getProperty(VellumIdentifier id) const;

  const Vec<VellumVariable>& getVariables() const { return variables; }
  void addVariable(VellumVariable variable) {
    variables.push_back(std::move(variable));
  }

  const Vec<VellumState>& getStates() const { return states; }
  void addState(VellumState state) { states.push_back(std::move(state)); }
  Opt<VellumState> getState(VellumIdentifier name) const;

  Opt<VellumValue> findIdentifier(VellumIdentifier identifier) const;
  Opt<VellumValue> findProperty(VellumIdentifier identifier) const;
  Opt<VellumFunction> findFunction(VellumIdentifier identifier) const;
  Opt<VellumVariable> findVariable(VellumIdentifier identifier) const;

 private:
  VellumType type;

  Vec<VellumFunction> functions;
  Vec<VellumProperty> properties;
  Vec<VellumVariable> variables;
  Vec<VellumState> states;
};
}  // namespace vellum