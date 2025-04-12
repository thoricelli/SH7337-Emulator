//TODO: Port to Linux and MacOS.

#define STARTS_WITH(str, prefix) (strncmp((str), (prefix), sizeof(prefix) - 1) == 0)

#include <stdlib.h>
#include <winsock2.h>
#include <stdio.h>

#include "cpu.h"
#include "mmu.h"
#include "endianness.h"
#include "gdb.h"
#include "string.h"
#include "checksum.h"

SOCKET server_fd, client_fd;
struct sockaddr_in server, client;

cpu_running_state_t* cpu_running_state;

#define LONG_HEX_SIZE 8

char* cpu_state_to_string() {
	sh7337_t cpu_state = *get_cpu_state();

	unsigned int bank0Size = sizeof(cpu_state.bank0->R) / sizeof(unsigned long);
	unsigned int rSize = sizeof(cpu_state.R) / sizeof(unsigned long);

	unsigned int hex_array_size = (bank0Size + rSize + 9) * LONG_HEX_SIZE;

	unsigned int index = 0;
	char* string = malloc(hex_array_size);

	//Bank 0 registers (R0-R6)
	for (unsigned int i = 0; i < bank0Size; i++)
	{
		to_hex_array(cpu_state.bank0->R[i], string + index + (i * LONG_HEX_SIZE), 8);
	}

	index += (bank0Size * LONG_HEX_SIZE);

	//Registers (R7-R14)
	for (unsigned int i = 0; i < rSize; i++)
	{
		to_hex_array(cpu_state.R[i], string + index + (i * LONG_HEX_SIZE), 8);
	}

	index += (rSize * LONG_HEX_SIZE);

	unsigned long* states = &cpu_state.PC;

	for (unsigned int i = 0; i < 9; i++)
	{
		to_hex_array(states[i], string + index, 8);

		index += LONG_HEX_SIZE;
	}

	string[hex_array_size] = '\0';

	return string;
}

char* cpu_memory_to_string(unsigned long address, unsigned int size) {
	mmu_translate(address);
}

char* cpu_get_memory(char* message) {
	//First value before the comma is the address
	//Value after comma is the length.

	char* size_ptr = (char*)strchr(message, ',') + 1;

	//Get the size of the memory needed, then malloc a string.
	unsigned long address = strtoul(message, NULL, 16);
	unsigned int size = (unsigned int)strtoul(size_ptr, NULL, 16);

	char* buffer = malloc(size * 8);

	unsigned long* memory = mmu_translate(address);

	if (memory != NULL) {
		unsigned int i = 0;

		for (i = 0; i < size; i++)
		{
			unsigned char value = *((unsigned char*)memory + i);

			to_hex_array(value, buffer + (i * 2), 2);
		}

		buffer[size * 2] = '\0';
	}
	else {
		buffer[0] = '\0';
	}

	return buffer;
}

//Well todo, is change the way breakpoints work
//I mean, currently it isn't a 1 by 1 simulation of the actualy CPU, thats done later
//But removing a breakpoint causes some unwanted behaviour, this is because it removes a breakpoint somewhere in the list.
//I should probably, when adding a breakpoint just loop to see which address is zeroed out.

void gdb_remove_breakpoint(const char* message) {
	char* size_ptr = strchr(message, ',');

	unsigned long address = strtoul(message, NULL, 16);

	char found = 0;
	size_t index = 0;

	while (index < cpu_running_state->breakpoint_index && found == 0) {
		if (cpu_running_state->breakpoints[index] == address)
			found = 1;

		index++;
	}

	if (found) {
		cpu_running_state->breakpoint_index--;
		cpu_running_state->breakpoints[index] = 0;
	}
}

void gdb_add_breakpoint(const char* message) {
	char* size_ptr = strchr(message, ',');

	cpu_running_state->breakpoints[cpu_running_state->breakpoint_index] = strtoul(message, NULL, 16);

	if (sizeof(cpu_running_state->breakpoints) / sizeof(unsigned long) > cpu_running_state->breakpoint_index)
		cpu_running_state->breakpoint_index++;
}

void send_raw(const char* message) {
	printf("<- %s\n", message);
	send(client_fd, message, strlen(message), 0);
}

