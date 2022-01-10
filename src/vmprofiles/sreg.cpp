#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t sreg = {
    "SREG",
    mnemonic_t::sreg,
    {{// MOV REG, [VSP]
      LOAD_VALUE,
      // ADD VSP, OFFSET
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_ADD &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[0].reg.value == vsp &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
      },
      // MOV REG, [VIP]
      IMM_FETCH,
      // MOV [RSP+REG], REG
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[0].mem.base == ZYDIS_REGISTER_RSP &&
               instr.operands[0].mem.index != ZYDIS_REGISTER_NONE &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER;
      }}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res;
      res.mnemonic = mnemonic_t::sreg;
      res.imm.has_imm = true;
      res.imm.size = 8;

      const auto add_vsp = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_ADD &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[0].reg.value == vsp &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
          });

      res.stack_size = add_vsp->m_instr.operands[1].imm.value.u * 8;
      const auto mov_vreg_value = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   i.operands[0].mem.base == ZYDIS_REGISTER_RSP &&
                   i.operands[0].mem.index != ZYDIS_REGISTER_NONE &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER;
          });

      uc_context* backup;
      uc_context_alloc(hndlr.m_uc, &backup);
      uc_context_save(hndlr.m_uc, backup);
      uc_context_restore(hndlr.m_uc, mov_vreg_value->m_cpu);

      const uc_x86_reg idx_reg =
          vm::instrs::reg_map[mov_vreg_value->m_instr.operands[0].mem.index];

      uc_reg_read(hndlr.m_uc, idx_reg, &res.imm.val);

      res.imm.val <<= (64 - res.imm.size);
      res.imm.val >>= (64 - res.imm.size);

      uc_context_restore(hndlr.m_uc, backup);
      uc_context_free(backup);
      return res;
    }};
}