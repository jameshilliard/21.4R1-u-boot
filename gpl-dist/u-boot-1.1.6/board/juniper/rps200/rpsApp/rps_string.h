/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __RPS_STRING_H
#define __RPS_STRING_H

#undef __HAVE_ARCH_STRCPY
#undef __HAVE_ARCH_STRNCPY
#undef __HAVE_ARCH_STRCAT
#undef __HAVE_ARCH_STRNCAT
#undef __HAVE_ARCH_STRCMP
#undef __HAVE_ARCH_STRNCMP
#undef __HAVE_ARCH_STRCHR
#undef __HAVE_ARCH_STRRCHR
#undef __HAVE_ARCH_STRLEN
#undef __HAVE_ARCH_STRNLEN
#undef __HAVE_ARCH_STRDUP
#undef __HAVE_ARCH_STRSPN
#undef __HAVE_ARCH_STRPBRK
#undef __HAVE_ARCH_STRTOK
#undef __HAVE_ARCH_STRSEP
#undef __HAVE_ARCH_STRSWAB
#undef __HAVE_ARCH_MEMSET
#undef __HAVE_ARCH_BCOPY
#undef __HAVE_ARCH_MEMCPY
#undef __HAVE_ARCH_MEMMOVE
#undef __HAVE_ARCH_MEMCMP
#undef __HAVE_ARCH_MEMSCAN
#undef __HAVE_ARCH_STRSTR
#undef __HAVE_ARCH_MEMCHR


extern char * strcpy(char * dest,const char *src);
extern char * strncpy(char * dest,const char *src, size_t count);
extern char * strcat(char * dest, const char * src);
extern char * strncat(char *dest, const char *src, size_t count);
extern int strcmp(const char * cs,const char * ct);
extern int strncmp(const char * cs,const char * ct,size_t count);
extern char * strchr(const char * s, int c);
extern char * strrchr(const char * s, int c);
extern size_t strlen(const char * s);
extern size_t strnlen(const char * s, size_t count);
extern char * strdup(const char *s);
extern size_t strspn(const char *s, const char *accept);
extern char * strpbrk(const char * cs,const char * ct);
extern char * strtok(char * s,const char * ct);
extern char * strsep(char **s, const char *ct);
extern char *strswab(const char *s);
extern void * memset(void * s,int c,size_t count);
extern char * bcopy(const char * src, char * dest, int count);
extern void * memcpy(void * dest,const void *src,size_t count);
extern void * memmove(void * dest,const void *src,size_t count);
extern int memcmp(const void * cs,const void * ct,size_t count);
extern void * memscan(void * addr, int c, size_t size);
extern char * strstr(const char * s1,const char * s2);
extern void *memchr(const void *s, int c, size_t n);
extern long simple_strtol(const char *cp,char **endp,unsigned int base);

#endif
