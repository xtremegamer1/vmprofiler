#pragma once
#include <Zydis/Zydis.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using zydis_decoded_instr_t = ZydisDecodedInstruction;
using zydis_register_t = ZydisRegister;
using zydis_mnemonic_t = ZydisMnemonic;
using zydis_decoded_operand_t = ZydisDecodedOperand;

struct zydis_instr_t {
  zydis_decoded_instr_t instr;
  std::vector<u8> raw;
  std::uintptr_t addr;
};

using zydis_routine_t = std::vector<zydis_instr_t>;

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
}  // namespace vm::utils