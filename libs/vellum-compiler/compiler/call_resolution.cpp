#include "call_resolution.h"

#include "compiler/compiler_error_handler.h"

namespace vellum {
using common::Opt;
using common::Vec;

namespace {

std::string formatImportedTypeNames(const Vec<ImportedFunction>& imports) {
  std::string names;
  for (size_t i = 0; i < imports.size(); ++i) {
    if (i > 0) {
      names += ", ";
    }
    names += imports[i].object.toString();
  }
  return names;
}

bool isShadowedByLocalOrParent(const Resolver& resolver,
                               VellumIdentifier name) {
  return resolver.resolveFunction(resolver.getObjectType(), name).has_value();
}

}  // namespace

void reportAmbiguousImportCall(CompilerErrorHandler& errorHandler,
                               const Token& location,
                               VellumIdentifier functionName,
                               const Vec<ImportedFunction>& imports) {
  errorHandler.errorAt(
      location, CompilerErrorKind::AmbiguousImportCall,
      "Ambiguous call to '{}': found in multiple imports ({}). Use "
      "Type.{}() to disambiguate.",
      functionName.toString(), formatImportedTypeNames(imports),
      functionName.toString());
}

BareCalleeResolution resolveBareCallee(const Resolver& resolver,
                                       VellumIdentifier name) {
  if (const auto func = resolver.resolveFunction(resolver.getObjectType(),
                                                 name)) {
    BareCalleeResolution result;
    result.status = BareCalleeStatus::Local;
    result.function = *func;
    if (func->isStatic()) {
      result.call = VellumFunctionCall::staticCall(resolver.getObjectType(),
                                                   name);
    } else {
      result.call = VellumFunctionCall::methodCall(
          VellumIdentifier("self"), resolver.getObjectType(), name);
    }
    return result;
  }

  const Vec<ImportedFunction> imported =
      resolver.resolveImportedFunction(name);
  if (imported.size() > 1) {
    return {.status = BareCalleeStatus::Ambiguous,
            .call = std::nullopt,
            .function = std::nullopt,
            .ambiguous = imported};
  }
  if (imported.empty()) {
    return {.status = BareCalleeStatus::NotFound,
            .call = std::nullopt,
            .function = std::nullopt,
            .ambiguous = {}};
  }

  return {.status = BareCalleeStatus::Imported,
          .call = VellumFunctionCall::staticCall(imported[0].object, name),
          .function = imported[0].function,
          .ambiguous = {}};
}

Opt<VellumFunction> resolveImportedFunctionIdentifier(const Resolver& resolver,
                                                      VellumIdentifier name) {
  const Vec<ImportedFunction> imported =
      resolver.resolveImportedFunction(name);
  if (imported.size() != 1) {
    return std::nullopt;
  }
  return imported[0].function;
}

Vec<BareCallableCandidate> collectBareCallableCandidates(
    const Resolver& resolver) {
  Vec<BareCallableCandidate> result;

  for (const auto& func : resolver.getObject().getFunctions()) {
    if (!func.isStatic()) {
      continue;
    }
    result.push_back(
        {.owner = resolver.getObjectType(), .function = func, .isLocal = true});
  }

  for (const auto& importName : resolver.getImportedObjects()) {
    const Resolver* moduleResolver = resolver.findScriptResolver(importName);
    if (!moduleResolver) {
      continue;
    }

    const VellumType ownerType = moduleResolver->getObjectType();
    for (const auto& func : moduleResolver->getObject().getFunctions()) {
      if (!func.isStatic()) {
        continue;
      }
      if (isShadowedByLocalOrParent(resolver, func.getName())) {
        continue;
      }
      result.push_back({.owner = ownerType, .function = func, .isLocal = false});
    }
  }

  return result;
}

}  // namespace vellum
