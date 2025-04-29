#include "pex_function.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {
PexWriter& operator<<(PexWriter& writer, const PexFunction& fun) {
  if (auto name = fun.getName()) {
    assert(name.value().isValid());
    writer << name.value();
  }
  writer << fun.getReturnTypeName() << fun.getDocumentationString()
         << fun.getUserFlags();
  uint8_t flags = 0;
  writer << flags;

  writer << fun.getParameters();
  writer << fun.getLocalVariables();
  writer << fun.getInstructions();

  return writer;
}
}  // namespace pex
}  // namespace vellum