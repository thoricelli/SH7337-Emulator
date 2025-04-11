//TODO: Port to Linux and MacOS.

#define STARTS_WITH(str, prefix) (strncmp((str), (prefix), sizeof(prefix) - 1) == 0)

#include <stdlib.h>
#include <winsock2.h>
#include <stdio.h>

#include "cpu.h"
#include "mmu.h"
#include "endianness.h"

SOCKET server_fd, client_fd;
struct sockaddr_in server, client;

cpu_running_state_t* cpu_running_state;

#define LONG_HEX_SIZE 8

void to_hex_array(unsigned long value, char* hex_array, int size) {
	static const char hex_digits[] = "0123456789ABCDEF";
	for (int i = size - 1; i >= 0; --i) {
		hex_array[i] = hex_digits[value & 0xF];
		value >>= 4;
	}
}

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
		to_hex_array(swap_uint32(cpu_state.bank0->R[i]), string + index + (i * LONG_HEX_SIZE), 8);
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

	char* size_ptr = (intptr_t)message;

	while (*size_ptr != '/0' && *size_ptr != ',') {
		size_ptr++;
	}

	*size_ptr = '\0';
	size_ptr++;

	//Get the size of the memory needed, then malloc a string.
	unsigned long address = strtoul(message, NULL, 16);
	unsigned int size = (unsigned int)strtoul(size_ptr, NULL, 16);

	char* buffer = malloc(size * 8);

	unsigned long* memory = mmu_translate(address);

	unsigned int i = 0;

	for (i = 0; i < size; i++)
	{
		unsigned char value = *((unsigned char*)memory + i);

		to_hex_array(value, buffer + (i * 2), 2);
	}

	buffer[size * 2] = '\0';

	return buffer;
}

unsigned char checskum256(char* msg) {
	unsigned char sum = 0;

	while (*msg && *msg != '\0') {
		sum += (unsigned char)*msg;
		msg++;
	}

	return sum;
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

char* gdb_get_response(char* command) {
	if (STARTS_WITH(command, "?")) {
		return "S00";
	}
	else if (STARTS_WITH(command, "c")) {
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

	//First char is always the ack.
	char ack = buffer[0];

	//TODO: Retransmit when - received!
	if (ack == '+')
		i++;
	else if (ack == '-')
		i += 2;

	//Then we require a dollar sign.
	char leading = buffer[i];

	if (leading != '$') {
		gdb_send_received("+");
		return;
	}

	i++;

	unsigned hashtagIndex = i;

	while(buffer[hashtagIndex] != '\0') {
		if (buffer[hashtagIndex] == '#') {
			buffer[hashtagIndex] = '\0';
		}
		else {
			hashtagIndex++;
		}
	}

	unsigned char* response = gdb_get_response(buffer + i);

	gdb_send_message(response);
	return;
}

void gdb_SIGILL() {
	gdb_send_message("S04");
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