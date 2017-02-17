#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

const size_t HEADER_SIZE = 17;

typedef struct {
	uint32_t width;
	uint32_t height;
} Dim;

void usage(const char *name)
{
	fprintf(stderr, "usage: %s [-e or -p] header_path\n", name);
}

Dim header(FILE *header_file, bool extract)
{
	FILE *ifile = extract? stdin : header_file;
	FILE *ofile = extract? header_file : stdout;

	uint8_t buf[HEADER_SIZE];
	fread(buf, 1, HEADER_SIZE, ifile);
	fwrite(buf, 1, HEADER_SIZE, ofile);

	Dim d;
	d.width = ntohl((buf[9] << 0)   |
			(buf[10] << 8)  |
			(buf[11] << 16) |
			(buf[12] << 24));
	d.height = ntohl((buf[13] << 0)  |
			 (buf[14] << 8)  |
			 (buf[15] << 16) |
			 (buf[16] << 24));

	return d;
}

void stream(Dim d, bool extract)
{
	size_t count = 0;
	size_t nread = 0;

	uint8_t buf[4];
	buf[3] = 255;

	size_t nbytes_r = extract? 4 : 3;
	size_t nbytes_w = extract? 3 : 4;

	while (count < d.width * d.height &&
	       (nread = fread(buf, 1, nbytes_r, stdin))) {
		fwrite(buf, 1, nbytes_w, stdout);
		count++;
	}

	if (count < d.width * d.height) { // too few pixels
		fprintf(stderr, "WARNING: missing pixels, filled with black\n");
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0;
		for (; count < d.width * d.height; count++)
			fwrite(buf, 1, nbytes_w, stdout);
	}
}

int main(int argc, const char *argv[])
{
	if (argc != 3) {
		usage(argv[0]);
		return 1;
	}

	const char *command = argv[1];
	const char *header_path = argv[2];
	bool extract;

	if (strcmp(command, "-e") == 0)
		extract = true;
	else if (strcmp(command, "-p") == 0)
		extract = false;
	else {
		usage(argv[0]);
		return 2;
	}

	const char *mode = extract? "w" : "r";
	FILE *header_file = fopen(header_path, mode);
	if (!header_file) {
		fprintf(stderr, "%s: %s\n", header_path, strerror(errno));
		return 3;
	}

	stream(header(header_file, extract), extract);

	fclose(header_file);

	return 0;
}
