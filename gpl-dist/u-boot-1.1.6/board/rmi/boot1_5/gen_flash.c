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
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <limits.h>

#include "elf.h"

#define dbg_msg(fmt, args...) printf(fmt, ##args)

#define swap16(x) ntohs((x))
#define swap32(x) ntohl((x))
#define swap64(x) ( swap32((uint64_t)((x) & 0xffffffff) << 32) | (uint64_t)swap32((x)>>32) )

#define elftohost16(x, littleendian) ((littleendian)?(x):swap16(x))
#define elftohost32(x, littleendian) ((littleendian)?(x):swap32(x))
#define elftohost64(x, littleendian) ((littleendian)?(x):swap64(x))

#define hosttotarget16(x, littleendian) ((littleendian)?(x):swap16(x))
#define hosttotarget32(x, littleendian) ((littleendian)?(x):swap32(x))
#define hosttotarget64(x, littleendian) ((littleendian)?(x):swap64(x))

static void download_elf32(FILE * ifp, FILE * ofp, int le,
			   unsigned long flash_base, int flash_size)
{
	Elf32_Ehdr ehdr32;
	Elf32_Phdr phdr32;
	int i, j, dwords;
	int offset = 0;
	int size = 0;
	int count = 0;
	char buf[8];
	int remainder = 0;
	unsigned long paddr = 0;
	unsigned long flash_start = flash_base + flash_size;
	unsigned long flash_end = flash_base;

	printf("download_elf32 : little_endian = %d\n", le);

	size = sizeof(Elf32_Ehdr);
	offset = 0;
	fseek(ifp, offset, SEEK_SET);
	count = fread(&ehdr32, size, 1, ifp);
	if (count != 1)
		goto read_error;

	/* Parse the program headers */
	for (i = 0; i < elftohost16(ehdr32.e_phnum, le); i++) {

		offset = (elftohost32(ehdr32.e_phoff, le) +
			  (i * elftohost16(ehdr32.e_phentsize, le))
		    );
		size = sizeof(Elf32_Phdr);

		printf("\n");

		fseek(ifp, offset, SEEK_SET);
		count = fread(&phdr32, size, 1, ifp);
		if (count != 1)
			goto read_error;

		if (elftohost32(phdr32.p_type, le) != PT_LOAD)
			continue;

		offset = elftohost32(phdr32.p_offset, le);
		paddr = (elftohost32(phdr32.p_paddr, le) & 0x1fffffff);
		size = elftohost32(phdr32.p_filesz, le);

		dbg_msg("Loading program header with paddr = %lx, size = %x\n",
			paddr, elftohost32(phdr32.p_filesz, le));

		fseek(ifp, offset, SEEK_SET);

		if (!(paddr >= flash_base && paddr < flash_base + flash_size
		      && paddr + elftohost32(phdr32.p_memsz,
					     le) < flash_base + flash_size)) {
			fprintf(stderr,
				"program header does not fit in the flash region! "
				"paddr=%lx, psize=%x, flash_base=%lx, flash_size=%x\n",
				paddr, elftohost32(phdr32.p_memsz, le),
				flash_base, flash_size);
			exit(-1);
		} else {
			/* update flash_start and flash_end */
			if (paddr < flash_start)
				flash_start = paddr;
			if (paddr + elftohost32(phdr32.p_memsz, le) > flash_end)
				flash_end =
				    paddr + elftohost32(phdr32.p_memsz, le);
		}

		fseek(ofp, paddr - flash_base, SEEK_SET);

		dbg_msg("ofp file pos = 0x%lx\n", ftell(ofp));
		dwords = size >> 3;
		for (j = 0; j < dwords; j++) {

			count = fread(buf, 8, 1, ifp);
			if (count != 1)
				goto read_error;

			count = fwrite(buf, 8, 1, ofp);
			if (count != 1)
				goto write_error;

		}

		remainder = size & 0x07;
		printf("size = %d, dwords = %d, remainder = %d\n", size, dwords,
		       remainder);

		if (remainder) {
			count = fread(buf, remainder, 1, ifp);
			if (count != 1)
				goto read_error;

			memset(&buf[remainder], 0x00, 8 - remainder);

			count = fwrite(buf, 8, 1, ofp);
			if (count != 1)
				goto write_error;

			dwords++;
		}

		if (elftohost32(phdr32.p_filesz, le) <
		    elftohost32(phdr32.p_memsz, le)) {

			size =
			    elftohost32(phdr32.p_memsz,
					le) - elftohost32(phdr32.p_filesz, le);
			size -= (remainder ? (8 - remainder) : 0);

			for (j = 0; j < (size / 8); j++) {
				memset(buf, 0x00, 8);
				count = fwrite(buf, 8, 1, ofp);
				if (count != 1)
					goto write_error;
			}
			dwords += (size >> 3);
			remainder = size & 0x07;

			printf("size = %d, dwords = %d, remainder = %d\n", size,
			       dwords, remainder);
			if (remainder) {
				memset(&buf[remainder], 0x00, 8 - remainder);
				count = fwrite(buf, 8, 1, ofp);
				if (count != 1)
					goto write_error;
				dwords++;
			}
		}
	}

	printf
	    ("flash_base=%lx, flash_size=%x, flash_start=%lx, flash_end=%lx\n",
	     flash_base, flash_size, flash_start, flash_end);
	if ((flash_end & 0x1ffff) != 0)
		flash_end = (flash_end + 0x20000) & ~0x1ffff;
	printf
	    ("Number of flash sectors(128k size) = %ld, aligned end address = %lx\n",
	     (flash_end - flash_base) >> 17, flash_end);
	if (ftruncate(fileno(ofp), (int)(flash_end - flash_base)) < 0) {
		fprintf(stderr, "Unable to truncate the output file\n");
	}
	return;

      read_error:
	fprintf(stderr, "file read error: %s\n", strerror(errno));
	abort();
      write_error:
	fprintf(stderr, "file write error: %s\n", strerror(errno));
	abort();
}

