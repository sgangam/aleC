#include "cbfList.h"
#include <assert.h>
#ifndef _PKTHEADERS_H
#include "pktHeaders.h"
#endif
#define BUFSIZE	 1024
#define STR_BUFLEN 1024

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

    snprintf(out_string, STR_BUFLEN, "%.6f %s %s %u %u %u", last + (last_us/1000000.0), 
            str_src_ip, str_dst_ip,
            sport, dport, ack_no);
}

void printPacketStdout(pkt_t* pkt, double rtt){
    char out_string[STR_BUFLEN];
    memset(out_string,'\0',STR_BUFLEN);
    printPacket(pkt, out_string);
    printf("%s %f\n", out_string, rtt);
}

enum _ale_type {
    U, E
};
typedef enum _ale_type ale_type;

typedef struct _Ale {
    u_int len; //This includes the current cbf which is the first element of the list.
    u_int counters;
    double W; //The span length (in milli seconds)
    double w; //The width of the time bucket B[0]. in milliseconds.
    ale_type t; 
    CBFList cbfl;
    double ts; // The timestamp of the Ale structure. Points to the max timestamp value of any packet stored in the CBFlist.

} Ale;

typedef struct _ReturnData {
    double rtt; //(in milli seconds)
    u_int rtt_valid;
} ReturnData;

void init_ale(Ale* ale, ale_type t,  double span_length, u_int window_count, u_int no_of_counters) {
    //printf("Ale length: %u\n", window_count);
    ale->W = span_length ;
    ale->len = window_count ;
    ale->counters = no_of_counters;
    ale->t = t;
    ale->ts = 0;
    if (t == U)
        ale->w = ale->W*1.0/ale->len ;
    else if (t == E) { //TODO
        assert(0);
    }

    create_cbf_list(&ale->cbfl, ale->len, ale->counters) ;  // uses malloc to allocate memory
    assert(ale->len == list_get_size(&ale->cbfl));
}

void cleanup_ale(Ale* ale)
{
    cleanup_cbf_list(&ale->cbfl);
}

void reset_ale(Ale* ale){
    reset_cbf_list(&ale->cbfl);
}

u_int get_pop_index(Ale* ale) {
    if (ale->t == U)
        return ale->len - 2 ;
    else if (ale->t == E){
        assert(0);
        return 0;
    //TODO
    }
    else
        assert(0);
}
void update_cbflist(Ale* ale, pkt_t* pkt) {
    u_int32_t last = DAG2SEC(pkt->time);
    u_int32_t last_us = DAG2USEC(pkt->time);
    double ts = last + (last_us/1000000.0); 

    if (ale->ts == 0) {
        ale->ts = ts;
        return;
    }
    while (ts >= ale->ts + ale->w/1000.0) {
        u_int index = get_pop_index(ale);
        if (index == ale->len - 2) // optimizing common case.
            list_pop_back(&ale->cbfl);
        else
            list_pop_index(&ale->cbfl, index);

        ale->ts = ale->ts + ((ale->w)/1000.0) ;
        append_empty_nodes_head(&ale->cbfl, 1, ale->counters);
        assert(ale->len == list_get_size(&ale->cbfl));
    }
    
}

void get_rtt_from_index(Ale* ale, pkt_t* pkt, ReturnData* rdata, u_int index) {
    u_int32_t last = DAG2SEC(pkt->time);
    u_int32_t last_us = DAG2USEC(pkt->time);
    double ts = last + (last_us/1000000.0);
    rdata->rtt_valid = 1;
    if (index == -1)
        rdata->rtt = (ts - ale->ts)*1000/2.0 ;// Latency in milliseconds (Actually it is a+b/2  - b = a-b/ = a-b/22)
    else if (index >= 0) {
        if (ale->t == U)
            rdata->rtt = (index + 0.5) * ale->w + ((ts - ale->ts)*1000);
        else if (ale->t == E) {
            //TODO
            assert(0);
            double min =  0, max = 0;
            rdata->rtt  = ((max + min)*0.5*ale->w) +  ((ts - ale->ts)*1000);
        }
        else 
            assert(0);
    }
}

Entry get_hash_value (u_int ack, u_int a, u_int b, u_int c, u_int d) {
    u_int array[5];
    array[0] = ack; array[1] = a; array[2] = b; array[3] = c; array[4] = d; 
    return hashword(array, 5, 0);
} 

void process_data(Ale* ale, pkt_t* pkt) {
    u_int pktlen = (int) H16(IP(len));
    u_int payload = pktlen - (IP(ihl) << 2) - ((TCP(hlen) & 0x0f) << 2);
    if (TCP(flags) & TH_SYN )
        payload++;
    if (TCP(flags) & TH_FIN)
        payload++;
    if (payload < 0){
        assert(0);
    }
    if (payload == 0) {
        return;
    }
    u_int expected_ack = H32(TCP(seq)) + payload;
    Entry entry = get_hash_value(expected_ack, N32(IP(src_ip)), N32(IP(dst_ip)), H16(TCP(src_port)), H16(TCP(dst_port)));
    //printf("expected_ack:%u , hashed value: %u \n",expected_ack, entry);
    int index = lookup_and_remove_cbf_list_entry(&ale->cbfl, entry);
    assert (index >=-2 && index <= (int)(ale->len - 2));
    if (index == -2)
        add_cbf_list_entry(&ale->cbfl, entry);
}

void process_ack(Ale* ale,  ReturnData* rdata, pkt_t* pkt) {
    Entry entry = get_hash_value(H32(TCP(ack)), N32(IP(dst_ip)), N32(IP(src_ip)), H16(TCP(dst_port)), H16(TCP(src_port)) ); // Note the hash order is different.
    //printf("hashed value: %u \n", entry);
    int index = lookup_and_remove_cbf_list_entry(&ale->cbfl, entry);
    assert (index >=-2 && index <= (int)(ale->len - 2));
    if (index >= -1) 
        get_rtt_from_index(ale, pkt, rdata, index); 
}

void get_RTT_sample(Ale* ale, ReturnData* rdata, pkt_t* pkt) {
    update_cbflist(ale, pkt); // Check if we have to push some cbf's and add new empty ones.
    process_data(ale, pkt);
    process_ack(ale, rdata, pkt);
}
