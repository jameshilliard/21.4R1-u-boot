/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 *
 * Updated-by: Prafulla Wadaskar <prafulla@marvell.com>
 *		default_image specific code abstracted from mkimage.c
 *		some functions added to address abstraction
 *
 * All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "mkimage.h"
#include <image.h>
#include <u-boot/crc.h>

static image_header_t header;

static int image_check_image_types(uint8_t type)
{
	if (((type > IH_TYPE_INVALID) && (type < IH_TYPE_FLATDT)) ||
	    (type == IH_TYPE_KERNEL_NOLOAD))
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

static int image_check_params(struct mkimage_params *params)
{
	return	((params->dflag && (params->fflag || params->lflag)) ||
		(params->fflag && (params->dflag || params->lflag)) ||
		(params->lflag && (params->dflag || params->fflag)));
}

static int image_verify_header(unsigned char *ptr, int image_size,
			struct mkimage_params *params)
{
	uint32_t len;
	const unsigned char *data;
	uint32_t checksum;
	image_header_t header;
	image_header_t *hdr = &header;

	/*
	 * create copy of header so that we can blank out the
	 * checksum field for checking - this can't be done
	 * on the PROT_READ mapped data.
	 */
	memcpy(hdr, ptr, sizeof(image_header_t));

	if (be32_to_cpu(hdr->ih_magic) != IH_MAGIC) {
		fprintf(stderr,
			"%s: Bad Magic Number: \"%s\" is no valid image\n",
			params->cmdname, params->imagefile);
		return -FDT_ERR_BADMAGIC;
	}

	data = (const unsigned char *)hdr;
	len  = sizeof(image_header_t);

	checksum = be32_to_cpu(hdr->ih_hcrc);
	hdr->ih_hcrc = cpu_to_be32(0);	/* clear for re-calculation */

	if (crc32(0, data, len) != checksum) {
		fprintf(stderr,
			"%s: ERROR: \"%s\" has bad header checksum!\n",
			params->cmdname, params->imagefile);
		return -FDT_ERR_BADSTATE;
	}

	data = (const unsigned char *)ptr + sizeof(image_header_t);
	len  = image_size - sizeof(image_header_t) ;

	checksum = be32_to_cpu(hdr->ih_dcrc);
	if (crc32(0, data, len) != checksum) {
		fprintf(stderr,
			"%s: ERROR: \"%s\" has corrupted data!\n",
			params->cmdname, params->imagefile);
		return -FDT_ERR_BADSTRUCTURE;
	}
	return 0;
}

static void image_set_header(void *ptr, struct stat *sbuf, int ifd,
				struct mkimage_params *params)
{
	uint32_t checksum;
/* Junos start */
	image_header_t * hdr;

	if (!params->cflag) {
	    hdr = (image_header_t *)ptr;

	    checksum = crc32(0,
			(const unsigned char *)(ptr +
				sizeof(image_header_t)),
			sbuf->st_size - sizeof(image_header_t));
	} else {
	    /* embedded header in firmware offset 0x30 */
	    hdr = (image_header_t *)(ptr + IMG_HEADER_OFFSET);

	    /* crc offset 0x100 */
	    checksum = crc32(0,
			     (const unsigned char *)(ptr + IMG_DATA_OFFSET),
			     sbuf->st_size - IMG_DATA_OFFSET);
	}

	/* Build new header */
	image_set_magic(hdr, IH_MAGIC);
	image_set_time(hdr, sbuf->st_mtime);
	if (!params->cflag) {
	    image_set_size(hdr, sbuf->st_size - sizeof(image_header_t));
	} else {
	    /*
	     * Data starts at an offset. Not just after image header
	     * IMG_DATA_OFFSET is an offset where the execution starts.
	     */
	    image_set_size(hdr, sbuf->st_size - IMG_DATA_OFFSET);
	}
/* Junos end */
	image_set_load(hdr, params->addr);
	image_set_ep(hdr, params->ep);
	image_set_dcrc(hdr, checksum);
	image_set_os(hdr, params->os);
	image_set_arch(hdr, params->arch);
	image_set_type(hdr, params->type);
	image_set_comp(hdr, params->comp);

	image_set_name(hdr, params->imagename);

	checksum = crc32(0, (const unsigned char *)hdr,
				sizeof(image_header_t));

	image_set_hcrc(hdr, checksum);
}

/*
 * Default image type parameters definition
 */
static struct image_type_params defimage_params = {
	.name = "Default Image support",
	.header_size = sizeof(image_header_t),
	.hdr = (void*)&header,
	.check_image_type = image_check_image_types,
	.verify_header = image_verify_header,
	.print_header = image_print_contents,
	.set_header = image_set_header,
	.check_params = image_check_params,
};

void init_default_image_type(void)
{
	mkimage_register(&defimage_params);
}
