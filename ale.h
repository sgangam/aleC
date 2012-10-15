#include "cbfList.h"

enum _ale_type {
    U, E
};
typedef enum _ale_type ale_type;

typedef struct _Ale {
    unsigned int len;
    unsigned int counters;
    float W; //The span length (in seconds)
    ale_type t; 
    CBFList* head;

} Ale;

typedef struct _ReturnData {
    float rtt; //(in milli seconds)
    unsigned int valid;
} ReturnData;


void init_ale(Ale* ale,ale_type t,  float span_length, unsigned int window_count, unsigned int no_of_counters) {

    ale->W = span_length ;
    ale->len = window_count ;
    ale->counters = no_of_counters;
    ale->t = t;
    create_cbf_list(ale->head, window_count, no_of_counters) ;  // uses malloc to allocate memory
}

void cleanup_ale(Ale* ale)
{
    cleanup_cbf_list(ale->head);
}

void reset_ale(Ale* ale){
    reset_cbf_list(ale->head);
}

int get_RTT_sample(Ale* ale, ReturnData* rdata, pkt_t* pkt) {

    rdata->valid = 1;
    return 0;
}

