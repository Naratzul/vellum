#include "pex_function_parameter.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

PexWriter& vellum::pex::operator<<(PexWriter& writer,
                                   const PexFunctionParameter& param) {
  writer << param.getName() << param.getType();
  return writer;
}

}  // namespace pex
}  // namespace vellum
