#pragma once
#include <unicorn\unicorn.h>

extern int g_allocation_tracker;

uc_err uct_context_alloc(uc_engine *uc, uc_context **context);
uc_err uct_context_free(uc_context *context);
void print_allocation_number();