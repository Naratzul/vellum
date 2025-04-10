#pragma once

namespace vellum {
namespace pex {
class PexString {
 public:
  PexString() = default;
  explicit PexString(int index) : index_(index) {}

  bool isValid() const { return index_ != -1; }

  int index() const { return index_; }

  bool operator==(const PexString& other) const {
    return index_ == other.index_;
  }
  bool operator!=(const PexString& other) const {
    return !(index_ == other.index_);
  }

 private:
  int index_ = -1;
};
}  // namespace pex
}  // namespace vellum