unsigned char checskum256(char* msg) {
	unsigned char sum = 0;

	while (*msg && *msg != '\0') {
		sum += (unsigned char)*msg;
		msg++;
	}

	return sum;
}