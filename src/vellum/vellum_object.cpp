#include "vellum_object.h"

#include "common/types.h"

namespace vellum {
using common::Opt;

Opt<VellumValue> VellumObject::findIdentifier(
    VellumIdentifier identifier) const {
  if (auto member = findVariable(identifier)) {
    return member;
  }

  return findProperty(identifier);
}

Opt<VellumValue> VellumObject::findProperty(
    VellumIdentifier identifier) const {
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