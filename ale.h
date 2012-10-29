#include "cbfList.h"
#include <assert.h>
#ifndef _PKTHEADERS_H
#include "pktHeaders.h"
#endif
#define BUFSIZE	 1024
#define STR_BUFLEN 1024
#define MAX_EXP_GROUPS 30 // We do not anticipate more than 30 groups.

//#define MAX(x, y) (((x) > (y)) ? (x) : (y))
//#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef enum _ale_type {
    U, E
} ale_type;

typedef struct _ExpIndexState {
    u_int min_bound;
    u_int max_bound;
    u_int min_offset;
    u_int max_offset;
    u_int group_id;
} ExpIndexState;

typedef struct _GroupState {
    u_int no_of_groups;
    u_int group_state_array[MAX_EXP_GROUPS];

}GroupState ;

typedef struct _ReturnData {
    double rtt; //(in milli seconds)
    u_int rtt_valid;
} ReturnData;


typedef struct _Ale {
    u_int len; //This includes the current cbf which is the first element of the list.
    u_int counters;
    double W; //The span length (in milli seconds)
    double w; //The width of the time bucket B[0]. in milliseconds.
    ale_type t; 
    CBFList cbfl;
    double ts; // The timestamp of the Ale structure. Points to the max timestamp value of any packet stored in the CBFlist.
    //The following are required only by ALE-E
    ExpIndexState* exp_index_state;
    GroupState* group_state;
    u_int time_bucket_state_counter;

} Ale;

//ALE-E has groups of buckets with increasing size (exponentially doubling)
//The number of buckets within a group are given by this function.
// the return value must be greater than zero and group Id starts from 0.
u_int  group_cardinality(u_int groupId) {
    //u_int retVal = 1 + groupId;
    u_int retVal = 1 + (u_int)(groupId/2);
    assert(retVal > 0);
    return retVal; 
}
//Additional initializations for ALE-E.
void init_ale_e (Ale* ale) {
    assert(ale->len > 0);
    ale->time_bucket_state_counter = 0;
    ale->exp_index_state = (ExpIndexState*) calloc(sizeof(ExpIndexState), ale->len);
    ale->group_state = (GroupState*) malloc(sizeof(GroupState));
    GroupState* group_state = ale->group_state;

    u_int group_id = 0, min_bound = 0, max_bound = 1, min_offset = 1, max_offset = 1, i; 
    u_int group_size = group_cardinality(group_id);

    group_state->no_of_groups = 0;
    group_state->group_state_array[group_state->no_of_groups] = (0 + group_size - 1);
    group_state->no_of_groups++;

    for (i = 0 ; i < ale->len ; i++) {
        if (group_size == 0) {
            group_id += 1;
            group_size = group_cardinality(group_id);
            group_state->group_state_array[group_state->no_of_groups] = (i + MIN(group_size, ale->len - i) - 1);
            group_state->no_of_groups++;
            max_offset = 1 << group_id;
        }
        ale->exp_index_state[i].min_bound = min_bound; ale->exp_index_state[i].max_bound = max_bound; 
        ale->exp_index_state[i].min_offset = min_offset; ale->exp_index_state[i].max_offset = max_offset; ale->exp_index_state[i].group_id = group_id;
        min_offset = max_offset;
        min_bound = max_bound;
        max_bound += (1 << group_id);
        group_size -= 1;
    }
    ale->w = ale->W*1.0/ale->exp_index_state[ale->len - 1].max_bound;
    //printf ("ALE-E ale->w:%f\n",ale->w);
}

void init_ale(Ale* ale, ale_type t,  double span_length, u_int bucket_count, u_int no_of_counters) {
    //printf("Ale length: %u\n", bucket_count);
    ale->W = span_length ;
    ale->len = bucket_count ;
    ale->counters = no_of_counters;
    ale->t = t;
    ale->ts = 0;

    create_cbf_list(&ale->cbfl, ale->len + 1, ale->counters) ;  // uses malloc to allocate memory.  use one more node in the list for the current bucket
    assert(ale->len + 1 == list_get_size(&ale->cbfl));

    if (t == U)
        ale->w = ale->W*1.0/ale->len ;
    else if (t == E) 
        init_ale_e(ale); 
}


