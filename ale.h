#include "cbfList.h"
#include "assert.h"
#include "hashfunctions.h"
#ifndef _PKTHEADERS_H
#include "pktHeaders.h"
#endif


enum _ale_type {
    U, E
};
typedef enum _ale_type ale_type;

typedef struct _Ale {
    u_int len;
    u_int counters;
    float W; //The span length (in milli seconds)
    float w; //The width of the time bucket B[0]. in milliseconds.
    ale_type t; 
    CBFList cbfl;
    float ts; // The timestamp of the Ale structure. Points to the max timestamp value of any packet stored in the CBFlist.

} Ale;

typedef struct _ReturnData {
    float rtt; //(in milli seconds)
    u_int rtt_valid;
} ReturnData;

void init_ale(Ale* ale, ale_type t,  float span_length, u_int window_count, u_int no_of_counters) {

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
}

void cleanup_ale(Ale* ale)
{
    cleanup_cbf_list(&ale->cbfl);
}

void reset_ale(Ale* ale){
    reset_cbf_list(&ale->cbfl);
}

void update_cbflist_u(Ale* ale, pkt_t* pkt) {
    u_int32_t last = DAG2SEC(pkt->time);
    u_int32_t last_us = DAG2USEC(pkt->time);
    float ts = last+(last_us/1000000.0) ;
    if (ale->ts == 0) {
        ale->ts = ts;
        return;
    }

    if (ts >= ale->ts + (ale->W + ale->w)/1000.0) {
        reset_ale(ale);
        ale->ts = ts;
        return;
    }
    else {
        u_int elapsed_windows = (u_int)((ts - ale->ts) / (ale->w/1000.0));
        assert (elapsed_windows > 0 && elapsed_windows < ale->len + 1);
        move_time_buckets(&ale->cbfl, elapsed_windows, ale->len, ale->counters);
    }
}

void get_rtt_from_index(Ale* ale, pkt_t* pkt, ReturnData* rdata, u_int index) {
    u_int32_t last = DAG2SEC(pkt->time);
    u_int32_t last_us = DAG2USEC(pkt->time);
    float ts = last+(last_us/1000000.0) ;
    rdata->rtt_valid = 1;
    if (index == -1)
        rdata->rtt = (ts - ale->ts)*1000/2.0 ;// Latency in milliseconds (Actually it is a+b/2  - b = a-b/ = a-b/22)
    else if (index >= 0 && index < ale->len) {
        if (ale->t == U)
            rdata->rtt = (index + 0.5) * ale->w + ((ts - ale->ts)*1000);
        else if (ale->t == E) {
            //TODO
            assert(0);
            float min =  0, max = 0;
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
    assert (index >=-2 && index <= (int)(ale->len -1));
    if (index == -2)
        add_cbf_list_entry(&ale->cbfl, entry);
}

void process_ack(Ale* ale,  ReturnData* rdata, pkt_t* pkt) {
    Entry entry = get_hash_value(H32(TCP(ack)), N32(IP(dst_ip)), N32(IP(src_ip)), H16(TCP(dst_port)), H16(TCP(src_port)) ); // Note the hash order is different.
    //printf("hashed value: %u \n", entry);
    int index = lookup_and_remove_cbf_list_entry(&ale->cbfl, entry);
    assert (index >=-2 && index <= (int)(ale->len -1));
    if (index >= -1) 
        get_rtt_from_index(ale, pkt, rdata, index); 
}

void get_RTT_sample(Ale* ale, ReturnData* rdata, pkt_t* pkt) {
    if (ale->t == U)
        update_cbflist_u(ale, pkt); // Check if we have to push some cbf's and add new empty ones.
    else
        assert(0); //TODO
    process_data(ale, pkt);
    process_ack(ale, rdata, pkt);
}
