/*
Instructions have a MSB and a LSB.
Most Significant Bit(s) and Least Significant Bit(s).

You can check the instruction table on the sh7720 documentation:
https://bible.planet-casio.com/common/hardware/mpu/sh7720.pdf#page=137
*/
#include <stdio.h>
#include "endianness.h"
#include "cpu.h"
#include "mmu.h"
#include "syscall_map.h"
#include "gdb.h"

#define MSB(x) (x[0] & 0xF0) >> 4

#define FIRST_OPERAND(x) x[0] & 0x0F
#define SECOND_OPERAND(x) (x[1] & 0xF0) >> 4

#define LSB(x) x[1] & 0x0F

#define DISP(x) ((x[0] & 0x0F) << 8) | x[1]

#define SECOND_FULL(x) x[1]

#define SECOND_FIRST_OPERAND(x) (x[1] & 0xF0) >> 4
#define SECOND_SECOND_OPERAND(x) x[1] & 0x0F

sh7337_banks_t bank0;
sh7337_banks_t bank1;

sh7337_t cpu_state =
{
	.bank0 = &bank0,
	.bank1 = &bank1
};

cpu_running_state_t running_state;

void* delay_slot;

sh7337_t* get_cpu_state() {
	return &cpu_state;
}

cpu_running_state_t* get_cpu_running_state() {
	return &running_state;
}

void pause_cpu() {
	running_state.paused = 1;
}

void step_cpu() {
	running_state.step = 1;
}

void set_register(unsigned long value, unsigned char rn) {
	if (rn < 8)
		cpu_state.bank0->R[rn] = value;
	else
		cpu_state.R[rn - 0x8] = value;
}

unsigned long get_register(unsigned char rn) {
	return rn < 8 ? cpu_state.bank0->R[rn] : cpu_state.R[rn - 0x8];
}

inline void increase_pc() {
	cpu_state.PC += 2;
}

inline void increase_pc_2() {
	cpu_state.PC += 4;
}

void illegal_instruction(unsigned char instruction[2]) {
	printf("Instruction %x%x not implemented.\n", instruction[0], instruction[1]);
	printf("PC: %lx\n", cpu_state.PC);

	pause_cpu();
	gdb_send_interrupt(SIGILL);
}

int sign_extend_int(int val) {
	if (val & 0x800)
		return 0xFFFFF000 | val;
	else
		return val & 0x0FFF;
}

int sign_extend_byte(unsigned char val) {
	if ((val & 0x80) == 0)
		return (0x000000FF & val);
	else
		return (0xFFFFFF00 | val);
}

/*
* INSTRUCTIONS
*/

inline void NOP() {
	//Does absolutely nothing!

	increase_pc();
}

inline void CMPGT_rm_rn(char rm, char rn) {
	cpu_state.SR.T = get_register(rn) > get_register(rm);

	increase_pc();
}

inline void CMPPL_rn(unsigned char rn) {
	cpu_state.SR.T = get_register(rn) > 0;

	increase_pc();
}

inline void CMPHS_rm_rn(unsigned char rm, unsigned char rn) {
	//When Rn is higher or equal to Rm, we set the T bit to 1.
	cpu_state.SR.T = get_register(rn) >= get_register(rm);

	increase_pc();
}

inline void STSL_rn(unsigned char rn) {
	//Subtracts 4 from Rn, sets it to Rn, sets value of PR to @Rn.
	set_register(rn, get_register(rn) - 4);

	*((unsigned long*)mmu_translate(get_register(rn))) = swap_uint32(cpu_state.PR);

	increase_pc();
}

inline void MOV_rm_rn(unsigned char rm, unsigned char rn) {
	//Sets value of RM to the value of register Rn.
	set_register(get_register(rn), rm);

	increase_pc();
}

inline void MOVL_at_rm_rn(unsigned char rm, unsigned char rn) {
	//Sets the value @Rm to Rn.
	set_register(swap_uint32(*((unsigned long*)mmu_translate(get_register(rm)))), rn);

	increase_pc();
}

inline void MOVL_rm_at_rn(unsigned char rm, unsigned char rn) {
	//Sets value of Rm to @Rn
	*((unsigned long*)mmu_translate(get_register(rn))) = swap_uint32(get_register(rm));

	increase_pc();
}

inline void MOVL_rn_disp(unsigned char rn, unsigned short disp) {
	//Moves value at PC + disp to register number RN.
	set_register(
		swap_uint32(*((unsigned long*)mmu_translate((cpu_state.PC + 4 + ((unsigned long)disp << 2)) & ~0x3))),
		rn
	);

	increase_pc();
}

inline void MOVW_disp_rm(unsigned char disp, unsigned char Rm) {
	//Sets register 0 to disp * 2 + value in Rm.

	set_register(0, (disp * 2 + get_register(Rm)));

	increase_pc();
}

