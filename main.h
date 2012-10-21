#include <stdio.h>
#include <math.h>

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#ifndef _PKTHEADERS_H
#include "pktHeaders.h"
#endif

/*
 * TCP sequence numbers are 32 bit integers operated
 * on with modular arithmetic.  These macros can be
 * used to compare such integers. 
 */
#define SEQ_LT(a,b)             ((int)((a)-(b)) < 0)
#define SEQ_LEQ(a,b)            ((int)((a)-(b)) <= 0)
#define SEQ_GT(a,b)             ((int)((a)-(b)) > 0)
#define SEQ_GEQ(a,b)            ((int)((a)-(b)) >= 0)

/* we do the same for the IP ids */
#define IPID_LT(a,b)             ((short)((a)-(b)) < 0)
#define IPID_LEQ(a,b)            ((short)((a)-(b)) <= 0)
#define IPID_GT(a,b)             ((short)((a)-(b)) > 0)
#define IPID_GEQ(a,b)            ((short)((a)-(b)) >= 0)

char *file;			/* input link trace files */
int (*nextpkt) (pkt_t * p);  // pointer to nextpkt function
