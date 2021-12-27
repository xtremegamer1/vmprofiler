#pragma once
#include <unicorn/unicorn.h>
#include <vmutils.hpp>

namespace vm::instrs {
/// <summary>
/// mnemonic representation of supported virtual instructions...
/// </summary>
enum class mnemonic_t {
  unknown,
  sreg,
  lreg,
  lconst,
  add,
  div,
  idiv,
  mul,
  imul,
  nand,
  read,
  write,
  shl,
  shld,
  shr,
  shrd,
  lvsp,
  svsp,
  writecr3,
  readcr3,
  writecr8,
  readcr8,
  cpuid,
  rdtsc,
  call,
  jmp,
  vmexit
};

/// <summary>
/// the main virtual instruction structure which is returned by profilers...
/// </summary>
struct vinstr_t {
  /// <summary>
  /// mnemonic of the virtual instruction...
  /// </summary>
  mnemonic_t mnemonic;

  /// <summary>
  /// size varient of the virtual instruction... I.E SREGQ would have a value of
  /// "64" here...where the SREGDW varient would have a "32" here... this is the
  /// stack disposition essentially, or the value on the stack...
  /// </summary>
  u8 stack_size;

  struct {
    /// <summary>
    /// true if the virtual instruction has an imm false if not...
    /// </summary>
    bool has_imm;

    /// <summary>
    /// size in bits of the imm... 8, 16, 32, 64...
    /// </summary>
    u8 size;

    /// <summary>
    /// imm value...
    /// </summary>
    u64 val;
  } imm;
};

/// <summary>
/// emu instruction containing current cpu register values and such...
/// </summary>
struct emu_instr_t {
  zydis_decoded_instr_t m_instr;
  uc_context* m_cpu;
};

/// <summary>
/// handler trace containing information about a stream of instructions... also
/// contains some information about the virtual machine such as vip and vsp...
/// </summary>
struct hndlr_trace_t {
  std::uintptr_t m_hndlr_addr;
  uc_engine* m_uc;
  zydis_reg_t m_vip, m_vsp;
  std::vector<emu_instr_t> m_instrs;
};

/// <summary>
/// matcher function which returns true if an instruction matches a desired
/// one...
/// </summary>
using matcher_t = std::function<bool(const zydis_reg_t vip,
                                     const zydis_reg_t vsp,
                                     const zydis_decoded_instr_t& instr)>;

/// <summary>
/// virtual instruction structure generator... this can update the vip and vsp
/// argument... it cannot update the instruction stream (hndlr)...
/// </summary>
using vinstr_gen_t =
    std::function<std::optional<vinstr_t>(zydis_reg_t& vip,
                                          zydis_reg_t& vsp,
                                          hndlr_trace_t& hndlr)>;

/// <summary>
/// each virtual instruction has its own profiler_t structure which can generate
/// all varients of the virtual instruction for each size...
/// </summary>
struct profiler_t {
  /// <summary>
  /// string name of the virtual instruction that this profile generates for...
  /// </summary>
  std::string name;

  /// <summary>
  /// mnemonic representation of the virtual instruction...
  /// </summary>
  mnemonic_t mnemonic;

  /// <summary>
  /// vector of matcher lambda's which return true if a given instruction
  /// matches...
  /// </summary>
  std::vector<matcher_t> matchers;

