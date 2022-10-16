#include <vmutils.hpp>

namespace vm::utils {
void print(const zydis_decoded_instr_t& instr) {
  char buffer[256];
  ZydisFormatterFormatInstruction(vm::utils::g_formatter.get(), &instr, buffer,
                                  sizeof(buffer), 0u);
  std::puts(buffer);
}

void print(zydis_rtn_t& routine) {
  char buffer[256];
  for (auto [instr, raw, addr] : routine) {
    ZydisFormatterFormatInstruction(vm::utils::g_formatter.get(), &instr,
                                    buffer, sizeof(buffer), addr);
    std::printf("> %p %s\n", addr, buffer);
  }
}

bool is_jmp(const zydis_decoded_instr_t& instr) {
  return instr.mnemonic >= ZYDIS_MNEMONIC_JB &&
         instr.mnemonic <= ZYDIS_MNEMONIC_JZ;
}

bool is_mov(const zydis_decoded_instr_t& instr) {
  return instr.mnemonic == ZYDIS_MNEMONIC_MOV ||
         instr.mnemonic == ZYDIS_MNEMONIC_MOVSX ||
         instr.mnemonic == ZYDIS_MNEMONIC_MOVZX;
}

bool is_32_bit_gp(const ZydisRegister reg)
{
  return reg >= ZYDIS_REGISTER_EAX && reg <= ZYDIS_REGISTER_R15D;
}

bool is_64_bit_gp(const ZydisRegister reg)
{
  return reg >= ZYDIS_REGISTER_RAX && reg <= ZYDIS_REGISTER_R15;
}

bool flatten(zydis_rtn_t& routine,
             std::uintptr_t routine_addr,
             bool keep_jmps,
             std::uint32_t max_instrs,
             std::uintptr_t module_base) {
  zydis_decoded_instr_t instr;
  std::uint32_t instr_cnt = 0u;

  while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(
      vm::utils::g_decoder.get(), reinterpret_cast<void*>(routine_addr), 0x1000,
      &instr))) {
    if (++instr_cnt > max_instrs)
      return false;
    // detect if we have already been at this instruction... if so that means
    // there is a loop and we are going to just return...
    if (std::find_if(routine.begin(), routine.end(),
                     [&](const zydis_instr_t& zydis_instr) -> bool {
                       return zydis_instr.addr == routine_addr;
                     }) != routine.end())
      return true;

    std::vector<u8> raw_instr;
    raw_instr.insert(raw_instr.begin(), (u8*)routine_addr,
                     (u8*)routine_addr + instr.length);

    if (is_jmp(instr) ||
        instr.mnemonic == ZYDIS_MNEMONIC_CALL &&
            instr.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER) {
      if (instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER) {
        routine.push_back({instr, raw_instr, routine_addr});
        return true;
      }

      if (keep_jmps)
        routine.push_back({instr, raw_instr, routine_addr});
      ZydisCalcAbsoluteAddress(&instr, &instr.operands[0], routine_addr,
                               &routine_addr);
    } else if (instr.mnemonic == ZYDIS_MNEMONIC_RET) {
      routine.push_back({instr, raw_instr, routine_addr});
      return true;
    } else {
      routine.push_back({instr, raw_instr, routine_addr});
      routine_addr += instr.length;
    }

    // optional sanity checking...
    if (module_base && !vm::utils::scn::executable(module_base, routine_addr))
      return false;
  }
  return false;
}

