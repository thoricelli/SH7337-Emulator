#include "cpu.h"
#include "mmu.h"
#include "vram.h"

#define ARG_1 0x4
#define ARG_2 0x5

typedef struct {
	int column;
	int row;
} locate_t;

locate_t current_locate;


/*
* Syscalls
*/

void Bdisp_AllClr_DDVRAM() {
	clear_vram();
}

inline void locate(int column, int row) {
	current_locate.column = column;
	current_locate.row = row;
}

inline void Print(unsigned char* string) {
	printf(string);
}

inline void GetKey(unsigned int* keycode) {
	*keycode = 31;
}

/*
* RAW
*/

void raw_Print(sh7337_t* cpu_state) {
	Print(mmu_translate(get_register(ARG_1)));
}

void raw_locate(sh7337_t* cpu_state) {
	locate(get_register(ARG_1), get_register(ARG_2));
}

void raw_GetKey(sh7337_t* cpu_state) {
	GetKey(mmu_translate(get_register(ARG_1)));
}