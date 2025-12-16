#pragma once

#include <optional>
#include <vector>

#include "vellum_function.h"
#include "vellum_identifier.h"
#include "vellum_property.h"
#include "vellum_value.h"
#include "vellum_variable.h"

namespace vellum {
class VellumObject {
 public:
  explicit VellumObject(VellumType type) : type(type) {}

  VellumType getType() const { return type; }

  const std::vector<VellumFunction>& getFunctions() const { return functions; }
  void addFunction(VellumFunction function) {
    functions.push_back(std::move(function));
  }

  const std::vector<VellumProperty>& getProperties() const {
    return properties;
  }
  void addProperty(VellumProperty property) {
    properties.push_back(std::move(property));
  }

  const std::vector<VellumVariable>& getVariables() const { return variables; }
  void addVariable(VellumVariable variable) {
    variables.push_back(std::move(variable));
  }

  std::optional<VellumValue> findIdentifier(VellumIdentifier identifier) const;
  std::optional<VellumValue> findProperty(VellumIdentifier identifier) const;
  std::optional<VellumFunction> findFunction(VellumIdentifier identifier) const;
  std::optional<VellumVariable> findVariable(VellumIdentifier identifier) const;

 private:
  VellumType type;

  std::vector<VellumFunction> functions;
  std::vector<VellumProperty> properties;
  std::vector<VellumVariable> variables;
};
}  // namespace vellum