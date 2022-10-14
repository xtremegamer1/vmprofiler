#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t read = {
    "READ",
    mnemonic_t::read,
    {{// MOV REG, [VSP]
      LOAD_VALUE,
      // MOV REG, [REG]
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.base != vsp;
      },
      // MOV [VSP], REG
      STR_VALUE}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res{mnemonic_t::read};
      res.imm.has_imm = false;

      // MOV REG, [REG]
      const auto mov_reg_reg = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_MOV || i.mnemonic == ZYDIS_MNEMONIC_MOVZX &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   i.operands[1].mem.base != vsp;
          });

      res.stack_size = mov_reg_reg->m_instr.operands[1].size;
      return res;
    }};
}