/*
 * Copyright (c) 2008 Stanislav Sedov <stas@FreeBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * FFSv2 filesystem driver.
 */

#include <common.h>

#ifdef CONFIG_ACX

#include <config.h>
#include <command.h>
#include <image.h>
#include <asm/byteorder.h>
#include <ffs.h>
#include <part.h>

block_dev_desc_t *get_dev (char* ifname, int dev)
{
#if (CONFIG_COMMANDS & CFG_CMD_IDE)
        if (strncmp(ifname,"ide",3)==0) {
                extern block_dev_desc_t * ide_get_dev(int dev);
                return(ide_get_dev(dev));
        }
#endif
#if (CONFIG_COMMANDS & CFG_CMD_SCSI)
        if (strncmp(ifname,"scsi",4)==0) {
                extern block_dev_desc_t * scsi_get_dev(int dev);
                return(scsi_get_dev(dev));
        }
#endif
#if ((CONFIG_COMMANDS & CFG_CMD_USB) && defined(CONFIG_USB_STORAGE))
        if (strncmp(ifname,"usb",3)==0) {
                extern block_dev_desc_t * usb_stor_get_dev(int dev);
                return(usb_stor_get_dev(dev));
        }
#endif
#if defined(CONFIG_MMC)
        if (strncmp(ifname,"mmc",3)==0) {
                extern block_dev_desc_t *  mmc_get_dev(int dev);
                return(mmc_get_dev(dev));
        }
#endif
#if defined(CONFIG_SYSTEMACE)
        if (strcmp(ifname,"ace")==0) {
                extern block_dev_desc_t *  systemace_get_dev(int dev);
                return(systemace_get_dev(dev));
        }
#endif
#if defined(CONFIG_NOR)
        if (strcmp(ifname,"nor")==0) {
                extern block_dev_desc_t *  nor_get_dev(int dev);
                return(nor_get_dev(dev));
        }
#endif
        return NULL;
}

int
ffs_parse_device_spec(char *iface, const char *str,
    block_dev_desc_t **desc, long *dev, long *part, int *subpart)
{
	char *ep, *p;

	*dev = simple_strtoul(str, &ep, 16);
	if ((*ep != '\0' && *ep != ':') || *str == '\0') {
		printf("Error: invalid device number: %s\n", str);
		return (1);
	}

	*desc = get_dev(iface, *dev);
	if (*desc == NULL) {
		printf("Error: block device %ld on %s is not supported\n",
		    *dev, iface);
		return (1);
	}

	if (*ep) {
		p = ++ep;
		*part = simple_strtoul(p, &ep, 16);
		if ((*ep != '\0' && *ep != ':') || *str == '\0') {
			printf("Error: invalid partition number %s\n", p);
			return (1);
		}
	} else {
		*part = 0;	/* Whole disk. */
	}

	if (*ep) {
	    p = ++ep;
	    *subpart = simple_strtoul(p, &ep, 16);
	    if (p == '\0' || *ep != '\0') {
		printf("Error: invalid sub partition number %s\n", p);
		return (1);
	    }
	} else {
	    *subpart = 0;
	}

	return (0);
}

int
do_ffs_ls(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *filename;
	long dev, part;
	block_dev_desc_t *dev_desc;
	int error, subpart;

	if (argc < 3) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return (1);
	}
	
	error = ffs_parse_device_spec(argv[1], argv[2], &dev_desc, &dev, &part,
				      &subpart);
	if (error != 0)
		return (error);

	filename = argc == 4 ? argv[3] : "/";
	error = ffs_probe(dev_desc, part, subpart);
	if (error != 0) {
		printf("Error: could not mount %ld dev %ld part on %s: %d\n",
		    dev, part, argv[1], error);
		return (1);
	}

	error = ffs_ls(dev_desc, part, subpart, filename);
	if (error != 0) {
		printf("Error: ffs_ls: %d\n", error);
		return (1);
	}
	return (0);
}

int
do_ffs_load(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *filename;
	long dev, part;
	unsigned long addr, length;
	block_dev_desc_t *dev_desc;
	char buf[12];
	unsigned long count;
	char *str, *ep;
	int error, subpart;

	if (argc >= 4) {	/* loadaddr is given. */
		addr = simple_strtoul(argv[3], &ep, 16);
		if (*ep != '\0' || *argv[3] == '\0') {
			printf("Error: invalid load address: %s\n", argv[3]);
			return (1);
		}
	} else {
		str = getenv("loadaddr");
		if (str != NULL) {
			addr = simple_strtoul(str, &ep, 16);
			if (*ep != '\0' || *argv[3] == '\0') {
				printf("Error: invalid load address: %s\n",
				    argv[3]);
				return (1);
			}
		} else {
			addr = CFG_LOAD_ADDR;
		}
	}

	if (argc >= 5)		/* filename is given. */
		filename = argv[4];
	else
		filename = getenv("bootfile");
	if (filename == NULL || filename == '\0') {
		printf("Error: no filename to load\n");
		return (1);
	}

	if (argc == 6) {	/* count is given. */
		count = simple_strtoul(argv[5], &ep, 16);
		if (*ep != '\0' || *argv[5] == '\0') {
			printf("Error: invalid count specification: %s\n",
			    argv[5]);
			return (1);
		}
	} else {
		count = 0;
	}

	if (argc < 3 || argc > 6) {	/* Catch up on errors. */
		printf("Usage:\n%s\n", cmdtp->usage);
		return (1);
	}

	error = ffs_parse_device_spec(argv[1], argv[2], &dev_desc, &dev, &part,
				      &subpart);
	if (error != 0)
		return (error);

	error = ffs_probe(dev_desc, part, subpart);
	if (error != 0) {
		printf("Error: could not mount %ld dev %ld part on %s: %d\n",
		    dev, part, argv[1], error);
		return (1);
	}

	error = ffs_getfilelength(dev_desc, part, subpart, filename, &length);
	if (error != 0) {
		printf("Error: file not found: %s\n", filename);
		return (1);
	}
	if ((count < length) && (count != 0)) {
	    length = count;
	}

	error = ffs_read(dev_desc, part, subpart, filename, (char *)addr, length);
	if (error != 0) {
		printf("Error reading %ld bytes from %s", length, filename);
		return (1);
	}

	load_addr = addr;		/* Update load address. */
	sprintf(buf, "%lX", length);
	setenv("filesize", buf);	/* Update filesize. */

	return (length);
}

/*
 * U-boot drop-in's.
 */

U_BOOT_CMD(
	ffsls, 4, 1, do_ffs_ls,
	"ffsls   - list files in a directory (default /)\n",
	"<interface> <dev[:part][:subpart]> [directory]\n"
	"    - list directory of device 'dev' partition 'part' and 'subpart' at 'interface'\n"
);

U_BOOT_CMD(
	ffsload, 6, 0, do_ffs_load,
	"ffsload - load binary file from a FFS filesystem\n",
	"<interface> <dev[:part][:subpart]> [addr] [filename] [bytes]\n"
	"    - load binary file from device 'dev' slice 'part' partition 'subpart'\n"
	"      at 'interface' to address 'addr'\n"
);

#endif

