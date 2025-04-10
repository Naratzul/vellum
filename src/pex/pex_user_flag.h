#pragma once

#include "pex_writer.h"

namespace vellum {
namespace pex {
class UserFlags {
};

PexWriter& operator<<(PexWriter& writer, const UserFlags& flags);

}  // namespace pex
}  // namespace vellum