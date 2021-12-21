#include <cli-parser.hpp>
#include <vmprofiler.hpp>

int __cdecl main(int argc, const char* argv[]) {
  argparse::argument_parser_t parser("VMEmu",
                                     "VMProtect 3 VM Handler Emulator");
  parser.add_argument()
      .name("--vmentry")
      .description("relative virtual address to a vm entry...")
      .required(true);

  parser.add_argument()
      .name("--bin")
      .description("path to unpacked virtualized binary...")
      .required(true);

  parser.enable_help();
  auto result = parser.parse(argc, argv);

  if (result) {
    std::printf("[!] error parsing commandline arguments... reason = %s\n",
                result.what().c_str());
    return -1;
  }

  if (parser.exists("help")) {
    parser.print_help();
    return 0;
  }

  vm::utils::init();
  std::vector<std::uint8_t> module_data, tmp, unpacked_bin;
  if (!vm::utils::open_binary_file(parser.get<std::string>("bin"),
                                   module_data)) {
    std::printf("[!] failed to open binary file...\n");
    return -1;
  }

  auto img = reinterpret_cast<win::image_t<>*>(module_data.data());
  auto image_size = img->get_nt_headers()->optional_header.size_image;
  const auto image_base = img->get_nt_headers()->optional_header.image_base;

  // page align the vector allocation so that unicorn-engine is happy girl...
  tmp.resize(image_size + 0x1000);
  const std::uintptr_t module_base =
      reinterpret_cast<std::uintptr_t>(tmp.data()) +
      (0x1000 - (reinterpret_cast<std::uintptr_t>(tmp.data()) & 0xFFFull));

  std::memcpy((void*)module_base, module_data.data(), 0x1000);
  std::for_each(img->get_nt_headers()->get_sections(),
                img->get_nt_headers()->get_sections() +
                    img->get_nt_headers()->file_header.num_sections,
                [&](const auto& section_header) {
                  std::memcpy(
                      (void*)(module_base + section_header.virtual_address),
                      module_data.data() + section_header.ptr_raw_data,
                      section_header.size_raw_data);
                });

  auto win_img = reinterpret_cast<win::image_t<>*>(module_base);

  auto basereloc_dir =
      win_img->get_directory(win::directory_id::directory_entry_basereloc);

  auto reloc_dir = reinterpret_cast<win::reloc_directory_t*>(
      basereloc_dir->rva + module_base);

  win::reloc_block_t* reloc_block = &reloc_dir->first_block;

  // apply relocations to all sections...
  while (reloc_block->base_rva && reloc_block->size_block) {
    std::for_each(reloc_block->begin(), reloc_block->end(),
                  [&](win::reloc_entry_t& entry) {
                    switch (entry.type) {
                      case win::reloc_type_id::rel_based_dir64: {
                        auto reloc_at = reinterpret_cast<std::uintptr_t*>(
                            entry.offset + reloc_block->base_rva + module_base);
                        *reloc_at = module_base + ((*reloc_at) - image_base);
                        break;
                      }
                      default:
                        break;
                    }
                  });

    reloc_block = reloc_block->next();
  }

  std::printf("> image base = %p, image size = %p, module base = %p\n",
              image_base, image_size, module_base);

  if (!image_base || !image_size || !module_base) {
    std::printf("[!] failed to open binary on disk...\n");
    return -1;
  }

  const auto vm_entry_rva =
      std::strtoull(parser.get<std::string>("vmentry").c_str(), nullptr, 16);

  vm::vmctx_t vmctx(module_base, image_base, image_size, vm_entry_rva);

  if (!vmctx.init()) {
    std::printf(
        "[!] failed to init vmctx... this can be for many reasons..."
        " try validating your vm entry rva... make sure the binary is "
        "unpacked and is"
        "protected with VMProtect 3...\n");
    return -1;
  }

  auto vm_enter = vmctx.get_vm_enter();
  vm::utils::print(vm_enter);

  std::printf("> Starting Virtual Instruction Pointer Register: %s\n",
              ZydisRegisterGetString(vmctx.get_vip()));

  std::printf("> Starting Virtual Stack Pointer Register: %s\n",
              ZydisRegisterGetString(vmctx.get_vsp()));

  // testing vmlocate port for vmp3...
  const auto vm_entries = vm::locate::get_vm_entries(module_base, image_size);
  std::printf("> number of vm entries = %d\n", vm_entries.size());
}