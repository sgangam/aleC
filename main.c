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
#include "ale.h"
#include "main.h"

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


/* 
 * -- processtrace 
 * 
 * this function reads all the packets from the input 
 * traces, identifies the flow the packets belong and calls
 * the handlepkt to compute per connection statistics. 
 */


void processtrace()
{
    pkt_t pkt;
    int trace;
    trace=nextpkt(&pkt);
    if (trace < 0)
	err(EXIT_FAILURE, "readnextpkt");

    Ale ale;
    ale_type type=U; double span_length=2000 ; 
    u_int window_count = 97, no_of_counters = 40000;
    init_ale(&ale, type, span_length, window_count, no_of_counters);
    /* start reading the trace */
    while (trace != 0) {
	if (pkt.ih.proto != 6) {
            continue;
        }
        ReturnData rdata; rdata.rtt_valid = 0; rdata.rtt = 0; //initialize
        get_RTT_sample(&ale, &rdata, &pkt);
        if (rdata.rtt_valid == 1)
            printPacketStdout(&pkt, rdata.rtt);
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
