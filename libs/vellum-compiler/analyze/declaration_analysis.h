#pragma once

#include <string_view>

#include "common/types.h"

namespace vellum {
using common::Shared;
using common::Unique;
using common::Vec;

class CompilerErrorHandler;
class Resolver;

namespace ast {
class Declaration;
}  // namespace ast

void collectDeclarations(Vec<Unique<ast::Declaration>>& declarations,
                        const Shared<CompilerErrorHandler>& errorHandler,
                        const Shared<Resolver>& resolver,
                        std::string_view scriptFilename);

}  // namespace vellum
