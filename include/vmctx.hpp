#pragma once
#include <vmutils.hpp>

namespace vm {
class vmctx_t {
 public:
  explicit vmctx_t(std::uintptr_t module_base,
                   std::uintptr_t image_base,
                   std::uintptr_t image_size,
                   std::uintptr_t vm_entry_rva);
  bool init();
  const std::uintptr_t m_module_base, m_image_base, m_vm_entry_rva,
      m_image_size;

  /// <summary>
  /// m_vip and m_vsp are volitile and are subject to change... they are set to
  /// the ones used in vm enter but can be changed by external source code...
  /// </summary>
  zydis_register_t m_vip, m_vsp;

  /// <summary>
  /// the virtual machine enter flattened and deobfuscated...
  /// </summary>
  zydis_routine_t m_vm_entry;
};
}  // namespace vm