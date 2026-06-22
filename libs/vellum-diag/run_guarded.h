#pragma once

#include <cpptrace/from_current.hpp>
#include <utility>

namespace vellum::diag {

template <class Body, class OnStdException, class OnUnknown>
void runGuarded(Body&& body, OnStdException&& onStd, OnUnknown&& onUnknown) {
  cpptrace::try_catch(std::forward<Body>(body),
                      std::forward<OnStdException>(onStd),
                      std::forward<OnUnknown>(onUnknown));
}

}  // namespace vellum::diag
