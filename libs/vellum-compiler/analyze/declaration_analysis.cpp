#include "declaration_analysis.h"

#include "analyze/declaration_collector.h"
#include "analyze/modifier_validator.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"

namespace vellum {
void collectDeclarations(Vec<Unique<ast::Declaration>>& declarations,
                         const Shared<CompilerErrorHandler>& errorHandler,
                         const Shared<Resolver>& resolver,
                         std::string_view scriptFilename) {
  ModifierValidator modifierValidator(errorHandler);
  modifierValidator.validate(declarations);

  DeclarationCollector collector(errorHandler, resolver, scriptFilename);
  collector.collect(declarations);
}
}  // namespace vellum
