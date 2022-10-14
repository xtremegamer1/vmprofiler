#include <vminstrs.hpp>
#include <uc_allocation_tracker.hpp>
namespace vm::instrs {
void deobfuscate(hndlr_trace_t& trace) {
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
    last_size = trace.m_instrs.size();
    for (auto itr = trace.m_instrs.begin(); itr != trace.m_instrs.end();
         ++itr) {
      if (std::find(whitelist.begin(), whitelist.end(),
                    itr->m_instr.mnemonic) != whitelist.end())
        continue;

      if (std::find(blacklist.begin(), blacklist.end(),
                    itr->m_instr.mnemonic) != blacklist.end()) {
        uct_context_free(itr->m_cpu);
        trace.m_instrs.erase(itr);
        break;
      }

      if (vm::utils::is_jmp(itr->m_instr)) {
        uct_context_free(itr->m_cpu);
        trace.m_instrs.erase(itr);
        break;
      }

      zydis_reg_t reg = ZYDIS_REGISTER_NONE;
      // look for operands with writes to a register...
      for (auto op_idx = 0u; op_idx < itr->m_instr.operand_count; ++op_idx)
        if (itr->m_instr.operands[op_idx].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            itr->m_instr.operands[op_idx].actions & ZYDIS_OPERAND_ACTION_WRITE)
          reg = vm::utils::reg::to64(itr->m_instr.operands[0].reg.value);

      // if this current instruction writes to a register, look ahead in the
      // instruction stream to see if it gets written too before it gets read...
      if (reg != ZYDIS_REGISTER_NONE) {
        // find the next place that this register is written too...
        auto write_result = std::find_if(itr + 1, trace.m_instrs.end(),
                                         [&](emu_instr_t& instr) -> bool {
                                           return _writes(instr.m_instr, reg);
                                         });

        auto read_result = std::find_if(itr + 1, write_result,
                                        [&](emu_instr_t& instr) -> bool {
                                          return _reads(instr.m_instr, reg);
                                        });

        // if there is neither a read or a write to this register in the
        // instruction stream then we are going to be safe and leave the
        // instruction in the stream...
        if (read_result == trace.m_instrs.end() &&
            write_result == trace.m_instrs.end())
          continue;

        // if there is no read of the register before the next write... and
        // there is a known next write, then remove the instruction from the
        // stream...
        if (read_result == write_result &&
            write_result != trace.m_instrs.end()) {
          // if the instruction reads and writes the same register than skip...
          if (_reads(read_result->m_instr, reg) &&
              _writes(read_result->m_instr, reg))
            continue;

          uct_context_free(itr->m_cpu);
          trace.m_instrs.erase(itr);
          break;
        }
      }
    }
  } while (last_size != trace.m_instrs.size());
}

void init() {
  if (static std::atomic_bool once = true; once.exchange(false))
    std::sort(profiles.begin(), profiles.end(),
              [&](profiler_t* a, profiler_t* b) -> bool {
                return a->matchers.size() > b->matchers.size();
              });
}

vinstr_t determine(hndlr_trace_t& hndlr) {
  const auto& instrs = hndlr.m_instrs;
  const auto profile = std::find_if(
      profiles.begin(), profiles.end(), [&](profiler_t* profile) -> bool {
        for (auto& matcher : profile->matchers) {
          const auto matched =
              std::find_if(instrs.begin(), instrs.end(),
                           [&](const emu_instr_t& instr) -> bool {
                             const auto& i = instr.m_instr;
                             return matcher(hndlr.m_vip, hndlr.m_vsp, i);
                           });
          if (matched == instrs.end())
            return false;
        }
        return true;
      });

  if (profile == profiles.end())
    return vinstr_t{mnemonic_t::unknown};

  auto result = (*profile)->generate(hndlr.m_vip, hndlr.m_vsp, hndlr);
  return result.has_value() ? result.value() : vinstr_t{mnemonic_t::unknown};
}

profiler_t* get_profile(mnemonic_t mnemonic) {
  if (mnemonic == mnemonic_t::unknown)
    return nullptr;

  const auto res = std::find_if(profiles.begin(), profiles.end(),
                                [&](profiler_t* profile) -> bool {
                                  return profile->mnemonic == mnemonic;
                                });

  return res == profiles.end() ? nullptr : *res;
}
}  // namespace vm::instrs