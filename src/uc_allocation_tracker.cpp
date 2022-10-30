#include <uc_allocation_tracker.hpp>
#include <cstdio>

int g_allocation_tracker;

uc_err uct_context_alloc(uc_engine *uc, uc_context **context)
{
  ++g_allocation_tracker;
  //std::printf("Allocations: %p\n", g_allocation_tracker);
  return uc_context_alloc(uc, context);
}
uc_err uct_context_free(uc_context *context)
{
  --g_allocation_tracker;
  //std::printf("Allocations: %p\n", g_allocation_tracker);
  return uc_context_free(context);
}

void print_allocation_number()
{
  std::printf("uc_context allocations: %p\n", g_allocation_tracker);
}