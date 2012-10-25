#include <assert.h>
#include "hashfunctions.h"

#define CBF_HASH_FUNCTIONS_COUNT 5
#define PRIMES_PER_HASH 4
#define PRIME_SIZE 30

#define LSG_BITS(B) (B & 0xf)
#define MSG_BITS(B) (B >> 4)

#define INC_LSB(B) (  (B & 0xf0) | (((B & 0x0f) + 1) & 0x0f) )
#define DCR_LSB(B) (  (B & 0xf0) | (((B & 0x0f) - 1) & 0x0f) )

#define INC_MSB(B) (  (((B & 0xf0) + 0x10) & 0xf0) | (B & 0x0f)  )
#define DCR_MSB(B) (  (((B & 0xf0) - 0x10) & 0xf0) | (B & 0x0f)  )

u_int prime_array [PRIME_SIZE] = {4151325961, 3533412479, 2918217551, 4279875371, 3705079459, 2728895899, 1017668207, 413158523, 1436135293, 520951843,
                        71480063, 3482876333, 4040666039, 3996428773, 4025176643, 4053938279, 4016331647, 4242190873, 4257712259, 2544507943, 
                        2285248019, 2311109513, 1948154267, 181328093, 1488884053, 1637064479, 1788027757, 2380165901, 2622509909, 2707161473 };
/*
 4 bit counters with indices on a 8 bit array.
  MSB LSB
0: 1   0
1: 3   2
2: 5   4
3: 7   6 
4: 9   8
 */
typedef uint32_t Entry ;

typedef struct _CBF{
    u_int C; //counter Size, number of 4 bit counters.
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
    //reset_cbf(cbf); //calloc initializes the counters to zero.
}

void print_cbf_array(CBF* cbf) {
    u_int i;
    for (i = 0;i < cbf->array_count; i++)
        printf("%x,",cbf->array[i]);
}

void cleanup_cbf(CBF* cbf) {
    free(cbf->array) ;
}

u_int get_hash_index(CBF* cbf, Entry entry, u_int hash_function_index) {
    assert(PRIME_SIZE >= hash_function_index*PRIMES_PER_HASH);
    u_int array[PRIMES_PER_HASH + 1];
    int i = 0;
    for (i = 0; i < PRIMES_PER_HASH; i++) {
        array[i] = prime_array[hash_function_index*PRIMES_PER_HASH + i];
    } 
    array[PRIMES_PER_HASH] = entry; 
    u_int hval = hashword(array, PRIMES_PER_HASH + 1, 0xaeeaaeea);
    return (hval % cbf->C);
}

void add_cbf_entry(CBF* cbf, Entry entry){
    int i;
    for (i = 0; i < cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i); 
        u_int ai = h >> 1; //divide by 2.
        assert (h >= 0 && h < cbf->C);
        if ((h & 1U) && MSG_BITS(cbf->array[ai]) != 0xf )
            cbf->array[ai] = INC_MSB(cbf->array[ai]);
        else if (LSG_BITS(cbf->array[ai]) != 0xf)
            cbf->array[ai] = INC_LSB(cbf->array[ai]);
        else
            fprintf(stderr,"Insufficient counters\n");

    }
}

u_int lookup_and_remove_cbf_entry(CBF* cbf, Entry entry, u_int del_entry){
    int i;
    u_int hash_indices[cbf->hash_count];
    for (i = 0; i < cbf->hash_count ; i++) {
        u_int h = get_hash_index(cbf, entry, i);    
        assert (h >= 0 && h < cbf->C);
        hash_indices[i] = h;
        u_int ai = h >> 1;
        if ((h & 1U)  && MSG_BITS(cbf->array[ai]) == 0)
                return 0; //not found
        else if (!(h & 1U)  && LSG_BITS(cbf->array[ai]) == 0)
            return 0;// not found
    }

    if (del_entry == 1) {
        for (i = 0; i < cbf->hash_count ; i++) {
            u_int h = hash_indices[i];
            u_int ai = h >> 1;
            //printf ( "h: %u, h&1U:%u  -- %u:%u\n",h, (h & 1U), MSG_BITS(cbf->array[ai]), LSG_BITS(cbf->array[ai]) );
            if ((h & 1U) && MSG_BITS(cbf->array[ai]) != 0)
                cbf->array[ai] = DCR_MSB(cbf->array[ai]); // h is Odd : most significant nibble
            else if (!(h & 1U) && LSG_BITS(cbf->array[ai]) != 0)
                cbf->array[ai] = DCR_LSB(cbf->array[ai]) ;// h is Even : least significant nibble
            //else    assert(0); // Since we have poor hash functions, this assertion fails. TODO: we need better hash functions which are faster.
        }
    }
    return 1; //found
}

void combine_cbf(CBF* cbf, CBF* cbf_next) {// append contents fo cbf to cbf_next
    assert(cbf->array_count == cbf_next->array_count);
    int i = 0;
    for (i = 0; i < cbf->array_count; i++) {
        u_int msb = LSG_BITS(cbf->array[i]) + LSG_BITS(cbf_next->array[i]);
        u_int lsb = MSG_BITS(cbf->array[i]) + MSG_BITS(cbf_next->array[i]);
        if (msb > 0xff) msb = 0xff; if (lsb > 0xff) lsb = 0xff;
        cbf_next->array[i] = (msb << 4) | lsb;
    }
}
