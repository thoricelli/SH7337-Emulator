#pragma once

#include "cpu.h"

typedef void (fx_func_t)(sh7337_t *cpu_state);

typedef struct {
	char* name;
	fx_func_t* function;
} fx_syscall_t;

void execute_syscall(sh7337_t *cpu_state);