#include <vmctx.hpp>

namespace vm {
vmctx_t::vmctx_t(std::uintptr_t module_base,
                 std::uintptr_t image_base,
                 std::uintptr_t vm_entry_rva,
                 std::uintptr_t image_size)
    : m_module_base(module_base),
      m_image_base(image_base),
      m_vm_entry_rva(vm_entry_rva),
      m_image_size(image_size) {}
}  // namespace vm