static void download_elf64(FILE * ifp, FILE * ofp, int le, unsigned long base,
			   int size)
{
#if 0
	Elf64_Ehdr ehdr;
	Elf64_Phdr phdr;
	int i, j, dwords;
	int offset = 0;
	int size = 0;
	int count = 0;
	char buf[8];
	int remainder = 0;

	printf("download_elf64 : little_endian = %d\n", le);
	printf("sizeof(reloc_tbl) = %d\n", sizeof(struct reloc_tbl));

	size = sizeof(ehdr);
	offset = 0;
	fseek(ifp, offset, SEEK_SET);
	count = fread(&ehdr, size, 1, ifp);
	if (count != 1)
		goto read_error;

	/* Copy the start address */
	memcpy(&table.start.lo, (char *)&ehdr.e_entry + 4, 4);
	memcpy(&table.start.hi, &ehdr.e_entry, 4);

	dbg_msg("entry = %llx\n", elftohost64(ehdr.e_entry, le));

	fseek(ofp, sizeof(struct reloc_tbl), SEEK_SET);

	/* Parse the program headers */
	for (i = 0; i < elftohost16(ehdr.e_phnum, le); i++) {

		offset = (elftohost64(ehdr.e_phoff, le) +
			  (i * elftohost16(ehdr.e_phentsize, le))
		    );
		size = sizeof(phdr);

		printf("\n");

		fseek(ifp, offset, SEEK_SET);
		count = fread(&phdr, size, 1, ifp);
		if (count != 1)
			goto read_error;

		if (elftohost32(phdr.p_type, le) != PT_LOAD)
			continue;

		offset = elftohost64(phdr.p_offset, le);
		/* copy the virtual address */
		memcpy(&table.entries[i].addr.hi, &phdr.p_vaddr, 4);
		memcpy(&table.entries[i].addr.lo, (char *)&phdr.p_vaddr + 4, 4);

		size = elftohost64(phdr.p_filesz, le);

		dbg_msg
		    ("Loading program header with vaddr = %llx, size = %llx\n",
		     elftohost64(phdr.p_vaddr, le), elftohost64(phdr.p_filesz,
								le));

		fseek(ifp, offset, SEEK_SET);
		dbg_msg("ofp file pos = %ld\n", ftell(ofp));
		for (j = 0; j < (size / 8); j++) {

			count = fread(buf, 8, 1, ifp);
			if (count != 1)
				goto read_error;

			count = fwrite(buf, 8, 1, ofp);
			if (count != 1)
				goto write_error;

		}
		dwords = (size >> 3);
		remainder = size & 0x07;

		printf("size = %d, dwords = %d, remainder = %d\n", size, dwords,
		       remainder);

		if (remainder) {
			count = fread(buf, remainder, 1, ifp);
			if (count != 1)
				goto read_error;

			memset(&buf[remainder], 0x00, 8 - remainder);

			count = fwrite(buf, 8, 1, ofp);
			if (count != 1)
				goto write_error;

			dwords++;
		}

		if (elftohost64(phdr.p_filesz, le) <
		    elftohost64(phdr.p_memsz, le)) {
			size =
			    elftohost64(phdr.p_memsz,
					le) - elftohost64(phdr.p_filesz, le);
			size -= (remainder ? (8 - remainder) : 0);
			for (j = 0; j < (size / 8); j++) {
				memset(buf, 0x00, 8);
				count = fwrite(buf, 8, 1, ofp);
				if (count != 1)
					goto write_error;
			}
			dwords += (size >> 3);
			remainder = size & 0x07;
			printf("size = %d, dwords = %d, remainder = %d\n", size,
			       dwords, remainder);
			if (remainder) {
				memset(&buf[remainder], 0x00, 8 - remainder);
				count = fwrite(buf, 8, 1, ofp);
				if (count != 1)
					goto write_error;
				dwords++;
			}
		}

		if (!le) {
			/* big endian */
			table.entries[i].dwords.hi = 0;
			table.entries[i].dwords.lo =
			    hosttotarget32(dwords & 0xffffffff, le);
		} else {
			/* little endian */
			table.entries[i].dwords.hi =
			    hosttotarget32(dwords & 0xffffffff, le);
			table.entries[i].dwords.lo = 0;
		}
		printf("dwords = %d\n", dwords);
	}

	if (!boot2_load_hack) {
		if (!le) {
			/* big endian */
			table.count.hi = 0;
			table.count.lo = hosttotarget32(i & 0xffffffff, le);
		} else {
			/* little endian */
			table.count.hi = hosttotarget32(i & 0xffffffff, le);
			table.count.lo = 0;
		}
	} else {
		/* for now, turn off loading boot2 from boot1 */
		table.count.lo = table.count.hi = 0;
	}

	printf("table.count.lo = %x, table.count.hi = %x\n", table.count.lo,
	       table.count.hi);

	fseek(ofp, 0, SEEK_SET);
	count = fwrite(&table, sizeof(struct reloc_tbl), 1, ofp);
	if (count != 1)
		goto write_error;

	return;

      read_error:
	fprintf(stderr, "file read error: %s\n", strerror(errno));
	abort();
      write_error:
	fprintf(stderr, "file write error: %s\n", strerror(errno));
	abort();
#endif
}