void cleanup_ale(Ale* ale)
{
    cleanup_cbf_list(&ale->cbfl);
    if (ale->t == E) {
        free(ale->exp_index_state);
        free(ale->group_state);
    }
}

u_int get_pop_index(Ale* ale) {
    if (ale->t == U)
        return ale->len - 1 ; // the last bucket has index: ale->len - 1
    else if (ale->t == E){
        u_int pop_group_index = 0, i;
        for (i = 0; i < ale->len ; i++) {
            if (ale->time_bucket_state_counter & (1 << i)) {
                pop_group_index = i; //first get the group from which we want to pop a bucket
                break;
            }
        }
        ale->time_bucket_state_counter++;
        if (ale->time_bucket_state_counter == (1 << ale->group_state->no_of_groups) - 1)
            ale->time_bucket_state_counter = 0;
        return ale->group_state->group_state_array[pop_group_index];
    }
    else
        assert(0);
}
void ale_pop_index(Ale* ale, u_int pop_index) {
    assert(pop_index <= ale->len - 1 && pop_index >= 0);
    if (pop_index == ale->len - 1) // the last bucket. ALE-E or ALE-U
        list_pop_back(&ale->cbfl);
    else { // ALE-E
        assert(ale->t == E);
        list_combine_bucket(&ale->cbfl, pop_index);// add bloom filter contents of pop_index to pop_index + 1
        list_pop_index(&ale->cbfl, pop_index);
    }
    assert(ale->len + 1 == list_get_size(&ale->cbfl) + 1);
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
        u_int pop_index = get_pop_index(ale);
        ale_pop_index(ale, pop_index);
        ale->ts = ale->ts + ((ale->w)/1000.0) ;
        append_empty_nodes_head(&ale->cbfl, 1, ale->counters);
        assert(ale->len + 1 == list_get_size(&ale->cbfl));
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
            u_int min_bound = ale->exp_index_state[index].min_bound + (ale->time_bucket_state_counter % ale->exp_index_state[index].min_offset);
            u_int max_bound = ale->exp_index_state[index].max_bound + (ale->time_bucket_state_counter % ale->exp_index_state[index].max_offset);
            rdata->rtt  = ((max_bound + min_bound)*0.5*ale->w) +  ((ts - ale->ts)*1000);
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
    assert (index >=-2 && index <= (int)(ale->len - 1));
    if (index == -2)
        add_cbf_list_entry(&ale->cbfl, entry);
}

void process_ack(Ale* ale,  ReturnData* rdata, pkt_t* pkt) {
    Entry entry = get_hash_value(H32(TCP(ack)), N32(IP(dst_ip)), N32(IP(src_ip)), H16(TCP(dst_port)), H16(TCP(src_port)) ); // Note the hash order is different.
    //printf("hashed value: %u \n", entry);
    int index = lookup_and_remove_cbf_list_entry(&ale->cbfl, entry);
    assert (index >=-2 && index <= (int)(ale->len - 1));
    if (index >= -1) 
        get_rtt_from_index(ale, pkt, rdata, index); 
}

void get_RTT_sample(Ale* ale, ReturnData* rdata, pkt_t* pkt) {
    update_cbflist(ale, pkt); // Check if we have to push some cbf's and add new empty ones.
    process_data(ale, pkt);
    process_ack(ale, rdata, pkt);
}

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
             str_dst_ip, str_src_ip,
            dport, sport, ack_no);
}

void printPacketStdout(Ale* ale, pkt_t* pkt, double rtt) {
    char out_string[STR_BUFLEN];
    memset(out_string,'\0',STR_BUFLEN);
    printPacket(pkt, out_string);
    char ale_type[5]; ale_type[1] = '\0';
    if (ale->t == U)
        ale_type[0] = 'U'; 
    else if (ale->t == E)
        ale_type[0] = 'E';
    printf("%s %f %s(%d)\n", out_string, rtt, ale_type, ale->len);
}
