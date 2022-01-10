#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t lconst = {
    "LCONST",
    mnemonic_t::lconst,
    {{// MOV REG, [VIP]
      IMM_FETCH,
      // SUB VSP, OFFSET
      SUB_VSP,
      // MOV [VSP], REG
      STR_VALUE}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res;
      res.mnemonic = mnemonic_t::lconst;
      res.imm.has_imm = true;

      const auto sub_vsp = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_SUB &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[0].reg.value == vsp &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
          });

      res.stack_size = sub_vsp->m_instr.operands[1].imm.value.u * 8;
      const auto fetch_imm = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return vm::utils::is_mov(i) &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   i.operands[1].mem.base == vip;
          });

      res.imm.size = fetch_imm->m_instr.operands[1].size;
      const auto mov_vsp_imm = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   i.operands[0].mem.base == vsp &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER;
          });

      uc_context* backup;
      uc_context_alloc(hndlr.m_uc, &backup);
      uc_context_save(hndlr.m_uc, backup);
      uc_context_restore(hndlr.m_uc, mov_vsp_imm->m_cpu);

      const uc_x86_reg imm_reg =
          vm::instrs::reg_map[mov_vsp_imm->m_instr.operands[1].reg.value];

      uc_reg_read(hndlr.m_uc, imm_reg, &res.imm.val);

      res.imm.val <<= (64 - res.imm.size);
      res.imm.val >>= (64 - res.imm.size);

      uc_context_restore(hndlr.m_uc, backup);
      uc_context_free(backup);
      return res;
    }};
}