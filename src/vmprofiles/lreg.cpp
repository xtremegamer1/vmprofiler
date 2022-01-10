#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t lreg = {
    "LREG",
    mnemonic_t::lreg,
    {{// MOV REG, [VIP]
      IMM_FETCH,
      // MOV REG, [RSP+REG]
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.base == ZYDIS_REGISTER_RSP &&
               instr.operands[1].mem.index != ZYDIS_REGISTER_NONE;
      },
      // SUB VSP, OFFSET
      SUB_VSP,
      // MOV [VSP], REG
      STR_VALUE}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res;
      res.mnemonic = mnemonic_t::lreg;
      res.imm.has_imm = true;
      res.imm.size = 8;

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
      // // MOV REG, [RSP+REG]... we want the register in [RSP+REG]...
      const auto mov_reg_vreg = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   i.operands[1].mem.base == ZYDIS_REGISTER_RSP &&
                   i.operands[1].mem.index != ZYDIS_REGISTER_NONE;
          });

      uc_context* backup;
      uc_context_alloc(hndlr.m_uc, &backup);
      uc_context_save(hndlr.m_uc, backup);
      uc_context_restore(hndlr.m_uc, mov_reg_vreg->m_cpu);

      const uc_x86_reg idx_reg =
          vm::instrs::reg_map[mov_reg_vreg->m_instr.operands[1].mem.index];

      uc_reg_read(hndlr.m_uc, idx_reg, &res.imm.val);

      res.imm.val <<= (64 - res.imm.size);
      res.imm.val >>= (64 - res.imm.size);

      uc_context_restore(hndlr.m_uc, backup);
      uc_context_free(backup);
      return res;
    }};
}