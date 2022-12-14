#include <string>
#include <vmlocate.hpp>

namespace vm::locate {
std::uintptr_t sigscan(void* base, std::uint32_t size, const char* pattern,
                       const char* mask) {
  static const auto check_mask = [&](const char* base, const char* pattern,
                                     const char* mask) -> bool {
    for (; *mask; ++base, ++pattern, ++mask)
      if (*mask == 'x' && *base != *pattern) return false;
    return true;
  };

  size -= std::strlen(mask);
  for (auto i = 0; i <= size; ++i) {
    void* addr = (void*)&(((char*)base)[i]);
    if (check_mask((char*)addr, pattern, mask))
      return reinterpret_cast<std::uintptr_t>(addr);
  }

  return {};
}

std::vector<vm_enter_t> get_vm_entries(std::uintptr_t module_base,
                                       std::uint32_t module_size) {
  std::uintptr_t result = module_base;
  std::vector<vm_enter_t> entries;

  static const auto push_regs = [&](const zydis_rtn_t& rtn) -> bool {
    for (unsigned reg = ZYDIS_REGISTER_RAX; reg < ZYDIS_REGISTER_R15; ++reg) {
      auto res = std::find_if(
          rtn.begin(), rtn.end(), [&](const zydis_instr_t& instr) -> bool {
            return instr.instr.mnemonic == ZYDIS_MNEMONIC_PUSH &&
                   instr.instr.operands[0].type ==
                       ZYDIS_OPERAND_TYPE_REGISTER &&
                   instr.instr.operands[0].reg.value == reg;
          });

      // skip RSP push...
      if (res == rtn.end() && reg != ZYDIS_REGISTER_RSP) return false;
    }
    return true;
  };

  do {
    result = sigscan((void*)++result, module_size - (result - module_base),
                     PUSH_4B_IMM, PUSH_4B_MASK);

    zydis_rtn_t rtn;
    if (!vm::utils::scn::executable(module_base, result)) continue;

    // Make sure that the form of the vmenter is a jmp immediately followed by a call imm
    ZydisDecodedInstruction after_push;
    if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(vm::utils::g_decoder.get(), 
                          (void*)(result + 5), 5, &after_push)))
    {
      if (after_push.mnemonic != ZYDIS_MNEMONIC_CALL ||
        after_push.operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
        continue;
    }
    else
      continue;

    if (!vm::utils::flatten(rtn, result, false, 500, module_base)) continue;

    // the last instruction in the stream should be a JMP to a register or a
    // return instruction...
    const auto& last_instr = rtn[rtn.size() - 1];
    if (!((last_instr.instr.mnemonic == ZYDIS_MNEMONIC_JMP &&
           last_instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER) ||
          last_instr.instr.mnemonic == ZYDIS_MNEMONIC_RET))
      continue;

    std::uint8_t num_pushs = 0u;
    std::for_each(rtn.begin(), rtn.end(), [&](const zydis_instr_t& instr) {
      if (instr.instr.mnemonic == ZYDIS_MNEMONIC_PUSH &&
          instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        ++num_pushs;
    });

    /*
    only one legit imm pushes for every vm entry...
    > 0x822c :                                    push 0xFFFFFFFF890001FA <---
    > 0x7fc9 :                                    call xxxxx
    > 0x48e4 :                                    push r13
    > 0x4690 :                                    push rsi
    > 0x4e53 :                                    push r14
    > 0x74fb :                                    push rcx
    > 0x607c :                                    push rsp
    > 0x4926 :                                    pushfq
    > 0x4dc2 :                                    push rbp
    > 0x5c8c :                                    push r12
    > 0x52ac :                                    push r10
    > 0x51a5 :                                    push r9
    > 0x5189 :                                    push rdx
    > 0x7d5f :                                    push r8
    > 0x4505 :                                    push rdi
    > 0x4745 :                                    push r11
    > 0x478b :                                    push rax
    > 0x7a53 :                                    push rbx
    > 0x500d :                                    push r15
    */
    if (num_pushs != 1) continue;

    // check for a pushfq...
    // > 0x4926 :                                    pushfq <---
    if (!vm::locate::find(rtn, [&](const zydis_instr_t& instr) -> bool {
          return instr.instr.mnemonic == ZYDIS_MNEMONIC_PUSHFQ;
        }))
      continue;

    /*
    check to see if we push all of these registers...
    > 0x48e4 :                                    push r13
    > 0x4690 :                                    push rsi
    > 0x4e53 :                                    push r14
    > 0x74fb :                                    push rcx
    > 0x607c :                                    push rsp
    > 0x4926 :                                    pushfq
    > 0x4dc2 :                                    push rbp
    > 0x5c8c :                                    push r12
    > 0x52ac :                                    push r10
    > 0x51a5 :                                    push r9
    > 0x5189 :                                    push rdx
    > 0x7d5f :                                    push r8
    > 0x4505 :                                    push rdi
    > 0x4745 :                                    push r11
    > 0x478b :                                    push rax
    > 0x7a53 :                                    push rbx
    > 0x500d :                                    push r15
    */
    if (!push_regs(rtn)) continue;

    // check for a mov reg, rsp
    if (!vm::locate::find(rtn, [&](const zydis_instr_t& instr) -> bool {
          return instr.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                 instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                 instr.instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                 instr.instr.operands[1].reg.value == ZYDIS_REGISTER_RSP;
        }))
      continue;

    // check for a mov reg, [rsp+0x90]
    if (!vm::locate::find(rtn, [&](const zydis_instr_t& instr) -> bool {
          return instr.instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
                 instr.instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                 instr.instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                 instr.instr.operands[1].mem.base == ZYDIS_REGISTER_RSP &&
                 instr.instr.operands[1].mem.disp.value == 0x90;
        }))
      continue;

    // check for invalid instructions... such as INT instructions...
    if (vm::locate::find(rtn, [&](const zydis_instr_t& instr) -> bool {
          const auto& i = instr.instr;
          return i.mnemonic >= ZYDIS_MNEMONIC_INT &&
                 i.mnemonic <= ZYDIS_MNEMONIC_INT3;
        }))
      continue;

    // if code execution gets to here then we can assume this is a legit vm
    // entry... its time to build a vm_enter_t... first we check to see if an
    // existing entry already exits...

    auto push_val = (std::uint32_t)rtn[0].instr.operands[0].imm.value.u;
    if (std::find_if(entries.begin(), entries.end(),
                     [&](const vm_enter_t& vm_enter) -> bool {
                       return vm_enter.encrypted_rva == push_val;
                     }) != entries.end())
      continue;

    vm_enter_t entry{(std::uint32_t)(result - module_base), push_val};
    entries.push_back(entry);
  } while (result);
  return entries;
}
}  // namespace vm::locate