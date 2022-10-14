#include <vminstrs.hpp>

//Loads CR0 onto the stack
namespace vm::instrs {
profiler_t lcr0 = {
  "LCR0",
  mnemonic_t::lcr0,
  {
    // MOV REG, CR0
    [](const zydis_reg_t vip, const zydis_reg_t vsp,
        const zydis_decoded_instr_t& instr) -> bool {
      return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
              instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[1].reg.value == ZYDIS_REGISTER_CR0;
    },
    // SUB VSP, OFFSET
    [](const zydis_reg_t vip, const zydis_reg_t vsp,
        const zydis_decoded_instr_t& instr) -> bool {
      return instr.mnemonic == ZYDIS_MNEMONIC_SUB &&
              instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[0].reg.value == vsp &&
              instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
    },
    // MOV [VSP], REG
    [](const zydis_reg_t vip, const zydis_reg_t vsp,
        const zydis_decoded_instr_t& instr) -> bool {
      return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
              instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
              instr.operands[0].mem.base == vsp &&
              instr.operands[0].mem.index == ZYDIS_REGISTER_NONE &&
              instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[1].reg.value != vsp;
    }
  },
  [](zydis_reg_t& vip, zydis_reg_t& vsp,
      hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res{mnemonic_t::lcr0};
      res.imm.has_imm = false;
      res.stack_size = 64;
      return res;
  }
};
}
