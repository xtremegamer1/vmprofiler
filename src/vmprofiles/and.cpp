#include <vminstrs.hpp>

// Loads an address and value from the stack, ands the derefed address with the value
namespace vm::instrs {
profiler_t _and = {
    "AND",
    mnemonic_t::_and,
    {{// MOV REG, [VSP] This is the address
      LOAD_VALUE,
      // MOV REG, [VSP+8]
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[1].mem.base == vsp &&
               instr.operands[1].mem.disp.has_displacement,
               instr.operands[1].mem.disp.value == 8;
      },
      // AND [REG], REG
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_AND &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[0].mem.base != ZYDIS_REGISTER_NONE &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER;
      },
      // PUSHFQ
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_PUSHFQ;
      },
      // POP [VSP]
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_POP &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr.operands[0].mem.base == vsp;
      }}},
    [](zydis_reg_t& vip, zydis_reg_t& vsp,
       hndlr_trace_t& hndlr) -> std::optional<vinstr_t> {
      vinstr_t res{mnemonic_t::_and};
      res.imm.has_imm = false;

      // MOV REG [VSP+OFFSET]
      const auto mov_vsp_offset = std::find_if(
          hndlr.m_instrs.begin(), hndlr.m_instrs.end(),
          [&](emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return 
                  i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                  i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                  i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                  i.operands[1].mem.base == vsp &&
                  i.operands[1].mem.disp.has_displacement;
          });
      if (mov_vsp_offset == hndlr.m_instrs.end())
        return std::nullopt;

      res.stack_size = mov_vsp_offset->m_instr.operands[0].size;
      return res;
    }};
}