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

void initialize_ale_array(Ale* ale_array, u_int ale_method_count) {
    ale_type type = U; double span_length=2000 ;
    u_int no_of_counters = 60000;// for bloom filters
    //u_int min_bucket_count = 12
    u_int max_bucket_count = 96;
    u_int ale_e_min_bucket_count = 16;
    u_int bucket_count = max_bucket_count;
    u_int i = 0;
    if (ale_method_count == 1) {
        init_ale(&ale_array[0], type, span_length, bucket_count, no_of_counters);
        return;
    }

    for (i = 0; i < ale_method_count - 1; i++) {
        init_ale(&ale_array[i], type, span_length, bucket_count, no_of_counters);
        bucket_count = bucket_count/2.0;
    }
    type = E;
    init_ale(&ale_array[ale_method_count - 1], type, span_length, ale_e_min_bucket_count, no_of_counters);
}

void process_ale_array_packet(Ale* ale_array, u_int ale_method_count, pkt_t* pkt){
    u_int i = 0;
    ReturnData rdata; rdata.rtt_valid = 0; rdata.rtt = 0; //initialize
    for (i = 0; i < ale_method_count; i++) {
        get_RTT_sample(&ale_array[i], &rdata, pkt);
        if (rdata.rtt_valid == 1)
            printPacketStdout(&ale_array[i], pkt, rdata.rtt);
    }
}

void cleanup_ale_array(Ale* ale_array, u_int ale_method_count){
    u_int i = 0;
    for (i = 0; i < ale_method_count; i++) {
        cleanup_ale(&ale_array[i]);
    }
}

void processtrace()
{
    pkt_t pkt;
    int trace;
    trace=nextpkt(&pkt);
    if (trace < 0)
	err(EXIT_FAILURE, "readnextpkt");

    u_int ale_method_count = 5;
    Ale ale_array[ale_method_count];
    initialize_ale_array(ale_array, ale_method_count);
    /* start reading the trace */
    while (trace != 0) {
	if (pkt.ih.proto != 6) {
            continue;
        }
        process_ale_array_packet(ale_array, ale_method_count, &pkt);
        trace = nextpkt(&pkt);
    }
    cleanup_ale_array(ale_array, ale_method_count);
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