  /// <summary>
  /// generates a virtual instruction structure...
  /// </summary>
  vinstr_gen_t generate;
};

/// <summary>
/// list of all profiles here...
/// </summary>
extern profiler_t jmp;
extern profiler_t sreg;
extern profiler_t lreg;
extern profiler_t lconst;
extern profiler_t add;
extern profiler_t lvsp;
extern profiler_t svsp;
extern profiler_t nand;
extern profiler_t read;
extern profiler_t write;

/// <summary>
/// unsorted vector of profiles... they get sorted once at runtime...
/// </summary>
inline std::vector<profiler_t*> profiles = {
    &write, &svsp, &read, &nand, &lvsp, &add, &jmp, &sreg, &lreg, &lconst};

/// <summary>
/// no i did not make this by hand, you cannot clown upon me!
/// </summary>
inline std::map<zydis_reg_t, uc_x86_reg> reg_map = {
    {ZYDIS_REGISTER_AL, UC_X86_REG_AL},
    {ZYDIS_REGISTER_CL, UC_X86_REG_CL},
    {ZYDIS_REGISTER_DL, UC_X86_REG_DL},
    {ZYDIS_REGISTER_BL, UC_X86_REG_BL},
    {ZYDIS_REGISTER_AH, UC_X86_REG_AH},
    {ZYDIS_REGISTER_CH, UC_X86_REG_CH},
    {ZYDIS_REGISTER_DH, UC_X86_REG_DH},
    {ZYDIS_REGISTER_BH, UC_X86_REG_BH},
    {ZYDIS_REGISTER_SPL, UC_X86_REG_SPL},
    {ZYDIS_REGISTER_BPL, UC_X86_REG_BPL},
    {ZYDIS_REGISTER_SIL, UC_X86_REG_SIL},
    {ZYDIS_REGISTER_DIL, UC_X86_REG_DIL},
    {ZYDIS_REGISTER_R8B, UC_X86_REG_R8B},
    {ZYDIS_REGISTER_R9B, UC_X86_REG_R9B},
    {ZYDIS_REGISTER_R10B, UC_X86_REG_R10B},
    {ZYDIS_REGISTER_R11B, UC_X86_REG_R11B},
    {ZYDIS_REGISTER_R12B, UC_X86_REG_R12B},
    {ZYDIS_REGISTER_R13B, UC_X86_REG_R13B},
    {ZYDIS_REGISTER_R14B, UC_X86_REG_R14B},
    {ZYDIS_REGISTER_R15B, UC_X86_REG_R15B},
    {ZYDIS_REGISTER_AX, UC_X86_REG_AX},
    {ZYDIS_REGISTER_CX, UC_X86_REG_CX},
    {ZYDIS_REGISTER_DX, UC_X86_REG_DX},
    {ZYDIS_REGISTER_BX, UC_X86_REG_BX},
    {ZYDIS_REGISTER_SP, UC_X86_REG_SP},
    {ZYDIS_REGISTER_BP, UC_X86_REG_BP},
    {ZYDIS_REGISTER_SI, UC_X86_REG_SI},
    {ZYDIS_REGISTER_DI, UC_X86_REG_DI},
    {ZYDIS_REGISTER_R8W, UC_X86_REG_R8W},
    {ZYDIS_REGISTER_R9W, UC_X86_REG_R9W},
    {ZYDIS_REGISTER_R10W, UC_X86_REG_R10W},
    {ZYDIS_REGISTER_R11W, UC_X86_REG_R11W},
    {ZYDIS_REGISTER_R12W, UC_X86_REG_R12W},
    {ZYDIS_REGISTER_R13W, UC_X86_REG_R13W},
    {ZYDIS_REGISTER_R14W, UC_X86_REG_R14W},
    {ZYDIS_REGISTER_R15W, UC_X86_REG_R15W},
    {ZYDIS_REGISTER_EAX, UC_X86_REG_EAX},
    {ZYDIS_REGISTER_ECX, UC_X86_REG_ECX},
    {ZYDIS_REGISTER_EDX, UC_X86_REG_EDX},
    {ZYDIS_REGISTER_EBX, UC_X86_REG_EBX},
    {ZYDIS_REGISTER_ESP, UC_X86_REG_ESP},
    {ZYDIS_REGISTER_EBP, UC_X86_REG_EBP},
    {ZYDIS_REGISTER_ESI, UC_X86_REG_ESI},
    {ZYDIS_REGISTER_EDI, UC_X86_REG_EDI},
    {ZYDIS_REGISTER_R8D, UC_X86_REG_R8D},
    {ZYDIS_REGISTER_R9D, UC_X86_REG_R9D},
    {ZYDIS_REGISTER_R10D, UC_X86_REG_R10D},
    {ZYDIS_REGISTER_R11D, UC_X86_REG_R11D},
    {ZYDIS_REGISTER_R12D, UC_X86_REG_R12D},
    {ZYDIS_REGISTER_R13D, UC_X86_REG_R13D},
    {ZYDIS_REGISTER_R14D, UC_X86_REG_R14D},
    {ZYDIS_REGISTER_R15D, UC_X86_REG_R15D},
    {ZYDIS_REGISTER_RAX, UC_X86_REG_RAX},
    {ZYDIS_REGISTER_RCX, UC_X86_REG_RCX},
    {ZYDIS_REGISTER_RDX, UC_X86_REG_RDX},
    {ZYDIS_REGISTER_RBX, UC_X86_REG_RBX},
    {ZYDIS_REGISTER_RSP, UC_X86_REG_RSP},
    {ZYDIS_REGISTER_RBP, UC_X86_REG_RBP},
    {ZYDIS_REGISTER_RSI, UC_X86_REG_RSI},
    {ZYDIS_REGISTER_RDI, UC_X86_REG_RDI},
    {ZYDIS_REGISTER_R8, UC_X86_REG_R8},
    {ZYDIS_REGISTER_R9, UC_X86_REG_R9},
    {ZYDIS_REGISTER_R10, UC_X86_REG_R10},
    {ZYDIS_REGISTER_R11, UC_X86_REG_R11},
    {ZYDIS_REGISTER_R12, UC_X86_REG_R12},
    {ZYDIS_REGISTER_R13, UC_X86_REG_R13},
    {ZYDIS_REGISTER_R14, UC_X86_REG_R14},
    {ZYDIS_REGISTER_R15, UC_X86_REG_R15}};

/// <summary>
/// deadstore and opaque branch removal from unicorn engine trace... this is the
/// same algorithm as the one in vm::utils::deobfuscate...
/// </summary>
/// <param name="trace"></param>
void deobfuscate(hndlr_trace_t& trace);

/// <summary>
/// sorts the profiles by descending order of matchers... this will prevent a
/// smaller profiler with less matchers from being used when it should not be...
///
/// this function can be called multiple times...
/// </summary>
void init();

/// <summary>
/// determines the virtual instruction for the vm handler given vsp and vip...
/// </summary>
/// <param name="vip">vip native register...</param>
/// <param name="vsp">vsp native register...</param>
/// <param name="hndlr"></param>
/// <returns>returns vinstr_t structure...</returns>
vinstr_t determine(zydis_reg_t& vip, zydis_reg_t& vsp, hndlr_trace_t& hndlr);

/// <summary>
/// get profile from mnemonic...
/// </summary>
/// <param name="mnemonic">mnemonic of the profile to get...</param>
/// <returns>pointer to the profile...</returns>
profiler_t* get_profile(mnemonic_t mnemonic);
}  // namespace vm::instrs

