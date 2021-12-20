#include <vmctx.hpp>

namespace vm
{
    ctx_t::ctx_t( std::uintptr_t module_base, std::uintptr_t image_base, std::uintptr_t image_size,
                  std::uintptr_t vm_entry_rva )
        : module_base( module_base ), image_base( image_base ), image_size( image_size ), vm_entry_rva( vm_entry_rva )
    {
    }

    bool ctx_t::init()
    {
        if ( !vm::util::flatten( vm_entry, vm_entry_rva + module_base ) )
            return false;

        vm::util::deobfuscate( vm_entry );
        return true;
    }
} // namespace vm