#pragma once
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
  /// "64" here...where the SREGDW varient would have a "32" here...
  /// </summary>
  u8 size;

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
using vinstr_gen_t = std::function<std::optional<vinstr_t>(zydis_reg_t& vip,
                                                           zydis_reg_t& vsp,
                                                           zydis_rtn_t& hndlr)>;

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

extern profiler_t jmp;
inline std::vector<profiler_t*> profiles = {&jmp};

inline vinstr_t determine(zydis_reg_t& vip,
                          zydis_reg_t& vsp,
                          zydis_rtn_t& hndlr) {
  const auto profile = std::find_if(
      profiles.begin(), profiles.end(), [&](profiler_t* profile) -> bool {
        for (auto& matcher : profile->matchers) {
          const auto matched = std::find_if(hndlr.begin(), hndlr.end(),
                                            [&](zydis_instr_t& instr) -> bool {
                                              const auto& i = instr.instr;
                                              return matcher(vip, vsp, i);
                                            });
          if (matched == hndlr.end())
            return false;
        }
        return true;
      });

  if (profile == profiles.end())
    return vinstr_t{mnemonic_t::unknown};

  auto result = (*profile)->generate(vip, vsp, hndlr);
  return result.has_value() ? result.value() : vinstr_t{mnemonic_t::unknown};
}
}  // namespace vm::instrs