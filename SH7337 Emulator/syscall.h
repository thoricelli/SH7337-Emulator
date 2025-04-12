#pragma once

#include "cpu.h"

void Bdisp_AllClr_DDVRAM();

void raw_locate(sh7337_t* cpu_state);
void raw_Print(sh7337_t* cpu_state);
void raw_GetKey(sh7337_t* cpu_state);