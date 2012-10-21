#ifndef _PKTHEADERS_H
#define _PKTHEADERS_H

#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <net/ethernet.h>	/* ETHERTYPE_IP */
#include  <netinet/in.h>
#include  <arpa/inet.h>		/* inet_ntop */
# include <inttypes.h>
# include <sys/time.h>


/* GPP record type defines */
#define TYPE_LEGACY       0
#define TYPE_HDLC_POS     1
#define TYPE_ETH          2
#define TYPE_ATM          3
#define TYPE_AAL5         4
#define TYPE_MC_HDLC      5	// for dag3.7t
#define TYPE_MC_RAW       6	// for dag3.7t

#define dag_record_size   16

/* GPP Type 1 */
typedef struct pos_rec {
    uint32_t hdlc;
    uint8_t pload[1];
} pos_rec_t;

/* GPP Type 2 */
typedef struct eth_rec {
    uint8_t offset;
    uint8_t pad;
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t etype;
    uint8_t pload[1];
} eth_rec_t;

/* GPP Type 3 */
typedef struct atm_rec {
    uint32_t header;
    uint8_t pload[1];
} atm_rec_t;

/* GPP Type 4 */
typedef struct aal5_rec {
    uint32_t header;
    uint8_t pload[1];
} aal5_rec_t;

/* GPP Type 5  */
/* for dag3.7t */
typedef struct mc_hdlc_rec {
    uint32_t channel;
    uint8_t pload[1];
} mc_hdlc_rec_t;

/* GPP Type 6  */
/* for dag3.7t */
typedef struct mc_raw_rec {
    uint32_t channel;
    uint8_t pload[1];
} mc_raw_rec_t;


typedef struct flags {
    uint8_t iface:2;
    uint8_t vlen:1;
    uint8_t trunc:1;
    uint8_t rxerror:1;
    uint8_t dserror:1;
    uint8_t pad:2;
} flags_t;

/* GPP Global type */
typedef struct dag_record {
#ifndef _WIN32
    uint64_t ts;
#else				/* _WIN32 */
    ULONGLONG ts;
#endif				/* _WIN32 */
    uint8_t type;
    flags_t flags;
    uint16_t rlen;
    uint16_t lctr;
    uint16_t wlen;
    union {
	pos_rec_t pos;
	eth_rec_t eth;
	atm_rec_t atm;
	aal5_rec_t aal5;
	mc_hdlc_rec_t mc_hdlc;
	mc_raw_rec_t mc_raw;
    } rec;
} dag_record_t;

/*
 * Object types
 */
typedef struct _pkt pkt_t;	/* packet descriptor */

/* typedefs for entries in network format. C99 types are used for native.
 * This makes the code a bit more boring to write, but safer in that
 * the compiler can catch error for us.
 * Access to network fields must be mediated by the 'N16()' and N32() macros
 * so it is easier to spot violations (never access explicitly
 * the field names).
 * Also never use explicitly the ntoh*(), hton*() macros.
 */
#define	N16(x)	((x).__x16)
#define	N32(x)	((x).__x32)
#define	H16(x)	(ntohs(N16(x)))
#define	H32(x)	(ntohl(N32(x)))

struct _n32_t {
    uint32_t __x32;
};

struct _n16_t {
    uint16_t __x16;
};
typedef struct _n32_t n32_t;	/* network format */
typedef struct _n16_t n16_t;	/* network format */


struct _ethdr {
    char dst[6];
    char src[6];
    n16_t type;
};

struct _hdlchdr {
    n32_t hdlc;
};

struct _iphdr {		/* possibly have __packed */
    uint8_t ihl:4;
    uint8_t ver:4;
    uint8_t tos; 
    n16_t len;
    n16_t id;
    n16_t ofs;			/* and flags */
    uint8_t ttl;
    uint8_t proto;
    n16_t cksum;
    n32_t src_ip;
    n32_t dst_ip;
};

struct _tcphdr {
    n16_t src_port;
    n16_t dst_port;
    n32_t seq;
    n32_t ack;
    u_char unused:4, hlen:4;
    uint8_t flags;
    n16_t window;
    n16_t cksum;
    n16_t urg;
};


struct _udphdr {
    n16_t src_port;
    n16_t dst_port;
    n16_t len;
    n16_t cksum;
};

struct _icmphdr {
};

/*
 * macros to use for packet header fields
 * There are more macros in modules/generic_filter.c
 */

#define IP(field)	(pkt->ih.field)
#define TCP(field)	(pkt->p.tcph.field)
#define UDP(field)	(pkt->p.udph.field)


/*
 * struct _pkt (pkt_t) is the structure describing a packet
 * passed around the capture module.
 */

struct _pkt {
    uint64_t time;		/* timestamp */
    uint32_t caplen;		/* capture length */
    uint32_t len;		/* length on the wire */
    uint32_t n_frags;		/* how many fragments ? */
    uint8_t dir;		/* traffic direction, if applicable */
    union {			/* the mac header */
	struct _ethdr eth;
	struct _hdlchdr hdlch;
    } l;
    struct _iphdr ih;	/* the ip header */
    union {
	struct _tcphdr tcph;
	struct _udphdr udph;
	struct _icmphdr icmph;
    } p;
    char data[4];
};

/* 
 * timestamp macros 
 */
#define DAG2SEC(t)    ((u_int32_t) ((t) >> 32))
#define DAG2MSEC(t)   ((u_int32_t) ((((t) & 0xffffffff) * 1000) >> 32))
#define DAG2USEC(t)   ((u_int32_t) ((((t) & 0xffffffff) * 1000000) >> 32))
#define TIME2DAG(s, u)				\
	 ((((u_int64_t) (s)) << 32) + ((((u_int64_t) (u)) << 32) / 1000000))


#endif				/* _PKTHEADERS_H */


struct flow_id {
    uint32_t dst_ip, src_ip;
    uint16_t dst_port, src_port;
    uint8_t proto;
};

struct rule {
    struct flow_id filter;
    struct flow_id mask;
    struct rule *next;
};


