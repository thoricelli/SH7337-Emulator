#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define HEADER_SIZE 0x200

void* load_file(const char* filename) {
    FILE* handle = fopen(filename, "rb");

    struct stat st;

    stat(filename, &st);

    void* memory = malloc(st.st_size - HEADER_SIZE);

    fseek(handle, 0x200, SEEK_SET);
    fread(memory, 1, st.st_size - HEADER_SIZE, handle);

    fclose(handle);

    return memory;
}