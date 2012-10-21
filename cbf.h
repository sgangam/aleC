#include <assert.h>
#include "hashfunctions.h"

#define LSG_BITS(B) (B & 0xf)
#define MSG_BITS(B) (B >> 4)
#define CBF_HASH_FUNCTIONS_COUNT 4

//TODO fill below
#define INC_LSB(B) ((B & 0xf0) | ((B & 0xf) + 1))
#define DCR_LSB(B) ((B & 0xf0) | ((B & 0xf) - 1))
#define INC_MSB(B) (((B & 0xf0) + 1) | (B & 0xf))
#define DCR_MSB(B) (((B & 0xf0) - 1) | (B & 0xf))

/*
 4 bit counters with indices on a 8 bit array.
0: 0 1
1: 2 3
2: 4 5
3: 6 7
4: 8 9
 */
typedef uint32_t Entry ;

typedef struct _CBF{
    u_int C; //counter Size
    uint8_t* array;
    u_int array_count;
    u_int hash_count;
} CBF;

void reset_cbf(CBF* cbf){
    memset(cbf->array, 0, cbf->array_count);
}

void create_cbf(CBF* cbf, u_int C) {
    cbf->hash_count = CBF_HASH_FUNCTIONS_COUNT;
    cbf->array_count =  (C >> 1);
    cbf->C = cbf->array_count << 1;
    cbf->array = (uint8_t*) calloc( cbf->array_count, sizeof(uint8_t)); // each 8 byte entry has two counters
    reset_cbf(cbf);
}

void print_array(CBF* cbf) {
    u_int i;
    for (i = 0;i < cbf->array_count; i++)
        printf("%u,",cbf->array[i]);
}

void cleanup_cbf(CBF* cbf) {
    //printf("Freeing cbf array:%x\n",cbf->array);
    //print_array(cbf);
    free(cbf->array) ;
}

u_int get_hash_index(CBF* cbf, Entry entry, u_int hash_function_index) {
    u_int array[5];
    array[0] = entry; array[1] = 0xdeadbeef; array[2] = 0xfeebdaed ; array[3] = entry ^ array[1] ; array[4] = entry ^ array[2];
    //srand(hash_function_index); array[0] = entry ; array[2] = rand() ; array[3] = entry ^ rand() ; array[4] = entry ^ rand();
    u_int hval = hashword(array, 5, 0);
    return (hval % cbf->C);
}

void add_cbf_entry(CBF* cbf, Entry entry){
    int i;
    for (i = 0; i < cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i);    
        u_int ai = h >> 1; //divide by 2.
        assert (h >= 0 && h < cbf->C);
        if (h & 1U)
            cbf->array[ai] = INC_MSB(cbf->array[ai]);
        else
            cbf->array[ai] = INC_LSB(cbf->array[ai]);
    }
}

void remove_cbf_entry(CBF* cbf, Entry entry){
    int i;
    for (i = 0; i< cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i);    
        assert (h >= 0 && h < cbf->C);
        u_int ai = h >> 1;
        if (h & 1U)
            cbf->array[ai] = DCR_MSB(cbf->array[ai]); // h is Odd : most significant nibble
        else
            cbf->array[ai] = DCR_LSB(cbf->array[ai]);// h is Even : least significant nibble
    }
}

u_int lookup_and_remove_cbf_entry(CBF* cbf, Entry entry){
    int i;
    u_int found = 1;
    for (i = 0; i< cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i);    
        assert (h >= 0 && h < cbf->C);
        u_int ai = h >> 1;
        if (h & 1U)
            found = found & MSG_BITS(cbf->array[ai]);
        else
            found = found & LSG_BITS(cbf->array[ai]);
    }
    if (found != 0) 
        remove_cbf_entry(cbf, entry);
    return found;
}

