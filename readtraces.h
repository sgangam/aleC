/*
 * Copyright (c) 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
 *               2002, 2003, 2004
 *	Ohio University.
 *
 * ---
 * 
 * Starting with the release of tcptrace version 6 in 2001, tcptrace
 * is licensed under the GNU General Public License (GPL).  We believe
 * that, among the available licenses, the GPL will do the best job of
 * allowing tcptrace to continue to be a valuable, freely-available
 * and well-maintained tool for the networking community.
 *
 * Previous versions of tcptrace were released under a license that
 * was much less restrictive with respect to how tcptrace could be
 * used in commercial products.  Because of this, I am willing to
 * consider alternate license arrangements as allowed in Section 10 of
 * the GNU GPL.  Before I would consider licensing tcptrace under an
 * alternate agreement with a particular individual or company,
 * however, I would have to be convinced that such an alternative
 * would be to the greater benefit of the networking community.
 * 
 * ---
 *
 * This file is part of Tcptrace.
 *
 * Tcptrace was originally written and continues to be maintained by
 * Shawn Ostermann with the help of a group of devoted students and
 * users (see the file 'THANKS').  The work on tcptrace has been made
 * possible over the years through the generous support of NASA GRC,
 * the National Science Foundation, and Sun Microsystems.
 *
 * Tcptrace is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Tcptrace is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tcptrace (in the file 'COPYING'); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 * 
 * Author:	Shawn Ostermann
 * 		School of Electrical Engineering and Computer Science
 * 		Ohio University
 * 		Athens, OH
 *		ostermann@cs.ohiou.edu
 *		http://www.tcptrace.org/
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>



/* several places in the code NEED numbers of a specific size. */
/* since the definitions aren't standard across everything we're */
/* trying to support, the types are gathered up here */
/* specifically, we need:
   tt_uint32	unsigned 32 bit 
   tt_uint16	unsigned 16 bit 
   tt_int32	signed 32 bit 
   tt_int16	signed 16 bit
*/

/* first, do the 32 bit ones */
typedef unsigned long tt_uint32;
typedef          long tt_int32;

/* first, do the 16 bit ones */
typedef unsigned short tt_uint16;
typedef          short tt_int16;


/* type for a TCP sequence number, ACK, FIN, or SYN */
/* This type MUST be a 32-bit unsigned number */
typedef tt_uint32 seqnum;

/* length of a segment */
typedef u_long seglen;

/* type for a quadrant number */
typedef u_char quadnum;  /* 1,2,3,4 */

/* type for a TCP port number */
typedef u_short portnum;

/* type for an IP address */
/* IP address can be either IPv4 or IPv6 */
typedef struct ipaddr {
    u_char addr_vers;	/* 4 or 6 */
    union {
	struct in_addr   ip4;
	struct in6_addr  ip6;
    } un;
} ipaddr;


/* some machines (TRUE64 for one) handle the 4-bit TCP/IP fields
   differently, so this macro simplifies life */
#define IP_HL(pip)   ((pip)->ip_hl)
#define IP_V(pip)    ((pip)->ip_v)
#define TH_X2(ptcp)  ((ptcp)->th_x2)
#define TH_OFF(ptcp) ((ptcp)->th_off)

/* type for a Boolean */
typedef u_char Bool;
#define TRUE	1
#define FALSE	0

/* ACK types */
enum t_ack {NORMAL = 1,		/* no retransmits, just advance */
	    AMBIG = 2,		/* segment ACKed was rexmitted */
	    CUMUL = 3,		/* doesn't advance */
	    TRIPLE = 4,		/* triple dupack */
	    NOSAMP = 5};	/* covers retransmitted segs, no rtt sample */

/* type for an internal file pointer */
typedef struct mfile MFILE;


typedef struct tcphdr tcphdr;

// typedef struct sudp_pair udp_pair;
typedef struct udphdr udphdr;

/* global routine decls */
void *MallocZ(int);
void *ReallocZ(void *oldptr, int obytes, int nbytes);

/* TCP flags macros */
#define SYN_SET(ptcp)((ptcp)->th_flags & TH_SYN)
#define FIN_SET(ptcp)((ptcp)->th_flags & TH_FIN)
#define ACK_SET(ptcp)((ptcp)->th_flags & TH_ACK)
#define RESET_SET(ptcp)((ptcp)->th_flags & TH_RST)
#define PUSH_SET(ptcp)((ptcp)->th_flags & TH_PUSH)
#define URGENT_SET(ptcp)((ptcp)->th_flags & TH_URG)
#define FLAG6_SET(ptcp)((ptcp)->th_flags & 0x40)
#define FLAG7_SET(ptcp)((ptcp)->th_flags & 0x80)

