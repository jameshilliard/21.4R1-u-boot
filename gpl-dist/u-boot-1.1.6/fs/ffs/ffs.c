/*
 * $Id$
 *
 * ffs.c -- Support for UFS File System 
 *
 * Venkanna Thadishetty, Jan 2014
 *
 * Copyright (c) 2014, Juniper Networks, Inc.
 * All rights reserved.
 *
 * CREDITS to original authors as published at:
 * http://webcache.googleusercontent.com/search?q=cache:_tOA-3WuBZUJ:
 * www.springdaemons.com/stas/u-boot-ffs.patch+&cd=1&hl=en&ct=clnk&gl=in
 * also as submitted to U-Boot for consideration
 * http://lists.denx.de/pipermail/u-boot/2008-November/042943.html
 *
 * FreeBSD functionality is taken from
 * http://svnweb.freebsd.org/base/head/sys/boot/common/ufsread.c?view=log
 *
 * Copyright (c) 2008 Stanislav Sedov <stas@FreeBSD.org>.
 * All rights reserved.
 * Copyright (c) 2002 McAfee, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and McAfee Research,, the Security Research Division of
 * McAfee, Inc. under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as
 * part of the DARPA CHATS research program
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 */

#include <common.h>

#include "ffs_dinode.h"
#include "ffs_fs.h"
#include "ffs.h"
#include <malloc.h>
#include "../../disk/part_dos.h"

/*
 * We use 4k `virtual' blocks for filesystem data, whatever the actual
 * filesystem block size. FFS blocks are always a multiple of 4k.
 */
#define	DEV_BSHIFT	9	/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1<<DEV_BSHIFT)
#define	VBLKSHIFT	12
#define	VBLKSIZE	(1 << VBLKSHIFT)
#define	VBLKMASK	(VBLKSIZE - 1)
#define	DBPERVBLK	(VBLKSIZE / DEV_BSIZE)
#define	INDIRPERVBLK(fs) (NINDIR(fs) / ((fs)->fs_bsize >> VBLKSHIFT))
#define	IPERVBLK(fs)	(INOPB(fs) / ((fs)->fs_bsize >> VBLKSHIFT))
#define	INO_TO_VBA(fs, ipervblk, x) \
    (fsbtodb(fs, cgimin(fs, ino_to_cg(fs, x))) + \
    (((x) % (fs)->fs_ipg) / (ipervblk) * DBPERVBLK))
#define	INO_TO_VBO(ipervblk, x) ((x) % ipervblk)
#define	FS_TO_VBA(fs, fsb, off) (fsbtodb(fs, fsb) + \
    ((off) / VBLKSIZE) * DBPERVBLK)
#define	FS_TO_VBO(fs, fsb, off) ((off) & VBLKMASK)
#define	DIP(node, field)	((node)->ver == 1 ? (node)->inode1.field : \
				    (node)->inode2.field)

/*
 * File types
 */
#define	DT_UNKNOWN	0
#define	DT_FIFO		1
#define	DT_CHR		2
#define	DT_DIR		4
#define	DT_BLK		6
#define	DT_REG		8
#define	DT_LNK		10
#define	DT_SOCK		12
#define	DT_WHT		14

#define	MAXBSIZE	65536
#define MAXPARTITIONS	8