inline void MOVW_rn_disp(unsigned char rn, unsigned char disp) {
	set_register(disp * 2 + get_register(rn), 0);

	increase_pc();
}

inline void MOV_rn_imm(unsigned char rn, unsigned short imm) {
	set_register(imm, rn);

	increase_pc();
}

inline void LDS_at_rm_pr(unsigned char rm) {
	cpu_state.PR = swap_uint32(*((unsigned long*)mmu_translate(get_register(rm))));
	set_register(get_register(rm) + 4, rm);

	increase_pc();
}

inline RTS() {
	set_pc_little_endian_delay(cpu_state.PR);
}

inline void BSR(int disp) {
	//Takes disp * 2 and adds that to the PC + 4.
	//Then it sets the PC.
	cpu_state.PR = cpu_state.PC + 4;

	set_pc_little_endian_delay(cpu_state.PC + 4 + (sign_extend_int(disp) << 1));
}

inline void BRA(int disp) {
	set_pc_little_endian_delay(cpu_state.PC + 4 + (sign_extend_int(disp) << 1));
}

inline void JSR(unsigned char rm) {
	//Sets the PR to the PC + 4
	//Then it sets the PC to the value in Rm.
	cpu_state.PR = cpu_state.PC + 4;

	set_pc_little_endian_delay(get_register(rm));
}

inline void JMP_rn(unsigned char rn) {
	//Jumps to address stored in RN.
	set_pc_little_endian_delay(get_register(rn));
}

inline void BTS(unsigned char disp) {
	if (cpu_state.SR.T)
		set_pc_little_endian_delay(cpu_state.PC + 4 + (sign_extend_byte(disp) << 1));
	else
		increase_pc_2();
}

inline void BT(unsigned char disp) {
	if (cpu_state.SR.T)
		set_pc_little_endian(cpu_state.PC + 4 + (sign_extend_byte(disp) << 1));
	else
		increase_pc();
}

inline void BF(unsigned char disp) {
	if (cpu_state.SR.T == 0)
		set_pc_little_endian_delay(cpu_state.PC + 4 + (sign_extend_byte(disp) << 1));
	else
		increase_pc_2();
}

inline void SUB_rm_rn(unsigned char rm, unsigned char rn) {
	set_register(get_register(rn) - get_register(rm), rn);

	increase_pc();
}

inline void ADD_rn_imm(unsigned char rn, char imm) {
	//Adds value in register Rn + imm and sets the result to Rn.
	set_register(get_register(rn) + imm, rn);

	increase_pc();
}

inline void EXTUW_rm_rn(unsigned char rm, unsigned char rn) {
	//Zero extends value in rm, then sets it to rn.
	set_register(get_register(rm) & 0x0000FFF, rn);

	increase_pc();
}

/*
* LOOKUP
*/

