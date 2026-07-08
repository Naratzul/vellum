#pragma once

#include "common/types.h"
#include "compiler/resolver.h"
#include "lexer/token.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_identifier.h"

namespace vellum {
using common::Opt;
using common::Vec;

class CompilerErrorHandler;

enum class BareCalleeStatus { Local, Imported, Ambiguous, NotFound };

struct BareCalleeResolution {
  BareCalleeStatus status;
  Opt<VellumFunctionCall> call;
  Opt<VellumFunction> function;
  Vec<ImportedFunction> ambiguous;
};

struct BareCallableCandidate {
  VellumType owner;
  VellumFunction function;
  bool isLocal;
};

BareCalleeResolution resolveBareCallee(const Resolver& resolver,
                                       VellumIdentifier name);

Opt<VellumFunction> resolveImportedFunctionIdentifier(const Resolver& resolver,
                                                      VellumIdentifier name);

Vec<BareCallableCandidate> collectBareCallableCandidates(
    const Resolver& resolver);

void reportAmbiguousImportCall(CompilerErrorHandler& errorHandler,
                               const Token& location,
                               VellumIdentifier functionName,
                               const Vec<ImportedFunction>& imports);

}  // namespace vellum