struct dirent {
	uint32_t	d_fileno;		/* file number of entry */
	uint16_t	d_reclen;		/* length of this record */
	uint8_t		d_type;			/* file type, see below */
	uint8_t		d_namlen;		/* length of string in d_name */
#define	MAXNAMLEN	255
	char		d_name[MAXNAMLEN + 1];	/* name must be no longer */
};
typedef struct {
	struct fs		fs;
	block_dev_desc_t	*dev;
	long			part;
	int			sub_part;
	uint8_t			ver;
} ffs_mount_t;
typedef struct {
	union {
		struct ufs1_dinode	inode1;
		struct ufs2_dinode	inode2;
	};
	ffs_mount_t	*mount;
	uint8_t		ver;
} ffs_node_t;
typedef struct {
	u_int32_t d_magic;              /* the magic number */
        u_int16_t d_type;               /* drive type */
        u_int16_t d_subtype;            /* controller/d_type specific */
        char      d_typename[16];       /* type name, e.g. "eagle" */

        char      d_packname[16];       /* pack identifier */

                        /* disk geometry: */
        u_int32_t d_secsize;            /* # of bytes per sector */
        u_int32_t d_nsectors;           /* # of data sectors per track */
        u_int32_t d_ntracks;            /* # of tracks per cylinder */
        u_int32_t d_ncylinders;         /* # of data cylinders per unit */
        u_int32_t d_secpercyl;          /* # of data sectors per cylinder */
        u_int32_t d_secperunit;         /* # of data sectors per unit */

        /*
         * Spares (bad sector replacements) below are not counted in
         * d_nsectors or d_secpercyl.  Spare sectors are assumed to
         * be physical sectors which occupy space at the end of each
         * track and/or cylinder.
         */
        u_int16_t d_sparespertrack;     /* # of spare sectors per track */
        u_int16_t d_sparespercyl;       /* # of spare sectors per cylinder */
        /*
         * Alternate cylinders include maintenance, replacement, configuration
         * description areas, etc.
         */
        u_int32_t d_acylinders;         /* # of alt. cylinders per unit */

                        /* hardware characteristics: */
        /*
         * d_interleave, d_trackskew and d_cylskew describe perturbations
         * in the media format used to compensate for a slow controller.
         * Interleave is physical sector interleave, set up by the
         * formatter or controller when formatting.  When interleaving is
         * in use, logically adjacent sectors are not physically
         * contiguous, but instead are separated by some number of
         * sectors.  It is specified as the ratio of physical sectors
         * traversed per logical sector.  Thus an interleave of 1:1
         * implies contiguous layout, while 2:1 implies that logical
         * sector 0 is separated by one sector from logical sector 1.
         * d_trackskew is the offset of sector 0 on track N relative to
         * sector 0 on track N-1 on the same cylinder.  Finally, d_cylskew
         * is the offset of sector 0 on cylinder N relative to sector 0
         * on cylinder N-1.
         */
        u_int16_t d_rpm;                /* rotational speed */
        u_int16_t d_interleave;         /* hardware sector interleave */
        u_int16_t d_trackskew;          /* sector 0 skew, per track */
        u_int16_t d_cylskew;            /* sector 0 skew, per cylinder */
        u_int32_t d_headswitch;         /* head switch time, usec */
        u_int32_t d_trkseek;            /* track-to-track seek, usec */
        u_int32_t d_flags;              /* generic flags */
#define NDDATA 5
        u_int32_t d_drivedata[NDDATA];  /* drive-type specific information */
#define NSPARE 5
        u_int32_t d_spare[NSPARE];      /* reserved for future use */
        u_int32_t d_magic2;             /* the magic number (again) */
        u_int16_t d_checksum;           /* xor of data incl. partitions */

                        /* filesystem and partition information: */
        u_int16_t d_npartitions;        /* number of partitions in following */
        u_int32_t d_bbsize;             /* size of boot area at sn0, bytes */
        u_int32_t d_sbsize;             /* max size of fs superblock, bytes */
        struct partition {              /* the partition table */
                u_int32_t p_size;       /* number of sectors in partition */
                u_int32_t p_offset;     /* starting sector */
                u_int32_t p_fsize;      /* filesystem basic fragment size */
                u_int8_t p_fstype;      /* filesystem type, see below */
                u_int8_t p_frag;        /* filesystem fragments per block */
                u_int16_t p_cpg;        /* filesystem cylinders per group */
        } d_partitions[MAXPARTITIONS];  /* actually may be more */
} disk_label_t;


/*
 * Convert between stat structure types and directory types.
 */
#define IFTODT(mode)    (((mode) & 0170000) >> 12)
#define DTTOIF(dirtype) ((dirtype) << 12)

static ino_t	lookup(ffs_mount_t *mount, const char *path);
static ssize_t	fsread(ffs_mount_t *mount, ffs_node_t *node, void *buf,
    size_t len, uint32_t off);
