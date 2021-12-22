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

/// <summary>
/// unsorted vector of profiles... they get sorted once at runtime...
/// </summary>
inline std::vector<profiler_t*> profiles = {&jmp};

/// <summary>
/// sorts the profiles by descending order of matchers... this will prevent a
/// smaller profiler with less matchers from being used when it should not be...
///
/// this function can be called multiple times...
/// </summary>
inline void init() {
  if (static std::atomic_bool once = true; once.exchange(false))
    std::sort(profiles.begin(), profiles.end(),
              [&](profiler_t* a, profiler_t* b) -> bool {
                return a->matchers.size() > b->matchers.size();
              });
}

/// <summary>
/// determines the virtual instruction for the vm handler given vsp and vip...
/// </summary>
/// <param name="vip">vip native register...</param>
/// <param name="vsp">vsp native register...</param>
/// <param name="hndlr"></param>
/// <returns>returns vinstr_t structure...</returns>
inline vinstr_t determine(zydis_reg_t& vip,
                          zydis_reg_t& vsp,
                          hndlr_trace_t& hndlr) {
  const auto& instrs = hndlr.m_instrs;
  const auto profile = std::find_if(
      profiles.begin(), profiles.end(), [&](profiler_t* profile) -> bool {
        for (auto& matcher : profile->matchers) {
          const auto matched =
              std::find_if(instrs.begin(), instrs.end(),
                           [&](const emu_instr_t& instr) -> bool {
                             const auto& i = instr.m_instr;
                             return matcher(vip, vsp, i);
                           });
          if (matched == instrs.end())
            return false;
        }
        return true;
      });

  if (profile == profiles.end())
    return vinstr_t{mnemonic_t::unknown};

  auto result = (*profile)->generate(vip, vsp, hndlr);
  return result.has_value() ? result.value() : vinstr_t{mnemonic_t::unknown};
}

/// <summary>
/// get profile from mnemonic...
/// </summary>
/// <param name="mnemonic">mnemonic of the profile to get...</param>
/// <returns>pointer to the profile...</returns>
inline profiler_t* get_profile(mnemonic_t mnemonic) {
  if (mnemonic == mnemonic_t::unknown)
    return nullptr;

  const auto res = std::find_if(profiles.begin(), profiles.end(),
                                [&](profiler_t* profile) -> bool {
                                  return profile->mnemonic == mnemonic;
                                });

  return res == profiles.end() ? nullptr : *res;
}
}  // namespace vm::instrs