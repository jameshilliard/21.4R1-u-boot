/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 *  ether.h
 *
 *  Driver for the Motorola Triple Speed Ethernet Controller
 *
 *  This software may be used and distributed according to the
 *  terms of the GNU Public License, Version 2, incorporated
 *  herein by reference.
 *
 * Copyright 2004 Freescale Semiconductor.
 * (C) Copyright 2003, Motorola, Inc.
 * maintained by Xianghua Xiao (x.xiao@motorola.com)
 * author Andy Fleming
 *
 */

#ifndef __ETHER_H
#define __ETHER_H

typedef struct _loopback_status_field {
    uint32_t loopType :4; /* loopback type */
    uint32_t pktSize :12;	/* packet size */
    uint32_t TxErr :8;	/* Tx Error */
    uint32_t RxErr :8;	/* Rx Error */
} loopback_status_field;

typedef union _loopback_status {
    loopback_status_field field;
    uint32_t status;
} loopback_status;


#endif /* __ETHER_H */
