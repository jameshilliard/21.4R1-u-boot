
/*
 * $Id$
 *
 * acx_post.h -- Common macros for debugging during POST
 *
 * Copyright (c) 2011, Juniper Networks, Inc.
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#define POST_LOG_DEBUG 1
#define POST_LOG_INFO  2
#define POST_LOG_IMP   3
#define POST_LOG_ERR   4

#define POST_LOG(level, args...)                                              \
    do {                                                                      \
	if  ((level >= POST_LOG_ERR) ||                                       \
	    ((flags & (POST_MANUAL | POST_SLOWTEST | POST_POWERON))           \
	     && (level > POST_LOG_DEBUG)))                                    \
	    post_log(args);                                                   \
    } while(0)

