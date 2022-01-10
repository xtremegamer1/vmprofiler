#pragma once
#include <nt/image.hpp>
#include <vmutils.hpp>

#define PUSH_4B_IMM "\x68\x00\x00\x00\x00"
#define PUSH_4B_MASK "x????"

namespace vm::locate {
inline bool find(const zydis_rtn_t& rtn,
                 std::function<bool(const zydis_instr_t&)> callback) {
  auto res = std::find_if(rtn.begin(), rtn.end(), callback);
  return res != rtn.end();
}

struct vm_enter_t {
  std::uint32_t rva;
  std::uint32_t encrypted_rva;
};

std::uintptr_t sigscan(void* base, std::uint32_t size, const char* pattern,
                       const char* mask);

std::vector<vm_enter_t> get_vm_entries(std::uintptr_t module_base,
                                       std::uint32_t module_size);
}  // namespace vm::locate