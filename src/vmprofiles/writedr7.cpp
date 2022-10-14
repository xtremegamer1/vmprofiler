#include <vminstrs.hpp>

//Write value on top of stack to dr7
namespace vm::instrs {
profiler_t writedr7 = {
  "WRITEDR7",
  mnemonic_t::writedr7,
  {
    // MOV REG, [VSP+OFFSET]
    LOAD_VALUE,
    // ADD VSP, OFFSET
    [](const zydis_reg_t vip, const zydis_reg_t vsp,
        const zydis_decoded_instr_t& instr) -> bool {
      return instr.mnemonic == ZYDIS_MNEMONIC_ADD &&
              instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[0].reg.value == vsp &&
              instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
    },
    // MOV DR7, REG
    [](const zydis_reg_t vip, const zydis_reg_t vsp,
        const zydis_decoded_instr_t& instr) -> bool {
      return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
              instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[0].reg.value == ZYDIS_REGISTER_DR7 &&
              instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
              instr.operands[1].reg.value != vsp;
    }
  },
  [](zydis_reg_t& vip, zydis_reg_t& vsp,
      hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res{mnemonic_t::writedr7};
      res.stack_size == 64;
      res.imm.has_imm = false;
      return res;
  }
};
}
