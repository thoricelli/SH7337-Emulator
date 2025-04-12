#include <stddef.h>
#include "endianness.h"

#define ROM_START 0x00300200
#define ROM_END 0x003FFFFF

#define RAM_SIZE 0x80000

#define RAM_START 0x88000000
#define RAM_END RAM_START + RAM_SIZE

#define USER_SPACE_RAM_SIZE 0x8000 //32kb.

#define USER_SPACE_RAM_START 0x8100000
#define USER_SPACE_RAM_END USER_SPACE_RAM_START + USER_SPACE_RAM_SIZE

#define USER_SPACE_RAM_PHYSICAL_OFFSET RAM_SIZE - USER_SPACE_RAM_SIZE

void* ROM;
unsigned char RAM[RAM_SIZE];

void* mmu_translate(unsigned long address) {

	//User-space RAM
	if (address >= USER_SPACE_RAM_START && address <= USER_SPACE_RAM_END)
		return ((void*)((uintptr_t)RAM + USER_SPACE_RAM_PHYSICAL_OFFSET + (address - USER_SPACE_RAM_START)));

	//RAM
	if (address >= RAM_START && address <= RAM_END)
		return ((void*)((uintptr_t)RAM + (address - RAM_START)));

	//ROM
	if (address >= ROM_START && address <= ROM_END)
		return ((void*)((uintptr_t)ROM + (address - ROM_START)));

	printf("Address not found: %lx\n", address);
	return NULL;
}

void set_rom_ptr(const void* ptr) {
	ROM = ptr;
}