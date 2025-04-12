void to_hex_array(unsigned long value, char* hex_array, int size) {
	static const char hex_digits[] = "0123456789ABCDEF";
	for (int i = size - 1; i >= 0; --i) {
		hex_array[i] = hex_digits[value & 0xF];
		value >>= 4;
	}
}