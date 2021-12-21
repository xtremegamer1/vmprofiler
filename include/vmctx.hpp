#pragma once
#include <vmutils.hpp>

namespace vm {
struct ctx_t {
  const std::uintptr_t module_base, image_base, vm_entry_rva, image_size;
};
}  // namespace vm