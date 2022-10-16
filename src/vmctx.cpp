#include <vmctx.hpp>

namespace vm {
vmctx_t::vmctx_t(std::uintptr_t module_base,
                 std::uintptr_t image_base,
                 std::uintptr_t image_size,
                 std::uintptr_t vm_entry_rva)
    : m_module_base(module_base),
      m_image_base(image_base),
      m_vm_entry_rva(vm_entry_rva),
      m_image_size(image_size),
      m_image_load_delta(m_module_base - m_image_base) {}

bool vmctx_t::init() {
  vm::utils::init();
  vm::instrs::init();

  // flatten and deobfuscate the vm entry...
  if (!vm::utils::flatten(m_vm_entry, m_module_base + m_vm_entry_rva))
    return false;

  vm::utils::deobfuscate(m_vm_entry);

  //Get the order in which native registers are pushed
  int push_index = 0;
  for (const auto& instr : m_vm_entry)
  {
    if (instr.instr.mnemonic == ZYDIS_MNEMONIC_PUSH &&
        instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
        vm::utils::is_64_bit_gp(instr.instr.operands[0].reg.value))
    {
      if (std::find(vmentry_push_order.begin(), vmentry_push_order.begin() + push_index,
          instr.instr.operands[0].reg.value) != vmentry_push_order.begin() + push_index)
      {
        //Every register should only be pushed once
        std::printf("Error initializing vmctx_t: vmenter pushes could not be parsed.\n");
        vm::utils::print(m_vm_entry);
        return false;
      }
      vmentry_push_order[push_index++] = instr.instr.operands[0].reg.value;
    }
    else if (instr.instr.mnemonic == ZYDIS_MNEMONIC_PUSHFQ)
    {
      if (std::find(vmentry_push_order.begin(), vmentry_push_order.begin() + push_index,
          instr.instr.operands[0].reg.value) != vmentry_push_order.begin() + push_index)
      {
        // Same shit
        std::printf("Error initializing vmctx_t: vmenter pushes could not be parsed.\n");
        vm::utils::print(m_vm_entry);
        return false;
      }
      vmentry_push_order[push_index++] = ZYDIS_REGISTER_RFLAGS;
    }
    if (push_index == 16)
      break;
  }
  
  // find mov reg, [rsp+0x90]. this register will be VIP...
  const auto vip_fetch = std::find_if(
      m_vm_entry.begin(), m_vm_entry.end(),
      [&](const zydis_instr_t& instr) -> bool {
        return instr.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.instr.operands[1].mem.base == ZYDIS_REGISTER_RSP &&
               instr.instr.operands[1].mem.disp.value == 0x90;
      });

  if (vip_fetch == m_vm_entry.end())
    return false;

  m_vip = vip_fetch->instr.operands[0].reg.value;

  // find the register that will be used for the virtual stack...
  // mov reg, rsp...
  const auto vsp_fetch = std::find_if(
      m_vm_entry.begin(), m_vm_entry.end(),
      [&](const zydis_instr_t& instr) -> bool {
        return instr.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.instr.operands[1].reg.value == ZYDIS_REGISTER_RSP;
      });

  if (vsp_fetch == m_vm_entry.end())
    return false;

  m_vsp = vsp_fetch->instr.operands[0].reg.value;
  return true;
}
const std::array<ZydisRegister, 16>& vm::vmctx_t::get_vmentry_push_order() const
{
  return vmentry_push_order;
}
}  // namespace vm