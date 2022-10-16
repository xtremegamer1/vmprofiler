#pragma once
#include <Zydis/Zydis.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <nt/image.hpp>
#include <optional>
#include <vector>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using zydis_decoded_instr_t = ZydisDecodedInstruction;
using zydis_reg_t = ZydisRegister;
using zydis_mnemonic_t = ZydisMnemonic;
using zydis_decoded_operand_t = ZydisDecodedOperand;

struct zydis_instr_t {
  zydis_decoded_instr_t instr;
  std::vector<u8> raw;
  std::uintptr_t addr;
};

using zydis_rtn_t = std::vector<zydis_instr_t>;

namespace vm::utils {
inline thread_local std::shared_ptr<ZydisDecoder> g_decoder = nullptr;
inline thread_local std::shared_ptr<ZydisFormatter> g_formatter = nullptr;

inline void init() {
  if (!vm::utils::g_decoder && !vm::utils::g_formatter) {
    vm::utils::g_decoder = std::make_shared<ZydisDecoder>();
    vm::utils::g_formatter = std::make_shared<ZydisFormatter>();

    ZydisDecoderInit(vm::utils::g_decoder.get(), ZYDIS_MACHINE_MODE_LONG_64,
                     ZYDIS_ADDRESS_WIDTH_64);

    ZydisFormatterInit(vm::utils::g_formatter.get(),
                       ZYDIS_FORMATTER_STYLE_INTEL);
  }
}

inline bool open_binary_file(const std::string& file,
                             std::vector<uint8_t>& data) {
  std::ifstream fstr(file, std::ios::binary);
  if (!fstr.is_open()) return false;

  fstr.unsetf(std::ios::skipws);
  fstr.seekg(0, std::ios::end);

  const auto file_size = fstr.tellg();

  fstr.seekg(NULL, std::ios::beg);
  data.reserve(static_cast<uint32_t>(file_size));
  data.insert(data.begin(), std::istream_iterator<uint8_t>(fstr),
              std::istream_iterator<uint8_t>());
  return true;
}

/// <summary>
/// determines if a given decoded native instruction is a JCC...
/// </summary>
/// <param name="instr"></param>
/// <returns></returns>
bool is_jmp(const zydis_decoded_instr_t& instr);

bool is_32_bit_gp(const ZydisRegister reg);

bool is_64_bit_gp(const ZydisRegister reg);

/// <summary>
/// used by profiles to see if an instruction is a MOV/SX/ZX...
/// </summary>
/// <param name="instr"></param>
/// <returns></returns>
bool is_mov(const zydis_decoded_instr_t& instr);

/// <summary>
/// prints a disassembly view of a routine...
/// </summary>
/// <param name="routine">reference to a zydis_routine_t to be
/// printed...</param>
void print(zydis_rtn_t& routine);

/// <summary>
/// prints a single disassembly view of an instruction...
/// </summary>
/// <param name="instr">instruction to print...</param>
void print(const zydis_decoded_instr_t& instr);

/// <summary>
/// utils pertaining to native registers...
/// </summary>
namespace reg {
/// <summary>
/// converts say... AL to RAX...
/// </summary>
/// <param name="reg">a zydis decoded register value...</param>
/// <returns>returns the largest width register of the given register... AL
/// gives RAX...</returns>
zydis_reg_t to64(zydis_reg_t reg);

/// <summary>
/// compares to registers with each other... calls to64 and compares...
/// </summary>
/// <param name="a">register a...</param>
/// <param name="b">register b...</param>
/// <returns>returns true if register to64(a) == to64(b)...</returns>
bool compare(zydis_reg_t a, zydis_reg_t b);
}  // namespace reg

/// <summary>
/// flatten native instruction stream, takes every JCC (follows the branch)...
/// </summary>
/// <param name="routine">filled with decoded instructions...</param>
/// <param name="routine_addr">linear virtual address to start flattening
/// from...</param> <param name="keep_jmps">keep JCC's in the flattened
/// instruction stream...</param> <returns>returns true if flattened was
/// successful...</returns>
bool flatten(zydis_rtn_t& routine, std::uintptr_t routine_addr,
             bool keep_jmps = false, std::uint32_t max_instrs = 500,
             std::uintptr_t module_base = 0ull);

/// <summary>
/// deadstore deobfuscation of a flattened routine...
/// </summary>
/// <param name="routine">reference to a flattened instruction vector...</param>
void deobfuscate(zydis_rtn_t& routine);

/// <summary>
/// small namespace that contains function wrappers to determine the validity of
/// linear virtual addresses...
/// </summary>
namespace scn {
/// <summary>
/// determines if a pointer lands inside of a section that is readonly...
///
/// this also checks to make sure the section is not discardable...
/// </summary>
/// <param name="module_base">linear virtual address of the module....</param>
/// <param name="ptr">linear virtual address</param>
/// <returns>returns true if ptr lands inside of a readonly section of the
/// module</returns>
bool read_only(std::uint64_t module_base, std::uint64_t ptr);

/// <summary>
/// determines if a pointer lands inside of a section that is executable...
///
/// this also checks to make sure the section is not discardable...
/// </summary>
/// <param name="module_base"></param>
/// <param name="ptr"></param>
/// <returns></returns>
bool executable(std::uint64_t module_base, std::uint64_t ptr);
}  // namespace scn
}  // namespace vm::utils