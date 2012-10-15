#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <error.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "ale.h"
#include "readtraces.h"
#include <stdio.h>

int is_tcpdump(char *filename);
int pread_tcpdump(struct timeval *ptime, int *plen, int	*ptlen, void **pphys, 
		  int *pphystype, struct ip **ppip, void **pplast);

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#define BUFSIZE	 1024

int  
nextpkt_TCPDUMP(pkt_t * pkt) {
  
  int len;
  int tlen;
  void *plast;
  void *phys; /* physical transport header */
  int phystype;
  struct ip *pip;
  
  struct timeval time;
  int ret;
	
  struct tcphdr *ptcp;
  struct udphdr *pudp;
  
  ret = pread_tcpdump(&time,&len,&tlen,&phys,&phystype,&pip,&plast);
  if (ret == 0)  /* EOF or low level some error */
    return 0;
  
  /* initialize our packet */
  bzero((void *)pkt, sizeof(pkt_t));
  
  /* make sure we have enough of the packet */
  if ((char *)pip+sizeof(struct ip)-1 > (char *)plast) {
    return -1;
  }

  /* Only consider IPv4 packets */
  if (!PIP_ISV4(pip))
    return -1;

  /* check the fragment field, if it's not the first fragment,
     it's useless (offset part of field must be 0) */
  if ((ntohs(pip->ip_off)&0x1fff) != 0) {
    return -1;
  }

  /* get the packet time */
  pkt->time = TIME2DAG(time.tv_sec, time.tv_usec);

  /* get packet size */
  pkt->caplen = tlen;
  pkt->len = len;

  /* IP protocol and header info */
  IP(ihl) = pip->ip_hl;
  IP(ver) = pip->ip_v;
  IP(tos) = pip->ip_tos;
  N16(IP(len)) = pip->ip_len;
  N16(IP(id)) = pip->ip_id;
  N16(IP(ofs)) = pip->ip_off;
  IP(ttl) = pip->ip_ttl;
  IP(proto) = pip->ip_p;

  /* IP addresses */
  N32(IP(src_ip)) = pip->ip_src.s_addr;
  N32(IP(dst_ip)) = pip->ip_dst.s_addr;

  /* look at the transport protocol */
  switch (pip->ip_p) {
  case IPPROTO_TCP :   
    /* TCP header */
    ptcp = (struct tcphdr *) ((char *)pip + 4*IP_HL(pip));

    /* make sure we have enough of the packet */
    if ((char *)ptcp+sizeof(struct tcphdr)-1 > (char *)((char *)pip + ntohs(pip->ip_len)))
      return -1;

    /* copy TCP header info */
    N16(TCP(src_port)) = ptcp->th_sport;
    N16(TCP(dst_port)) = ptcp->th_dport;
    N32(TCP(seq)) = ptcp->th_seq;
    N32(TCP(ack)) =  ptcp->th_ack;
    TCP(unused) = ptcp->th_x2;
    TCP(hlen) = ptcp->th_off;
    TCP(flags) = ptcp->th_flags;
    N16(TCP(window)) = ptcp->th_win;
    
    break;

  case IPPROTO_UDP :
    /* UDP header */
    pudp = (struct udphdr *) ((char *)pip + 4*IP_HL(pip));

    /* make sure we have enough of the packet */
    if ((char *)pudp+sizeof(struct udphdr)-1 > (char *)((char *)pip + ntohs(pip->ip_len)))
      return -1;

    /* copy UDP header info */
    N16(UDP(src_port)) = pudp->uh_sport;
    N16(UDP(dst_port)) = pudp->uh_dport;
    N16(UDP(len)) = pudp->uh_ulen;
    N16(UDP(cksum)) = pudp->uh_sum;
    
    break;

  default: 
    return -1;
  }

  return 1;
}

int
open_tracefile(char* file) {

    if (!(is_tcpdump(file))) {
      fprintf(stderr," File format IS NOT tcpdump\n");
      return -1;
    }

    nextpkt = nextpkt_TCPDUMP;

  return 1;
}

