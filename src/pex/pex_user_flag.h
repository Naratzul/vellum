#pragma once

namespace vellum {
namespace pex {
class PexWriter;

class UserFlags {};

PexWriter& operator<<(PexWriter& writer, const UserFlags& flags);

}  // namespace pex
}  // namespace vellum