#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t nop = {
    "NOP",
    mnemonic_t::nop,
    {{// LEA REG, [0x0] ; disp is -7...
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_LEA &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.disp.has_displacement &&
               instr.operands[1].mem.disp.value == -7;
      }}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res;
      res.mnemonic = mnemonic_t::nop;
      res.imm.has_imm = false;
      return res;
    }};
}