/* Changed the following macros to reflect the correct position
of bits as specified in RFC 2481 and draft-ietf-tsvwg-ecn-04.txt */
/*
	#define CWR_SET(ptcp)     (TH_X2((ptcp)) & TH_CWR)
	#define ECN_ECHO_SET(ptcp)(TH_X2((ptcp)) & TH_ECN_ECHO)
*/

#define CWR_SET(ptcp)	(TH_FLAGS((ptcp)) & TH_CWR)
#define ECN_ECHO_SET(ptcp)	(TH_FLAGS((ptcp)) & TH_ECN_ECHO)

/* connection directions */
#define A2B 1
#define B2A -1

/* If the AVL node is to left or right in the AVL tree */
/* Words "LEFT" and "RIGHT" have already been taken in an enum
 * above. Let us call them LT, RT just not to make it ambiguous for ourselves
 * or the compiler or both :-)
 */
#define LT -2
#define RT 2

/*macros for maintaining the seqspace used for rexmit*/
#define QUADSIZE	(0x40000000)
#define QUADNUM(seq)	((seq>>30)+1)
#define IN_Q1(seq)	(QUADNUM(seq)==1)
#define IN_Q2(seq)	(QUADNUM(seq)==2)
#define IN_Q3(seq)	(QUADNUM(seq)==3)
#define IN_Q4(seq)	(QUADNUM(seq)==4)
#define FIRST_SEQ(quadnum)	(QUADSIZE*(quadnum-1))
#define LAST_SEQ(quadnum)	((QUADSIZE*quadnum)-1) /* bug fix by Priya */
#define BOUNDARY(beg,fin) (QUADNUM((beg)) != QUADNUM((fin)))


/* physical layers currently understood					*/
#define PHYS_ETHER	1
#define PHYS_FDDI       2

/*
 * SEQCMP - sequence space comparator
 *	This handles sequence space wrap-around. Overlow/Underflow makes
 * the result below correct ( -, 0, + ) for any a, b in the sequence
 * space. Results:	result	implies
 *			  - 	 a < b
 *			  0 	 a = b
 *			  + 	 a > b
 */
#define	SEQCMP(a, b)		((long)(a) - (long)(b))
#define	SEQ_LESSTHAN(a, b)	(SEQCMP(a,b) < 0)
#define	SEQ_GREATERTHAN(a, b)	(SEQCMP(a,b) > 0)


/* SACK TCP options (not an RFC yet, mostly from draft and RFC 1072) */
/* I'm assuming, for now, that the draft version is correct */
/* sdo -- Tue Aug 20, 1996 */
#define	TCPOPT_SACK_PERM 4	/* sack-permitted option */
#define	TCPOPT_SACK      5	/* sack attached option */
#define	MAX_SACKS       10	/* max number of sacks per segment (rfc1072) */
typedef struct sack_block {
    seqnum	sack_left;	/* left edge */
    seqnum	sack_right;	/* right edge */
} sack_block;

#define MAX_UNKNOWN 16
typedef struct opt_unknown {
    u_char	unkn_opt;
    u_char	unkn_len;
} opt_unknown;

/* RFC 1323 TCP options (not usually in tcp.h yet) */
#define	TCPOPT_WS	3	/* window scaling */
#define	TCPOPT_TS	8	/* timestamp */

/* other options... */
#define	TCPOPT_ECHO		6	/* echo (rfc1072) */
#define	TCPOPT_ECHOREPLY	7	/* echo (rfc1072) */
#define TCPOPT_TIMESTAMP	8	/* timestamps (rfc1323) */
#define TCPOPT_CC		11	/* T/TCP CC options (rfc1644) */
#define TCPOPT_CCNEW		12	/* T/TCP CC options (rfc1644) */
#define TCPOPT_CCECHO		13	/* T/TCP CC options (rfc1644) */

/* RFC 2481 (ECN) IP and TCP flags (not usually defined yet) */
#define IPTOS_ECT	0x02	/* ECN-Capable Transport */
#define IPTOS_CE	0x01	/* Experienced Congestion */

// Modified the following macros to reflect the
// correct bit positions for CWR and ECE as specified in 
// RFC 2481 and the latest draft: draft-ietf-tsvwg-ecn-04.txt.
// The bits CWR and ECE are actually the most significant
// bits in the TCP flags octet respectively.

/*#define TH_ECN_ECHO	0x02 */	/* Used by receiver to echo CE bit */
/*#define TH_CWR		0x01 */	/* Congestion Window Reduced */