static int	ffs_open(ffs_mount_t *mount, ino_t ino, ffs_node_t *node);

/* Convert char[4] in little endian format to the host format integer
 */
static inline int le32_to_int(unsigned char *le32)
{
    return ((le32[3] << 24) +
            (le32[2] << 16) +
            (le32[1] << 8) +
             le32[0]
           );
}

int print=0;

static int
dsklabelread(block_dev_desc_t *dev_desc, ulong slice_lba_start, int sub_part,
	 ulong *part_offset, ulong *part_size)
{
    int i, size=0, offset=0;
    unsigned char dl_buffer[DEFAULT_SECTOR_SIZE];
    disk_label_t *lp;

    slice_lba_start = slice_lba_start + 1;

    if (dev_desc->block_read(dev_desc->dev, slice_lba_start,
			     1, (ulong *) dl_buffer) != 1) {
	printf ("** Can't read partition table on %d:%d **\n",
		dev_desc->dev, slice_lba_start);
    }

    lp = (disk_label_t *) dl_buffer;

    for (i = 0; i < 8; i++)
    {
	/*
	 * Get size & offset for this partition from
	 * disklabel structure
	 */
	size = le32_to_int((uchar *) &lp->d_partitions[i].p_size);
	offset = le32_to_int((uchar *) &lp->d_partitions[i].p_offset);

	*part_size = size;
	*part_offset = offset;

	if (i == sub_part)
	    break;
    }

    return 0;
}


static int
dskread(block_dev_desc_t *desc, long part, int sub_part, void *buf,
	unsigned lba,
	unsigned int nblk)
{
	disk_partition_t pinfo;
	int error;

	if (part == 0) {
		pinfo.start = 0;
		pinfo.size = desc->lba;
		pinfo.blksz = desc->blksz;
	} else {
		error = get_partition_info(desc, part, &pinfo);
		if (error != 0)
			return (error);

		dsklabelread(desc, pinfo.start, sub_part, &pinfo.start,
			     &pinfo.size);

	}
	if (lba >= pinfo.size) {
		printf("Error: reading outside the partition size\n");
		return (1);
	}
	error = desc->block_read(desc->dev, pinfo.start + lba, nblk, buf);
	if (error != nblk) {
		printf("Error: not all blocks read: wanted %d got %d\n", nblk,
		    error);
		return (1);
	}
	return (0);
}

static int
fsfind(ffs_mount_t *mount, const char *name, ino_t * ino)
{
	char buf[DEV_BSIZE];
	struct dirent *d;
	char *s;
	ssize_t n;
	uint32_t off;
	ffs_node_t *node = NULL;
	int error;

	node = (ffs_node_t *)malloc(sizeof(*node));
	if (node == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*node));
		return (0);
	}
	error = ffs_open(mount, *ino, node);
	if (error != 0) {
		printf("Error: could not read inode.\n");
		free(node);
		return (0);
	}
	off = 0;
	while ((n = fsread(mount, node, buf, DEV_BSIZE, off)) > 0) {
		for (s = buf; s < buf + DEV_BSIZE;) {
			d = (void *)s;
			if (!strcmp(name, d->d_name)) {
				*ino = d->d_fileno;
				free(node);
				return (d->d_type);
			}
			s += d->d_reclen;
		}
		off += n;
	}
	free(node);
	return (0);
}

static ino_t
lookup(ffs_mount_t *mount, const char *path)
{
	char name[MAXNAMLEN + 1];
	const char *s;
	ino_t ino;
	ssize_t n;
	int dt;

	ino = ROOTINO;
	dt = DT_DIR;
	name[0] = '/';
	name[1] = '\0';
	for (;;) {
		if (*path == '/')
			path++;
		if (!*path)
			break;
		for (s = path; *s && *s != '/'; s++);
		if ((n = s - path) > MAXNAMLEN)
			return (0);
		memcpy(name, path, n);
		name[n] = 0;
		if (dt != DT_DIR) {
			printf("%s: not a directory.\n", name);
			return (0);
		}
		if ((dt = fsfind(mount, name, &ino)) <= 0)
			break;
		path = s;
	}
	return (ino);
}

