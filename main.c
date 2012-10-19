#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <error.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "main.h"
#include "ale.h"

char *progname;			/* the program name */
int open_tracefile(char* file);



/* flow hashing function. 
 * XXX i don't know if it is a good one but 'til now 
 * it has been working quite well 
 */
#define HASH(id)                                                        \
    ( ((id).dst_ip ^ ((id).src_ip << 1) ^ 				\
       ((id).dst_port << 1) ^ (id).src_port) & (ENTRIES - 1) )


#define MAX_PKTS (sizeof(pkt_t))
#define BUFSIZE	 1024
#define STR_BUFLEN 1024


/* 
 * -- processtrace 
 * 
 * this function reads all the packets from the input 
 * traces, identifies the flow the packets belong and calls
 * the handlepkt to compute per connection statistics. 
 */
void printPacket(pkt_t* pkt, char* out_string) {
    struct in_addr inaddr_src_ip, inaddr_dst_ip;
    inaddr_src_ip.s_addr = N32(IP(src_ip))  ;
    inaddr_dst_ip.s_addr = N32(IP(dst_ip)) ;
    char str_src_ip[BUFSIZE];
    char str_dst_ip[BUFSIZE];
    memset(str_src_ip,'\0',STR_BUFLEN);
    memset(str_dst_ip,'\0',STR_BUFLEN);
    inet_ntop(AF_INET,& (inaddr_src_ip), str_src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET,& (inaddr_dst_ip), str_dst_ip, INET_ADDRSTRLEN);
    uint16_t sport = H16(TCP(src_port));
    uint16_t dport = H16(TCP(dst_port));
    u_int32_t last = DAG2SEC(pkt->time);
    u_int32_t last_us = DAG2USEC(pkt->time);
    u_int32_t ack_no = H32(TCP(ack));

    snprintf(out_string, STR_BUFLEN, "%.6f %s %s %u %u %u\n", last+(last_us/1000000.0), 
            str_src_ip, str_dst_ip,
            sport, dport, ack_no);
}

void processtrace()
{
    pkt_t pkt;
    int trace;
    trace=nextpkt(&pkt);
    if (trace < 0)
	err(EXIT_FAILURE, "readnextpkt");

    Ale ale;
    ale_type type=U; float span_length=2000 ; 
    u_int window_count = 96, no_of_counters = 20000;
    init_ale(&ale, type, span_length, window_count, no_of_counters);
    /* start reading the trace */
    while (trace != 0) {
	if (pkt.ih.proto != 6) {
            continue;
        }
        ReturnData rdata; rdata.rtt_valid = 0; rdata.rtt = 0; //initialize
        get_RTT_sample(&ale, &rdata, &pkt);
        if (rdata.rtt_valid == 1) {
            char out_string[STR_BUFLEN];
            memset(out_string,'\0',STR_BUFLEN);
            printPacket(&pkt, out_string);
            printf("%s", out_string);
        }
        trace = nextpkt(&pkt);
    }
    cleanup_ale(&ale);
}

/* 
 * -- main 
 * 
 * this function first initializes the structures, parses 
 * the command line parameters,  opens input and output files and
 * then calls processtrace to process the input traces. 
 */
int
main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Usage: <program> pcapFile\n");
        return -1;
    }
    /*
    struct timeval start, finish;
    int hh, mm, ss;
    int i;

    gettimeofday(&start, NULL);
    */
    /* parse command line */
    progname = argv[0];
    file =  argv[1];

    /* open trace files */
    if (open_tracefile(file) < 0)
      return EXIT_FAILURE;

    /* process the trace */
    processtrace();

    return EXIT_SUCCESS;
}