static void usage(const char *progname)
{
	printf("Usage: %s -i <input> -o <output> -b <base> -s <size>\n",
	       progname);
	exit(-1);
}

static void init_flash_file(FILE * ofp, int size)
{
	char buf[1024];
	int i = 0;

	memset(buf, 0xff, 1024);

	for (i = 0; i < (size / 1024); i++)
		if (fwrite(buf, 1024, 1, ofp) != 1)
			goto write_error;

	for (i = 0; i < (size % 1024); i++)
		if (fwrite(buf, 1024, 1, ofp) != 1)
			goto write_error;

	return;

      write_error:
	fprintf(stderr, "file write error: %s\n", strerror(errno));
	abort();
}

int main(int argc, char *argv[])
{
	const char *output = "flash.bin";
	FILE *ofp, *ifp;
	const char *input = "boot1";
	unsigned char e_ident[EI_NIDENT];
	int littleendian = 0;
	int ch = 0;
	unsigned long base = 0x1fc00000;
	int size = (4 << 20);

	while ((ch = getopt(argc, argv, "hi:o:b:s:")) != -1) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			break;
		case 'i':
			input = strdup(optarg);
			break;
		case 'o':
			output = strdup(optarg);
			break;
		case 'b':{
				base = strtoul(optarg, 0, 16);
				if (base == ULONG_MAX || base > (512 << 20)) {
					fprintf(stderr,
						"bad base value %s, use a hex value or a value < 512MB\n",
						optarg);
					return -1;
				}
			}
			break;
		case 's':{
				size = strtoul(optarg, 0, 16);
				if (size == ULONG_MAX || size > (512 << 20)) {
					fprintf(stderr,
						"cannot generate flash image > 512MB\n");
					return -1;
				}
			}
			break;
		default:
			fprintf(stderr, "unrecognized option %c\n", ch);
			usage(argv[0]);
		}
	}

	printf
	    ("Generating \"%s\" from \"%s\" with flash physical addr = 0x%lx and image of size 0x%x\n",
	     output, input, base, size);

	ifp = fopen(input, "r");
	if (!ifp) {
		fprintf(stderr, "Unable to open output file \"%s\": %s\n",
			input, strerror(errno));
		return -1;
	}

	ofp = fopen(output, "w");
	if (!ofp) {
		fprintf(stderr, "Unable to open output file \"%s\": %s\n",
			output, strerror(errno));
		return -1;
	}

	/* truncate the file to the size requested */
	if (ftruncate(fileno(ofp), size) < 0) {
		fprintf(stderr, "Unable to truncate the output file\n");
		return -1;
	}
	init_flash_file(ofp, size);

	if (fread(e_ident, sizeof(e_ident), 1, ifp) != 1) {
		fprintf(stderr, "error reading \"%s\"\n", input);
		return -1;
	}

	if (e_ident[EI_DATA] == ELFDATA2LSB)
		littleendian = 1;
	else if (e_ident[EI_DATA] == ELFDATA2MSB)
		littleendian = 0;
	else {
		fprintf(stderr, "Unrecognized ELF Data %d\n", e_ident[EI_DATA]);
		return -1;
	}

	if (e_ident[EI_MAG0] != ELFMAG0 || e_ident[EI_MAG1] != ELFMAG1 ||
	    e_ident[EI_MAG2] != ELFMAG2 || e_ident[EI_MAG3] != ELFMAG3) {
		printf("Bad ELF header!\n");
		return -1;
	}

	if (e_ident[EI_CLASS] == ELFCLASS32)
		download_elf32(ifp, ofp, littleendian, base, size);
	else if (e_ident[EI_CLASS] == ELFCLASS64)
		download_elf64(ifp, ofp, littleendian, base, size);
	else {
		fprintf(stderr, "Unrecognized ELF Class %d\n",
			e_ident[EI_CLASS]);
		return -1;
	}

	fclose(ifp);
	fclose(ofp);

	return 0;
}