/*
 * Possible superblock locations ordered from most to least likely.
 */
static int sblocksearch[] = SBLOCKSEARCH;

static int
ffs_mount(block_dev_desc_t *desc, long part, int sub_part, ffs_mount_t *mount)
{
	int i;
	struct fs *buf;
	int error;

	assert(mount != NULL);
	buf = (struct fs *)malloc(SBLOCKSIZE);
	if (buf == NULL) {
		printf("Error: allocating buffer of size %d failed.\n",
		    SBLOCKSIZE);
		return (-1);
	}

	/*
	 * Try to read the superblock.
	 */
	for (i = 0; sblocksearch[i] != -1; i++) {
		error = dskread(desc, part, sub_part, buf, sblocksearch[i] / DEV_BSIZE,
		    SBLOCKSIZE / DEV_BSIZE);
		if (error != 0) {
			printf("Error reading superblock.\n");
			free(buf);
			return (-1);
		}
		/* Check for correct block size. */
		if (buf->fs_bsize > MAXBSIZE || 
		    buf->fs_bsize < sizeof(struct fs))
			continue;
		/* Check for magic. */
		if (buf->fs_magic == FS_UFS1_MAGIC) {
			mount->ver = 1;
			break;
		}
		if (buf->fs_magic == FS_UFS2_MAGIC &&
		    buf->fs_sblockloc == sblocksearch[i]) {
			mount->ver = 2;
			break;
		}
	}
	if (sblocksearch[i] == -1) {
		debug("UFS superblock was not found.\n");
		free(buf);
		return(-1);
	}
	mount->dev = desc;
	mount->part = part;
	mount->sub_part = sub_part;
	memcpy(&mount->fs, buf, sizeof(struct fs));
	free(buf);
	return (0);
}

static int
ffs_open(ffs_mount_t *mount, ino_t ino, ffs_node_t *node)
{
	size_t ipervblk, i_off, len;
	struct fs *fs;
	char *blkbuf;
	int error;

	assert(node != NULL);
	assert(ino != 0);
	fs = &mount->fs;

	blkbuf = (char *)malloc(VBLKSIZE);
	if (blkbuf == NULL) {
		printf("Error: allocating buffer of size %d failed.\n",
		    VBLKSIZE);
		return (-1);
	}

	/*
	 * Read the inode requested.
	 */
	ipervblk = IPERVBLK(fs);
	error = dskread(mount->dev, mount->part, mount->sub_part, blkbuf,
	    INO_TO_VBA(fs, ipervblk, ino), DBPERVBLK);
	if (error != 0) {
		printf("Error reading inode.\n");
		free(blkbuf);
		return (-1);
	}
	node->mount = mount;
	node->ver = mount->ver;
	i_off = INO_TO_VBO(ipervblk, ino);
	if (mount->ver == 1) {
		len = sizeof(struct ufs1_dinode);
		memcpy(&node->inode1, blkbuf + i_off * len, len);
	} else {
		len = sizeof(struct ufs2_dinode);
		memcpy(&node->inode2, blkbuf + i_off * len, len);
	}
	free(blkbuf);
	return (0);
}

