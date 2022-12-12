#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t lvsp = {
    "LVSP",
    mnemonic_t::lvsp,
    {{// MOV VSP, [VSP]
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[0].reg.value == vsp &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.base == vsp;
      }}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res{mnemonic_t::lvsp};
      res.imm.has_imm = false;

      const auto load_vsp = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[0].reg.value == vsp &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   i.operands[1].mem.base == vsp;
          });

      res.stack_size = load_vsp->m_instr.operands[0].size;
      return res;
    }};
}