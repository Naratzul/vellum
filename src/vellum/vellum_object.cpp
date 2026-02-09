#include "vellum_object.h"

#include <optional>

#include "common/types.h"

namespace vellum {
using common::Opt;

Opt<VellumFunction> VellumObject::getFunction(VellumIdentifier id) const {
  for (const auto& func : functions) {
    if (func.getName() == id) {
      return func;
    }
  }
  return std::nullopt;
}

Opt<VellumProperty> VellumObject::getProperty(VellumIdentifier id) const {
  for (const auto& prop : properties) {
    if (prop.getName() == id) {
      return prop;
    }
  }
  return std::nullopt;
}

Opt<VellumState> VellumObject::getState(VellumIdentifier name) const {
  for (const auto& state : states) {
    if (state.getName() == name) {
      return state;
    }
  }
  return std::nullopt;
}

Opt<VellumValue> VellumObject::findIdentifier(
    VellumIdentifier identifier) const {
  if (auto member = findVariable(identifier)) {
    return member;
  }

  return findProperty(identifier);
}

Opt<VellumValue> VellumObject::findProperty(VellumIdentifier identifier) const {
  for (const auto& member : properties) {
    if (member.getName() == identifier) {
      return member;
    }
  }

  return findFunction(identifier);
}

Opt<VellumFunction> VellumObject::findFunction(
    VellumIdentifier identifier) const {
  for (const auto& member : functions) {
    if (member.getName() == identifier) {
      return member;
    }
  }

  return std::nullopt;
}

Opt<VellumVariable> VellumObject::findVariable(
    VellumIdentifier identifier) const {
  for (const auto& member : variables) {
    if (member.getName() == identifier) {
      return member;
    }
  }

  return std::nullopt;
}

}  // namespace vellum