static ssize_t
fsread(ffs_mount_t *mount, ffs_node_t *node, void *buf, size_t len,
    uint32_t off)
{
	char *blkbuf = NULL;
	void *indbuf = NULL;
	size_t left, size, f_off, vboff;
	ufs_lbn_t f_lbn;
	ufs2_daddr_t addr, vbaddr;
	int i, n;
	int error;
	int indirpervblk;
	struct fs *fs;

	assert(mount != NULL);
	assert(node != NULL);

	/*
	 * Allocate work memory.
	 */
	blkbuf = (char *)malloc(VBLKSIZE);
	indbuf = malloc(VBLKSIZE);
	if (blkbuf == NULL || indbuf == NULL) {
		printf("Error: allocating buffers of size %d failed.\n",
		    VBLKSIZE);
		error = -1;
		goto fail;
	}

	fs = &mount->fs;
	size = DIP(node, di_size);
	n = size - off;
	if (len > n)
		len = n;
	left = len;
	while (left > 0) {
		f_lbn = lblkno(fs, off);	/* Block number of the file. */
		f_off = blkoff(fs, off);	/* Offset in the file block. */

		/*
		 * Get the address of the block requested.
		 */
		if (f_lbn < NDADDR) {
			addr = DIP(node, di_db[f_lbn]);
		} else if (f_lbn < NDADDR + NINDIR(fs)) {
			/*
			 * Try to read the block map table.
			 */
			indirpervblk = INDIRPERVBLK(fs);
			addr = DIP(node, di_ib[0]);
			vbaddr = fsbtodb(fs, addr) +	/* Block map address. */
			    (f_lbn - NDADDR) / (indirpervblk * DBPERVBLK);
			error = dskread(mount->dev, mount->part,
					mount->sub_part,
					indbuf,
					vbaddr,
					DBPERVBLK);
			if (error != 0) {
				printf("Error reading indirect block map.\n");
				error = -1;
				goto fail;
			}
			/* Offset in the table. */
			i = (f_lbn - NDADDR) & (indirpervblk - 1);
			if (fs->fs_magic == FS_UFS1_MAGIC)
				addr = ((ufs1_daddr_t *)indbuf)[i];
			else
				addr = ((ufs2_daddr_t *)indbuf)[i];
		} else {
			printf("Error: double-indirect blocks are not"
			    " supported (yet).");
			goto fail;
		}

		/*
		 * Read the block itself.
		 */
		vbaddr = fsbtodb(fs, addr) + (f_off >> VBLKSHIFT) * DBPERVBLK;
		vboff = f_off & VBLKMASK;
		i = sblksize(fs, size, f_lbn) - (f_off & ~VBLKMASK);
		if (i > VBLKSIZE)
			i = VBLKSIZE;
		error = dskread(mount->dev, mount->part, mount->sub_part,
				blkbuf,
				vbaddr,
				i >> DEV_BSHIFT);
		if (error != 0) {
			printf("Error reading data block.\n");
			error = -1;
			goto fail;
		}
		i -= vboff;
		if (i > left)
			i = left;
		memcpy(buf, blkbuf + vboff, i);
		buf += i;
		off += i;
		left -= i;
	}
	error = len;

fail:
	if (blkbuf != NULL)
		free(blkbuf);
	if (indbuf != NULL)
		free(indbuf);
	return (error);
}

int
ffs_probe(block_dev_desc_t *desc, long part, int sub_part)
{
	ffs_mount_t *mount = NULL;
	int error;

	mount = (ffs_mount_t *)malloc(sizeof(*mount));
	if (mount == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*mount));
		return (1);
	}

	/*
	 * Try to mount the fs.
	 */
	error = ffs_mount(desc, part, sub_part, mount);
	free(mount);
	return (error);
}

int
ffs_getfilelength(block_dev_desc_t *desc, long part, int sub_part,
		  const char *filename,
		  unsigned long *len)
{
	ffs_mount_t *mount = NULL;
	ffs_node_t *node = NULL;
	ino_t ino;
	int error;

	mount = (ffs_mount_t *)malloc(sizeof(*mount));
	if (mount == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*mount));
		return (1);
	}

	/*
	 * Try to mount the fs.
	 */
	error = ffs_mount(desc, part, sub_part, mount);
	if (error != 0) {
		printf("Error: could not mount the filesystem.\n");
		goto fail;
	}

	ino = lookup(mount, filename);
	if (ino == 0) {
		error = 1;
		goto fail;
	}

	/*
	 * Open the file.
	 */
	node = (ffs_node_t *)malloc(sizeof(*node));
	if (node == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*node));
		error = 1;
		goto fail;
	}
	error = ffs_open(mount, ino, node);
	if (error != 0)
		printf("Error: could not read inode.\n");
	else
		*len = DIP(node, di_size);
fail:
	if (mount != NULL)
		free(mount);
	return (error);
}

