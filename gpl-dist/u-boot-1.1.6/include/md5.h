/* MD5.H - header file for MD5C.C
 * $FreeBSD: src/sys/sys/md5.h,v 1.18.2.1 2006/04/04 19:59:52 andre Exp $
 */

/*-
 Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/*
 * !!! IMPORTANT !!!
 * This file is used by both the kernel and also for the Known Answer
 * Tests (KATS) in src/juniper/lib/fips/kats/kernel. If you modify this
 * file then please ensure that you do a build in
 *
 * src/secure/juniper/sbin/kats/kernel
 *
 * to ensure that nothing is broken
 */

#ifndef _MD5_H_
#define _MD5_H_

#define MD5_BLOCK_LENGTH		64
#define MD5_DIGEST_LENGTH		16
#define MD5_DIGEST_STRING_LENGTH	(MD5_DIGEST_LENGTH * 2 + 1)

/* MD5 context. */
typedef struct MD5Context {
  u_int32_t state[4];	/* state (ABCD) */
  u_int32_t count[2];	/* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];	/* input buffer */
} MD5_CTX;

void   MD5Init (MD5_CTX *);
void   MD5Update (MD5_CTX *, const void *, unsigned int);
void   MD5Final (unsigned char [16], MD5_CTX *);
char * MD5End(MD5_CTX *, char *);
char * MD5File(const char *, char *);
char * MD5FileChunk(const char *, char *, off_t, off_t);
char * MD5Data(const void *, unsigned int, char *);
#endif /* _MD5_H_ */