// MOV REG, [VIP]
#define IMM_FETCH                                                   \
  [&](const zydis_reg_t vip, const zydis_reg_t vsp,                 \
      const zydis_decoded_instr_t& instr) -> bool {                 \
    return vm::utils::is_mov(instr) &&                              \
           instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER && \
           instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&   \
           instr.operands[1].mem.base == vip;                       \
  }

// MOV [VSP], REG
#define STR_VALUE                                                 \
  [&](const zydis_reg_t vip, const zydis_reg_t vsp,               \
      const zydis_decoded_instr_t& instr) -> bool {               \
    return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&                \
           instr.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY && \
           instr.operands[0].mem.base == vsp &&                   \
           instr.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER; \
  }

// MOV REG, [VSP]
#define LOAD_VALUE                                                  \
  [&](const zydis_reg_t vip, const zydis_reg_t vsp,                 \
      const zydis_decoded_instr_t& instr) -> bool {                 \
    return instr.mnemonic == ZYDIS_MNEMONIC_MOV &&                  \
           instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER && \
           instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&   \
           instr.operands[1].mem.base == vsp;                       \
  }

// SUB VSP, OFFSET
#define SUB_VSP                                                     \
  [&](const zydis_reg_t vip, const zydis_reg_t vsp,                 \
      const zydis_decoded_instr_t& instr) -> bool {                 \
    return instr.mnemonic == ZYDIS_MNEMONIC_SUB &&                  \
           instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER && \
           instr.operands[0].reg.value == vsp &&                    \
           instr.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;  \
  }