/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define BUFSIZE 16
static unsigned char buf[BUFSIZE];

static void usage(const char *progname)
{
	printf("Usage: %s [-f file] [-s start] [-n num_bytes]\n", progname);
	exit(-1);
}

int main(int argc, char *argv[])
{
	const char *file = "boot2.bin";
	FILE *fp = 0;
	int count = 0;
	int i = 0;
	int start = 0;
	unsigned int size = INT_MAX;
	int ch = 0;
	int total = 0;

	while ((ch = getopt(argc, argv, "hf:s:n:")) != -1) {
		switch (ch) {
		case 'f':
			file = strdup(optarg);
			break;
		case 's':
			start = atoi(optarg);
			break;
		case 'n':
			size = atoi(optarg);
			break;
		default:
			fprintf(stderr, "unrecognized option: %c\n", ch);
		case 'h':
			usage(argv[0]);
		}
	}

	fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "unable to open %s: %s\n", file,
			strerror(errno));
		return -1;
	}

	fseek(fp, start, SEEK_SET);
	printf("Dumping %d bytes of %s from offset = %d\n", size, file, start);

	for (;;) {
		count = fread(buf, 1, 16, fp);
		for (i = 0; i < count; i++)
			printf("%02x ", buf[i] & 0xff);
		printf("\n");
		if (count)
			total += count;
		if (total >= size)
			break;
		if (feof(fp) || ferror(fp))
			break;
	}
	if (ferror(fp)) {
		fprintf(stderr, "Unable to read from file\n");
		return -1;
	}
	return 0;
}
