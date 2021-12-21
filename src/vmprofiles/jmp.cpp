#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t jmp = {
    "JMP",
    mnemonic_t::jmp,
    // MOV REG, [VSP]
    {{[&](const zydis_reg_t vip,
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
      // MOV VIP, REG
      [&](const zydis_reg_t vip,
          const zydis_reg_t vsp,
          const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[0].reg.value == vip &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER;
      }}},
    [&](zydis_reg_t& vip,
        zydis_reg_t& vsp,
        zydis_rtn_t& hndlr) -> std::optional<vinstr_t> {
      const auto xchg = std::find_if(
          hndlr.begin(), hndlr.end(), [&](const zydis_instr_t& instr) -> bool {
            const auto& i = instr.instr;
            return i.mnemonic == ZYDIS_MNEMONIC_XCHG &&
                   i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   // exclusive or... operand 1 or operand 2 can be VSP but they
                   // both cannot be...
                   ((i.operands[1].reg.value == vsp ||
                     i.operands[0].reg.value == vsp) &&
                    !((i.operands[1].reg.value == vsp) &&
                      (i.operands[0].reg.value == vsp)));
          });

      // this JMP virtual instruction changes VSP as well as VIP...
      if (xchg != hndlr.end()) {
        // grab the register that isnt VSP in the XCHG...
        // xchg reg, vsp or xchg vsp, reg...
        zydis_reg_t write_dep = xchg->instr.operands[0].reg.value != vsp
                                    ? xchg->instr.operands[0].reg.value
                                    : xchg->instr.operands[1].reg.value;

        // update VIP... VSP becomes VIP... with the XCHG...
        vip = xchg->instr.operands[0].reg.value != vsp
                  ? xchg->instr.operands[1].reg.value
                  : xchg->instr.operands[0].reg.value;

        // find the next MOV REG, write_dep... this REG will be VSP...
        const auto mov_reg_write_dep = std::find_if(
            xchg, hndlr.end(), [&](const zydis_instr_t& instr) -> bool {
              const auto& i = instr.instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].reg.value == write_dep;
            });

        if (mov_reg_write_dep == hndlr.end())
          return {};

        vsp = mov_reg_write_dep->instr.operands[0].reg.value;
      } else {
        // find the MOV REG, [VSP] instruction...
        const auto mov_reg_deref_vsp = std::find_if(
            hndlr.begin(), hndlr.end(),
            [&](const zydis_instr_t& instr) -> bool {
              const auto& i = instr.instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                     i.operands[1].mem.base == vsp;
            });

        if (mov_reg_deref_vsp == hndlr.end())
          return {};

        // find the MOV REG, mov_reg_deref_vsp->operands[0].reg.value
        const auto mov_vip_reg = std::find_if(
            mov_reg_deref_vsp, hndlr.end(),
            [&](const zydis_instr_t& instr) -> bool {
              const auto& i = instr.instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].reg.value ==
                         mov_reg_deref_vsp->instr.operands[0].reg.value;
            });

        if (mov_vip_reg == hndlr.end())
          return {};

        vip = mov_vip_reg->instr.operands[0].reg.value;

        // see if VSP gets updated as well...
        const auto mov_reg_vsp = std::find_if(
            mov_reg_deref_vsp, hndlr.end(),
            [&](const zydis_instr_t& instr) -> bool {
              const auto& i = instr.instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].reg.value == vsp;
            });

        if (mov_reg_vsp != hndlr.end())
          vsp = mov_reg_vsp->instr.operands[0].reg.value;
      }
      return vinstr_t{mnemonic_t::jmp};
    }};
}