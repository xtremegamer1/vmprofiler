#include <uc_allocation_tracker.hpp>
#include <cstdio>

int g_allocation_tracker;

uc_err uct_context_alloc(uc_engine *uc, uc_context **context)
{
  std::printf("Allocations: %p\n", ++g_allocation_tracker);
  return uc_context_alloc(uc, context);
}
uc_err uct_context_free(uc_context *context)
{
  std::printf("Allocations: %p\n", --g_allocation_tracker);
  return uc_context_free(context);
}