inline void LSB_0100_0010(unsigned char instruction[2]) {
	switch (SECOND_OPERAND(instruction)) {
		case 0b0010:
			STSL_rn(FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void LSB_0100_1011(unsigned char instruction[2]) {
	switch (SECOND_OPERAND(instruction))
	{
		case 0b0000:
			JSR(FIRST_OPERAND(instruction));
			break;
		case 0b0010:
			JMP_rn(FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline LSB_0000_1011(unsigned char instruction[2]) {
	switch (SECOND_OPERAND(instruction)) {
		case 0b0000:
			RTS();
			break;
	}
}

inline void LSB_0000_1001(unsigned char instruction[2]) {
	if (SECOND_OPERAND(instruction) == 0) {
		NOP();
	}
	else {
		//DIV0U
	}
}

inline void LSB_0100_0101(unsigned char instruction[2]) {
	switch (SECOND_OPERAND(instruction)) {
		case 0b0001:
			CMPPL_rn(FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void LSB_0100_0110(unsigned char instruction[2]) {
	switch (SECOND_OPERAND(instruction)) {
		case 0b0010:
			LDS_at_rm_pr(FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void MSB_0000(unsigned char instruction[2]) {
	switch (LSB(instruction)) {
		case 0b1001:
			LSB_0000_1001(instruction);
			break;
		case 0b1011:
			LSB_0000_1011(instruction);
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void MSB_0010(unsigned char instruction[2]) {
	switch (LSB(instruction))
	{
		case 0b0010:
			MOVL_rm_at_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void MSB_0011(unsigned char instruction[2]) {
	switch (LSB(instruction)) {
		case 0b0010:
			CMPHS_rm_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		case 0b0111:
			CMPGT_rm_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		case 0b1000:
			SUB_rm_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void MSB_0100(unsigned char instruction[2]) {
	//Get the LSB.
	switch (LSB(instruction))
	{
		case 0b0010:
			LSB_0100_0010(instruction);
			break;
		case 0b0110:
			LSB_0100_0110(instruction);
			break;
		case 0b0101:
			LSB_0100_0101(instruction);
			break;
		case 0b1011:
			LSB_0100_1011(instruction);
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

#define MOV_LSB_RM_RN 0b0011
#define MOVL_LSB_AT_RM_RN 0b0010
#define EXTUW_RM_RN 0b1101

inline void MSB_0110(unsigned char instruction[2]) {
	switch (LSB(instruction))
	{
		case MOVL_LSB_AT_RM_RN:
			MOVL_at_rm_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		case MOV_LSB_RM_RN:
			MOV_rm_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		case EXTUW_RM_RN:
			EXTUW_rm_rn(SECOND_OPERAND(instruction), FIRST_OPERAND(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

inline void MSB_1000(unsigned char instruction[2]) {
	switch (FIRST_OPERAND(instruction)) {
		case 0b0001:
			MOVW_rn_disp(SECOND_FIRST_OPERAND(instruction), SECOND_SECOND_OPERAND(instruction));
			break;
		case 0b0101:
			MOVW_disp_rm(SECOND_FIRST_OPERAND(instruction), SECOND_SECOND_OPERAND(instruction));
			break;
		case 0b1001:
			BT(SECOND_FULL(instruction));
			break;
		case 0b1011:
			BF(SECOND_FULL(instruction));
			break;
		case 0b1101:
			BTS(SECOND_FULL(instruction));
			break;
		default:
			illegal_instruction(instruction);
			break;
	}
}

/*
* CPU
*/

inline void set_pc_little_endian(unsigned long PC) {
	cpu_state.PC = PC;
}

void set_pc_little_endian_delay(unsigned long PC) {
	delay_slot = mmu_translate(cpu_state.PC + 2);

	set_pc_little_endian(PC);
}

void set_pc_big_endian(unsigned long PC) {
	PC = swap_uint32(PC);

	set_pc_little_endian(PC);
}

#define BRA_MSB 0b1010
#define MOVL_MSB 0b1101
#define MOV_MSB 0b1110

//Instruction is a big endian short. We use a char array here for easy access.
void execute_instruction(unsigned char instruction[2]) {

	switch (MSB(instruction))
	{
		case 0b0000:
			MSB_0000(instruction);
			break;
		case 0b0010:
			MSB_0010(instruction);
			break;
		case 0b0011:
			MSB_0011(instruction);
			break;
		case 0b0100:
			MSB_0100(instruction);
			break;
		case 0b0110:
			MSB_0110(instruction);
			break;
		case 0b0111:
			ADD_rn_imm(FIRST_OPERAND(instruction), SECOND_FULL(instruction));
			break;
		case 0b1000:
			MSB_1000(instruction);
			break;
		case BRA_MSB:
			BRA(DISP(instruction));
			break;
		case 0b1011:
			BSR(DISP(instruction));
			break;
		case MOVL_MSB:
			MOVL_rn_disp(FIRST_OPERAND(instruction), SECOND_FULL(instruction));
			break;
		case MOV_MSB:
			MOV_rn_imm(FIRST_OPERAND(instruction), SECOND_FULL(instruction));
			break;
		default:
			illegal_instruction(instruction);
		break;
	}
}

void step_logic() {
	//Check for step instruction
	if (running_state.step) {
		running_state.paused = 1;
		running_state.step = 0;
	}

	gdb_send_interrupt(SIGTRAP);
}

void breakpoint_check() {
	unsigned char index = 0;
	unsigned char breakpoint_hit = 0;

	while (running_state.breakpoint_index > index && breakpoint_hit == 0) {
		if (cpu_state.PC == running_state.breakpoints[index])
			breakpoint_hit = 1;

		index++;
	}

	if (breakpoint_hit) {
		pause_cpu();
		gdb_send_interrupt(SIGTRAP);
	}
}

#define SYSCALL 0x80010070

unsigned long previousPC;

//TODO: Add execution states.
void cpu_tick() {
	if (delay_slot == NULL) {

		if (cpu_state.PC != SYSCALL) {
			unsigned long* address = mmu_translate(cpu_state.PC);

			if (address != NULL) {

				previousPC = cpu_state.PC;
				execute_instruction(address);

				//When stepping using GDB.
				if (running_state.step)
					step_logic();

				breakpoint_check();
			}
			else {
				//Address at PC was not found, reverting to the previous PC.
				set_pc_little_endian(previousPC);
				pause_cpu();
				gdb_send_interrupt(SIGSEGV);
			}
		}
		else
			//When we jump to 0x80010070, in headless, that means we're trying to access a syscall.
			execute_syscall(&cpu_state);
	}
	else {
		//Delay slot for actions that set the PC.
		unsigned long oldPC = cpu_state.PC;

		execute_instruction(delay_slot);

		cpu_state.PC = oldPC;
		delay_slot = NULL;
	}
}