#include <assert.h>

#define LSG_BITS(B) (((1U << 5U) - 1U) & B)
#define MSG_BITS(B) (B >> 4)
#define CBF_HASH_FUNCTIONS_COUNT 4

//TODO fill below
#define INC_LSB(B) (B)
#define INC_MSB(B) (B)
#define DCR_LSB(B) (B)
#define DCR_MSB(B) (B)

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
    u_int hash_count;
} CBF;

void reset_cbf(CBF* cbf){
    memset(cbf->array,0, cbf->C << 1);
}

void create_cbf(CBF* cbf, u_int C) {
    cbf = (CBF*) malloc(sizeof(CBF));
    cbf->hash_count = CBF_HASH_FUNCTIONS_COUNT;
    cbf->C = (C >> 1) << 1;
    printf ("C: %u, C <<1:%u", cbf->C, cbf->C << 1);
    cbf->array = (uint8_t*) calloc(cbf->C << 1, sizeof(uint8_t)); // each 8 byte entry has two counters
    reset_cbf(cbf);
}

void cleanup_cbf(CBF* cbf) {
    free(cbf->array) ;
    free(cbf);
}

u_int get_hash_index(CBF* cbf, Entry entry, u_int hash_function_index) {
    //TODO
    return 0;
}

void add_cbf_entry(CBF* cbf, Entry entry){
    int i;
    for (i = 0; i< cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i);    
        assert (h >= 0 && h < cbf->C);
        if (h & 1U)
            cbf->array[h] = INC_MSB(cbf->array[h]);
        else
            cbf->array[h] = INC_LSB(cbf->array[h]);
    }
}

void remove_cbf_entry(CBF* cbf, Entry entry){
    int i;
    for (i = 0; i< cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i);    
        assert (h >= 0 && h < cbf->C);
        if (h & 1U)
            cbf->array[h] = DCR_MSB(cbf->array[h]);
        else
            cbf->array[h] = DCR_LSB(cbf->array[h]);
    }
}

u_int lookup_and_remove_cbf_entry(CBF* cbf, Entry entry){
    int i;
    u_int found = 1;
    for (i = 0; i< cbf->hash_count ; i++){
        u_int h = get_hash_index(cbf, entry, i);    
        assert (h >= 0 && h < cbf->C);
        if (h & 1U)
            found = found & MSG_BITS(cbf->array[h]);
        else
            found = found & LSG_BITS(cbf->array[h]);
    }
    if (found != 0) 
        remove_cbf_entry(cbf, entry);
    return found;
}

//(struct CBF*) malloc(sizeof(CBF)*blist->len);
