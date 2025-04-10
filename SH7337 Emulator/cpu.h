#pragma once

typedef struct {
	unsigned long R[8];
} sh7337_banks_t;

typedef struct {
	unsigned long Reserved : 1;
	
	unsigned long MD : 1;
	unsigned long RB : 1;
	unsigned long BL : 1;
	
	unsigned long Reserved_2 : 18;
	
	unsigned long M : 1;
	unsigned long Q : 1;
	
	unsigned long L3 : 1;
	unsigned long L2 : 1;
	unsigned long L1 : 1;
	unsigned long L0 : 1;

	unsigned long Reserved_3 : 2;

	unsigned long S : 1;
	unsigned long T : 1;

} sh7337_sr;

typedef struct {
	sh7337_banks_t* bank0;

	unsigned long R[8];

	unsigned long PC;
	unsigned long PR;
	unsigned long GBR;
	unsigned long VBR;
	unsigned long MACH;
	unsigned long MACL;
	sh7337_sr SR;

	unsigned long SSR;
	unsigned long SPC;

	sh7337_banks_t* bank1;
} sh7337_t;

typedef struct {
	char paused;
} cpu_running_state_t;


sh7337_t* get_cpu_state();
cpu_running_state_t* get_cpu_running_state();

void pause_cpu();

void set_register(unsigned long value, unsigned char rn);

inline void set_pc_little_endian(unsigned long PC);
void set_pc_little_endian_delay(unsigned long PC);
void set_pc_big_endian(unsigned long PC);

void execute_instruction(unsigned char instruction[2]);
void cpu_tick();