#define TH_CWR 0x80			/* Used by sender to indicate congestion
							   window size reduction. */
#define TH_ECN_ECHO 0x40	/* Used by receiver to echo CE bit. */


/* some compilers seem to want to make "char" unsigned by default, */
/* which is breaking stuff.  Rather than introduce (more) ugly */
/* machine dependencies, I'm going to FORCE some chars to be */
/* signed... */
typedef signed char s_char;

struct tcp_options {
    short	mss;		/* maximum segment size 	*/
    s_char	ws;		/* window scale (1323) 		*/
    long	tsval;		/* Time Stamp Val (1323)	*/
    long	tsecr;		/* Time Stamp Echo Reply (1323)	*/

    Bool	sack_req;	/* sacks requested 		*/
    s_char	sack_count;	/* sack count in this packet */
    sack_block	sacks[MAX_SACKS]; /* sack blocks */

    /* echo request and reply */
    /* assume that value of -1 means unused  (?) */
    u_long	echo_req;
    u_long	echo_repl;

    /* T/TCP stuff */
    /* assume that value of -1 means unused  (?) */
    u_long	cc;
    u_long	ccnew;
    u_long	ccecho;

    /* record the stuff we don't understand, too */
    char	unknown_count;	/* number of unknown options */
    opt_unknown	unknowns[MAX_UNKNOWN]; /* unknown options */
};


/* packet-reading options... */
/* the type for a packet reading routine */
typedef int pread_f(struct timeval *, int *, int *, void **,
		   int *, struct ip **, void **);


/*
 * timeval compare macros
 */
#define tv_ge(lhs,rhs) (tv_cmp((lhs),(rhs)) >= 0)
#define tv_gt(lhs,rhs) (tv_cmp((lhs),(rhs)) >  0)
#define tv_le(lhs,rhs) (tv_cmp((lhs),(rhs)) <= 0)
#define tv_lt(lhs,rhs) (tv_cmp((lhs),(rhs)) <  0)
#define tv_eq(lhs,rhs) (tv_cmp((lhs),(rhs)) == 0)

/* handy constants */
#define US_PER_SEC 1000000	/* microseconds per second */
#define MS_PER_SEC 1000		/* milliseconds per second */


/*
 * Macros to simplify access to IPv4/IPv6 header fields
 */
#define PIP_VERS(pip) (IP_V((struct ip *)(pip)))
#define PIP_ISV6(pip) (PIP_VERS(pip) == 6)
#define PIP_ISV4(pip) (PIP_VERS(pip) == 4)
#define PIP_V6(pip) ((struct ipv6 *)(pip))
#define PIP_V4(pip) ((struct ip *)(pip))
#define PIP_EITHERFIELD(pip,fld4,fld6) \
   (PIP_ISV4(pip)?(PIP_V4(pip)->fld4):(PIP_V6(pip)->fld6))
#define PIP_LEN(pip) (PIP_EITHERFIELD(pip,ip_len,ip6_lngth))

/*
 * Macros to simplify access to IPv4/IPv6 addresses
 */
#define ADDR_VERSION(paddr) ((paddr)->addr_vers)
#define ADDR_ISV4(paddr) (ADDR_VERSION((paddr)) == 4)
#define ADDR_ISV6(paddr) (ADDR_VERSION((paddr)) == 6)
struct ipaddr *IPV4ADDR2ADDR(struct in_addr *addr4);    
struct ipaddr *IPV6ADDR2ADDR(struct in6_addr *addr6);

/*
 * Macros to check for congestion experienced bits
 */
#define IP_CE(pip) (((struct ip *)(pip))->ip_tos & IPTOS_CE)
#define IP_ECT(pip) (((struct ip *)(pip))->ip_tos & IPTOS_ECT)

/*
 * fixes for various systems that aren't exactly like Solaris
 */
#ifndef IP_MAXPACKET
#define IP_MAXPACKET 65535
#endif /* IP_MAXPACKET */

/* max 32 bit number */
#define MAX_32 (0x100000000LL)

#ifndef ETHERTYPE_REVARP
#define ETHERTYPE_REVARP        0x8035
#endif /* ETHERTYPE_REVARP */

#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN		0x8100
#endif	/* 802.1Q Virtual LAN */

/* support for PPPoE encapsulation added by Yann Samama (ysamama@nortelnetworks.com)*/
#ifndef ETHERTYPE_PPPOE_SESSION
#define ETHERTYPE_PPPOE_SESSION	0x8864
#endif /* PPPoE ether type */
#ifndef PPPOE_SIZE
#define PPPOE_SIZE		22
#endif /* PPPOE header size */