int
ffs_ls(block_dev_desc_t *desc, long part, int sub_part, const char *filename)
{
	char buf[DEV_BSIZE];
	struct dirent *d;
	char *s;
	ssize_t n;
	ino_t ino;
	uint32_t off;
	ffs_mount_t *mount = NULL;
	ffs_node_t *node = NULL, *fnode = NULL;
	int error;

	if (*filename == '\0') {
		printf("Error: empty filename\n");
		return (1);
	}

	mount = (ffs_mount_t *)malloc(sizeof(*mount));
	if (mount == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*mount));
		return (1);
	}

	/*
	 * Try to mount the fs.
	 */
	error = ffs_mount(desc, part, sub_part, mount);
	if (error != 0) {
		printf("Error: could not mount the filesystem.\n");
		error = 1;
		goto fail;
	}

	/*
	 * Try to locate the file.
	 */
	ino = lookup(mount, filename);
	if (ino == 0) {
		printf("Error: not found\n");
		error = 1;
		goto fail;
	}

	/*
	 * Open the directory file and read it.
	 */
	node = (ffs_node_t *)malloc(sizeof(*node));
	fnode = (ffs_node_t *)malloc(sizeof(*fnode));
	if (node == NULL || fnode == NULL) {
		printf("Error: allocating %ld bytes failed.\n",
		    2 * sizeof(*node));
		error = 1;
		goto fail;
	}
	error = ffs_open(mount, ino, node);
	if (error != 0) {
		printf("Error: could not read inode.\n");
		error = 1;
		goto fail;
	}

	off = 0;
	printf("%-8s%-15s%s\n", "INODE", "SIZE", "NAME");
	while ((n = fsread(mount, node, buf, DEV_BSIZE, off)) > 0) {
		for (s = buf; s < buf + DEV_BSIZE;) {
			d = (void *)s;
			s += d->d_reclen;
			error = ffs_open(mount, d->d_fileno, fnode);
			if (error != 0) {
				printf("Error: could not open file %s\n",
				    d->d_name);
				continue;
			}
			printf("%-8d%-15d%s", d->d_fileno,
			    (int)DIP(fnode, di_size), d->d_name);
			if (d->d_type == DT_DIR)
				printf("/");
			printf("\n");
		}
		off += n;
	}
	printf("\n");
	error = 0;
fail:
	if (node != NULL)
		free(node);
	if (fnode != NULL)
		free(fnode);
	if (mount != NULL)
		free(mount);
	return (error);
}

int
ffs_read(block_dev_desc_t *desc, long part, int sub_part, const char *filename,
	 char *addr,
	 unsigned long len)
{
	ino_t ino;
	int error;
	ffs_mount_t *mount = NULL;
	ffs_node_t *node = NULL;

	if (*filename == '\0') {
		printf("Error: empty filename\n");
		return (1);
	}

	mount = (ffs_mount_t *)malloc(sizeof(*mount));
	if (mount == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*mount));
		return (1);
	}

	/*
	 * Try to mount the fs.
	 */
	error = ffs_mount(desc, part, sub_part, mount);
	if (error != 0) {
		printf("Error: could not mount the filesystem.\n");
		error = 1;
		goto fail;
	}

	/*
	 * Try to locate the file.
	 */
	ino = lookup(mount, filename);
	if (ino == 0) {
		printf("Error: not found\n");
		error = 1;
		goto fail;
	}

	/*
	 * Open the file and read it.
	 */
	node = (ffs_node_t *)malloc(sizeof(*node));
	if (node == NULL) {
		printf("Error: allocating %ld bytes failed.\n", sizeof(*node));
		error = 1;
		goto fail;
	}
	error = ffs_open(mount, ino, node);
	if (error != 0) {
		printf("Error: could not read inode.\n");
		error = 1;
		goto fail;
	}
	error = fsread(mount, node, addr, len, 0);
	if (error != len) {
		printf("Error: not all data read: wanted %ld got %d\n", len,
		    error);
		error = 1;
	} else
		error = 0;
fail:
	if (node != NULL)
		free(node);
	if (mount != NULL)
		free(mount);
	return (error);
}
