#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t sreg = {
    "SREG",
    mnemonic_t::sreg,
    {{// MOV REG, [VSP]
      [&](const zydis_reg_t vip,
          const zydis_reg_t vsp,
          const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.base == vsp;
      },
      // ADD VSP, 8
      [&](const zydis_reg_t vip,
          const zydis_reg_t vsp,
          const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_ADD &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[0].reg.value == vsp &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
               instr.operands[1].imm.value.u == 8;
      },
      // MOV REG, [VIP]
      [&](const zydis_reg_t vip,
          const zydis_reg_t vsp,
          const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.base == vip;
      },
      // MOV [RSP+REG], REG
      [&](const zydis_reg_t vip,
          const zydis_reg_t vsp,
          const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[0].mem.base == ZYDIS_REGISTER_RSP &&
               instr.operands[0].mem.index != ZYDIS_REGISTER_NONE &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER;
      }}},
    [&](zydis_reg_t& vip,
        zydis_reg_t& vsp,
        hndlr_trace_t& hndlr) -> std::optional<vinstr_t> { return {}; }};
}