void gdb_send_received() {
	send_raw("+");
}

void gdb_send_message(const char* response) {
	unsigned char response_buffer[1024];
	memcpy(response_buffer, "+$", 2);

	unsigned int size = strlen(response);

	if (size > 0)
		memcpy(response_buffer + 2, response, size);

	*(response_buffer + 2 + size) = '#';

	unsigned char checksum = checskum256(response);
	sprintf_s(response_buffer + 3 + size, 4, "%02X", checksum);

	*(response_buffer + 5 + size) = '\0';

	send_raw(response_buffer);
}

char* gdb_get_signal_string(posix_signal signal) {
	char string[4];
	string[0] = 'S';

	to_hex_array(signal, string + 1, 2);

	string[3] = '\0';

	return string;
}

void gdb_send_interrupt(posix_signal signal) {
	char* interrupt_str = gdb_get_signal_string(signal);

	gdb_send_message(interrupt_str);
}

char* gdb_get_response(char* command) {
	if (STARTS_WITH(command, "?")) {
		return gdb_get_signal_string(SIGTRAP);
	}
	else if (STARTS_WITH(command, "c") || STARTS_WITH(command, "Hc0")) {
		cpu_running_state->step = 0;
		cpu_running_state->paused = 0;
		return "OK";
	}
	else if (STARTS_WITH(command, "g")) {
		return cpu_state_to_string();
	}
	else if (STARTS_WITH(command, "qfThreadInfo")) {
		return "l";
	}
	else if (STARTS_WITH(command, "qAttached")) {
		return "1";
	}
	else if (STARTS_WITH(command, "m")) {
		return cpu_get_memory(command + 1);
	}
	else if (STARTS_WITH(command, "D")) {
		return "OK";
	}
	else if (STARTS_WITH(command, "S04") || STARTS_WITH(command, "s")) {
		cpu_running_state->step = 1;
		cpu_running_state->paused = 0;
		return "OK";
	}
	else if (STARTS_WITH(command, "Z0")) {
		gdb_add_breakpoint(command + 3);
		return "OK";
	}
	else if (STARTS_WITH(command, "z0")) {
		gdb_remove_breakpoint(command + 3);
		return "OK";
	}
	else {
		return "\0";
	}
}

char is_socket_connected() {
	char buf;
	int result = recv(client_fd, &buf, 1, MSG_PEEK);
	if (result > 0) return 1;
	if (result == 0) return 0;
	if (errno == EAGAIN || errno == EWOULDBLOCK) return 1;
	return 0;
}

// https://sourceware.org/gdb/current/onlinedocs/gdb.html/Overview.html#Overview

void gdb_message_received(char* buffer) {
	unsigned int i = 0;

	printf("-> %s\n", buffer);

	unsigned int response_index = 0;

	//First get the ack.
	char ack = buffer[0];

	//TODO: Retransmit when - received!

	//Find the dollar sign index.
	char* dollar = strchr(buffer + i, '$');

	if (dollar == NULL || *dollar != '$') {
		gdb_send_received("+");
		return;
	}

	//Temporarily blocking out the checksum.
	*(strchr(buffer, '#')) = '\0';

	unsigned char* response = gdb_get_response(dollar + 1);

	gdb_send_message(response);
	return;
}

void gdb_start() {

	WSADATA wsa;

	WSAStartup(MAKEWORD(2, 2), &wsa);
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(1234);

	bind(server_fd, (struct sockaddr*)&server, sizeof(server));
	listen(server_fd, 3);

	printf("Waiting for connection...\n");
	int c = sizeof(struct sockaddr_in);
	client_fd = accept(server_fd, (struct sockaddr*)&client, &c);
	printf("Client connected.\n");

	cpu_running_state = get_cpu_running_state();

	char buffer[1024];

	while (1) {
		memset(buffer, 0, sizeof(buffer));
		int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

		if (bytes <= 0) break;

		gdb_message_received(buffer);
	}

	closesocket(client_fd);
	closesocket(server_fd);
	WSACleanup();

	return 0;
}

void gdb_loop() {
	while (1) {
		gdb_start();
	}
}