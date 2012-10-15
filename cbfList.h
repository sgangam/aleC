#include "cbf.h"

typedef struct _CBFList{
    //CBF* head;
    CBF* current;
    CBF* next;
    CBF* prev;
} CBFList;

void create_cbf_list(CBFList* head, unsigned int len, unsigned int no_of_counters) {  // uses malloc to allocate memory
    
}

void cleanup_cbf_list(CBFList* head){
}

void reset_cbf_list(CBFList* head){

}