void deobfuscate(zydis_rtn_t& routine) {
  static const auto _uses_reg = [](zydis_decoded_operand_t& op,
                                   zydis_reg_t reg) -> bool {
    switch (op.type) {
      case ZYDIS_OPERAND_TYPE_MEMORY: {
        return vm::utils::reg::compare(op.mem.base, reg) ||
               vm::utils::reg::compare(op.mem.index, reg);
      }
      case ZYDIS_OPERAND_TYPE_REGISTER: {
        return vm::utils::reg::compare(op.reg.value, reg);
      }
      default:
        break;
    }
    return false;
  };

  static const auto _reads = [](zydis_decoded_instr_t& instr,
                                zydis_reg_t reg) -> bool {
    for (auto op_idx = 0u; op_idx < instr.operand_count; ++op_idx)
      if ((instr.operands[op_idx].actions & ZYDIS_OPERAND_ACTION_READ ||
           instr.operands[op_idx].type == ZYDIS_OPERAND_TYPE_MEMORY) &&
          _uses_reg(instr.operands[op_idx], reg))
        return true;
    return false;
  };

  static const auto _writes = [](zydis_decoded_instr_t& instr,
                                 zydis_reg_t reg) -> bool {
    for (auto op_idx = 0u; op_idx < instr.operand_count; ++op_idx)
      if (instr.operands[op_idx].type == ZYDIS_OPERAND_TYPE_REGISTER &&
          instr.operands[op_idx].actions & ZYDIS_OPERAND_ACTION_WRITE &&
          vm::utils::reg::compare(instr.operands[op_idx].reg.value, reg))
        return true;
    return false;
  };

  std::uint32_t last_size = 0u;
  static const std::vector<ZydisMnemonic> blacklist = {
      ZYDIS_MNEMONIC_CLC,    ZYDIS_MNEMONIC_BT,      ZYDIS_MNEMONIC_TEST,
      ZYDIS_MNEMONIC_CMP,    ZYDIS_MNEMONIC_CMC,     ZYDIS_MNEMONIC_STC,
      ZYDIS_MNEMONIC_CMOVB,  ZYDIS_MNEMONIC_CMOVBE,  ZYDIS_MNEMONIC_CMOVL,
      ZYDIS_MNEMONIC_CMOVLE, ZYDIS_MNEMONIC_CMOVNB,  ZYDIS_MNEMONIC_CMOVNBE,
      ZYDIS_MNEMONIC_CMOVNL, ZYDIS_MNEMONIC_CMOVNLE, ZYDIS_MNEMONIC_CMOVNO,
      ZYDIS_MNEMONIC_CMOVNP, ZYDIS_MNEMONIC_CMOVNS,  ZYDIS_MNEMONIC_CMOVNZ,
      ZYDIS_MNEMONIC_CMOVO,  ZYDIS_MNEMONIC_CMOVP,   ZYDIS_MNEMONIC_CMOVS,
      ZYDIS_MNEMONIC_CMOVZ,
  };

  static const std::vector<ZydisMnemonic> whitelist = {
      ZYDIS_MNEMONIC_PUSH, ZYDIS_MNEMONIC_POP, ZYDIS_MNEMONIC_CALL,
      ZYDIS_MNEMONIC_DIV};

  do {
    last_size = routine.size();
    for (auto itr = routine.begin(); itr != routine.end(); ++itr) {
      if (std::find(whitelist.begin(), whitelist.end(), itr->instr.mnemonic) !=
          whitelist.end())
        continue;

      if (std::find(blacklist.begin(), blacklist.end(), itr->instr.mnemonic) !=
          blacklist.end()) {
        routine.erase(itr);
        break;
      }

      zydis_reg_t reg = ZYDIS_REGISTER_NONE;
      // look for operands with writes to a register...
      for (auto op_idx = 0u; op_idx < itr->instr.operand_count; ++op_idx)
        if (itr->instr.operands[op_idx].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            itr->instr.operands[op_idx].actions & ZYDIS_OPERAND_ACTION_WRITE)
          reg = vm::utils::reg::to64(itr->instr.operands[0].reg.value);

      // if this current instruction writes to a register, look ahead in the
      // instruction stream to see if it gets written too before it gets read...
      if (reg != ZYDIS_REGISTER_NONE) {
        // find the next place that this register is written too...
        auto write_result = std::find_if(itr + 1, routine.end(),
                                         [&](zydis_instr_t& instr) -> bool {
                                           return _writes(instr.instr, reg);
                                         });

        auto read_result = std::find_if(itr + 1, write_result,
                                        [&](zydis_instr_t& instr) -> bool {
                                          return _reads(instr.instr, reg);
                                        });

        // if there is neither a read or a write to this register in the
        // instruction stream then we are going to be safe and leave the
        // instruction in the stream...
        if (read_result == routine.end() && write_result == routine.end())
          continue;

        // if there is no read of the register before the next write... and
        // there is a known next write, then remove the instruction from the
        // stream...
        if (read_result == write_result && write_result != routine.end()) {
          // if the instruction reads and writes the same register than skip...
          if (_reads(read_result->instr, reg) &&
              _writes(read_result->instr, reg))
            continue;

          routine.erase(itr);
          break;
        }
      }
    }
  } while (last_size != routine.size());
}

namespace reg {
zydis_reg_t to64(zydis_reg_t reg) {
  return ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LONG_64, reg);
}

bool compare(zydis_reg_t a, zydis_reg_t b) {
  return to64(a) == to64(b);
}
}  // namespace reg

namespace scn {
bool read_only(std::uint64_t module_base, std::uint64_t ptr) {
  auto win_image = reinterpret_cast<win::image_t<>*>(module_base);
  auto section_count = win_image->get_file_header()->num_sections;
  auto sections = win_image->get_nt_headers()->get_sections();

  for (auto idx = 0u; idx < section_count; ++idx)
    if (ptr >= sections[idx].virtual_address + module_base &&
        ptr < sections[idx].virtual_address + sections[idx].virtual_size +
                  module_base)
      return !(sections[idx].characteristics.mem_discardable) &&
             !(sections[idx].characteristics.mem_write);

  return false;
}

bool executable(std::uint64_t module_base, std::uint64_t ptr) {
  auto win_image = reinterpret_cast<win::image_t<>*>(module_base);
  auto section_count = win_image->get_file_header()->num_sections;
  auto sections = win_image->get_nt_headers()->get_sections();

  for (auto idx = 0u; idx < section_count; ++idx)
    if (ptr >= sections[idx].virtual_address + module_base &&
        ptr < sections[idx].virtual_address + sections[idx].virtual_size +
                  module_base)
      return !(sections[idx].characteristics.mem_discardable) &&
             sections[idx].characteristics.mem_execute;

  return false;
}
}  // namespace scn
}  // namespace vm::utils