#include "cbf.h"
#include <assert.h>

typedef struct _ListNode {
    struct _ListNode* next;
    struct _ListNode* prev;
    CBF cbf;
}ListNode;

typedef struct _CBFList{
    ListNode* head;
    ListNode* tail;
    u_int len; // length of the List.
} CBFList;

u_int list_get_size (CBFList* cbfl) {
    return cbfl->len;
}

void create_empty_list (CBFList* cbfl) {
    cbfl->len = 0;
    cbfl->head = (ListNode*) malloc(sizeof(ListNode));
    cbfl->tail = (ListNode*) malloc(sizeof(ListNode));
    cbfl->head->next = cbfl->tail;
    cbfl->head->prev = NULL;
    cbfl->tail->prev = cbfl->head;
    cbfl->tail->next = NULL;
}

void list_insert_after(ListNode* node, ListNode* new_node) {
    ListNode* next_node = node->next;
    new_node->prev = node;
    new_node->next = next_node;
    next_node->prev = new_node;
    node->next = new_node;
}

void list_pop_node(ListNode* node) {
    ListNode* next_node = node->next;
    ListNode* prev_node = node->prev;
    prev_node->next = next_node;
    next_node->prev = prev_node;
    cleanup_cbf(&node->cbf);
    free(node);
}

void list_push_front(CBFList* cbfl, ListNode* new_node) {
    list_insert_after(cbfl->head, new_node);
    cbfl->len++;
    assert(cbfl->head->prev == NULL);
    assert(cbfl->head->next->prev == cbfl->head);
}

void list_pop_back(CBFList* cbfl) {
    list_pop_node(cbfl->tail->prev);
    cbfl->len--;
    assert(cbfl->tail->next == NULL);
    assert(cbfl->tail->prev->next == cbfl->tail);
}

void list_pop_index(CBFList* cbfl, u_int index) { // index 0 is the first element in the list
    assert (index>=0 && index < cbfl->len - 1); // cbfl->len has 1 element more than reqd.
    ListNode* curr = cbfl->head->next;
    u_int current_index = -1;
    while (curr != cbfl->tail) {
        if (index == current_index) {
            list_pop_node(curr);
            cbfl->len--;
            return;
        }
        current_index++;
        curr = curr->next;
    }
    assert(0);
}

void append_empty_nodes_head(CBFList* cbfl, u_int len, u_int no_of_counters) {  // uses malloc to allocate memory
    int i;
    for (i=0;i<len;i++) {
        ListNode* new_node = (ListNode*) malloc(sizeof(ListNode));
        create_cbf(&new_node->cbf, no_of_counters);
        list_push_front(cbfl, new_node);
    }
}


void create_cbf_list(CBFList* cbfl, u_int len, u_int no_of_counters) { 
    create_empty_list(cbfl);
    append_empty_nodes_head(cbfl, len, no_of_counters);
}

void clear_list(CBFList* cbfl){
    if (cbfl->len == 0)
        return;
    ListNode* curr = cbfl->head->next;
    while (curr != cbfl->tail) {
        cleanup_cbf(&curr->cbf);
        ListNode* next_ptr = curr->next;
        free(curr);
        cbfl->len--;
        curr = next_ptr;
    }
}

void cleanup_cbf_list(CBFList* cbfl){
    clear_list(cbfl);
    free(cbfl->head);
    free(cbfl->tail);
}

void reset_cbf_list(CBFList* cbfl){
    ListNode* curr = cbfl->head->next;
    while (curr != cbfl->tail)  {
        reset_cbf(&curr->cbf);
        curr = curr->next;
    }
}

/*Looks up the entry in all the Ale's CBFs and returns the bucket index where
 * the entry was found. It also the entry in the CBF
 *Returns -2 if not found or an index > 0 if found.
 * */
int lookup_and_remove_cbf_list_entry(CBFList* cbfl, Entry e) {
    ListNode* curr = cbfl->head->next;
    u_int found = 0, index = -1;
    while (curr != cbfl->tail) {
        u_int del_entry = 1;
        found = lookup_and_remove_cbf_entry(&curr->cbf, e, del_entry);
        if (found == 1)
            return index;
        index++;
        curr = curr->next;
    }
    return -2;
}

void add_cbf_list_entry(CBFList* cbfl, Entry entry) {
    ListNode* first = cbfl->head->next;
    add_cbf_entry(&first->cbf, entry);
}

void list_combine_bucket(CBFList* cbfl, u_int index) { // add bloom filter contents of index into index + 1
    //TODO
    assert(0);
}

