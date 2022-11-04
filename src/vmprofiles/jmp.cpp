#include <vminstrs.hpp>

namespace vm::instrs {
profiler_t jmp = {
    "JMP",
    mnemonic_t::jmp,
    {{// MOV REG, [VSP]
      LOAD_VALUE,
      // ADD VSP, 8
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_ADD &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[0].reg.value == vsp &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
               instr.operands[1].imm.value.u == 8;
      },
      // MOV REG, IMM_64
      [](const zydis_reg_t vip, const zydis_reg_t vsp,
         const zydis_decoded_instr_t& instr) -> bool {
        return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&
               instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
               instr.operands[1].size == 64;
      },
      // LEA REG, [0x0] ; disp is -7...
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
      const auto& instrs = hndlr.m_instrs;
      const auto xchg = std::find_if(
          instrs.begin(), instrs.end(), [&](const emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
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
      if (xchg != instrs.end()) {
        // grab the register that isnt VSP in the XCHG...
        // xchg reg, vsp or xchg vsp, reg...
        zydis_reg_t write_dep = xchg->m_instr.operands[0].reg.value != vsp
                                    ? xchg->m_instr.operands[0].reg.value
                                    : xchg->m_instr.operands[1].reg.value;

        // update VIP... VSP becomes VIP... with the XCHG...
        vip = xchg->m_instr.operands[0].reg.value != vsp
                  ? xchg->m_instr.operands[1].reg.value
                  : xchg->m_instr.operands[0].reg.value;

        // find the next MOV REG, write_dep... this REG will be VSP...
        const auto mov_reg_write_dep = std::find_if(
            xchg, instrs.end(), [&](const emu_instr_t& instr) -> bool {
              const auto& i = instr.m_instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].reg.value == write_dep;
            });

        if (mov_reg_write_dep == instrs.end()) 
          vsp = write_dep;
        else
          vsp = mov_reg_write_dep->m_instr.operands[0].reg.value;
      } else {
        // find the MOV REG, [VSP] instruction...
        const auto mov_reg_deref_vsp = std::find_if(
            instrs.begin(), instrs.end(),
            [&](const emu_instr_t& instr) -> bool {
              const auto& i = instr.m_instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                     i.operands[1].mem.base == vsp;
            });

        if (mov_reg_deref_vsp == instrs.end()) 
          return {};

        // find the MOV REG, mov_reg_deref_vsp->operands[0].reg.value
        const auto mov_vip_reg = std::find_if(
            mov_reg_deref_vsp, instrs.end(),
            [&](const emu_instr_t& instr) -> bool {
              const auto& i = instr.m_instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].reg.value ==
                         mov_reg_deref_vsp->m_instr.operands[0].reg.value;
            });
        //It is possible that mov_vip_reg is actually updating the rolling key, if so use original vip
        const auto load_handler_rva = std::find_if(
          mov_vip_reg, instrs.end(),
          [&](const emu_instr_t& instr) -> bool {
            const auto& i = instr.m_instr;
            return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                    i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                    vm::utils::is_32_bit_gp(i.operands[0].reg.value) &&
                    i.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    i.operands[1].mem.base ==
                        mov_vip_reg->m_instr.operands[0].reg.value;
          });

        if (mov_vip_reg == instrs.end()) 
          return {};

        vip = (load_handler_rva != instrs.end()) ? 
          mov_vip_reg->m_instr.operands[0].reg.value : 
          mov_vip_reg->m_instr.operands[1].reg.value; 
        //Ok so basically mov_vip_reg, despite its name, isn't guaranteed to be
        //mov vip, reg, and can in fact be mov rkey, vip. 

        // see if VSP gets updated as well...
        const auto mov_reg_vsp = std::find_if(
            mov_reg_deref_vsp, instrs.end(),
            [&](const emu_instr_t& instr) -> bool {
              const auto& i = instr.m_instr;
              return i.mnemonic == ZYDIS_MNEMONIC_MOV &&
                     i.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                     i.operands[1].reg.value == vsp;
            });

        if (mov_reg_vsp != instrs.end())
          vsp = mov_reg_vsp->m_instr.operands[0].reg.value;
      }

      vinstr_t res;
      res.mnemonic = mnemonic_t::jmp;
      res.imm.has_imm = false;
      res.stack_size = 64;
      return res;
    }};
}