typedef uint64_t Entry ;

typedef struct _CBF{
    unsigned int C; //counter Size
    uint8_t* array;
} CBF;

void reset_cbf(CBF* cbf){

}

void create_cbf(CBF* cbf, unsigned int C) {
    cbf->C = C;
    cbf->array = (uint8_t*) malloc(sizeof(uint8_t)*C);
    reset_cbf(cbf);
}


void cleanup_cbf(CBF* cbf, unsigned int C) {
    free(cbf->array) ;
}

void add_entry(CBF* cbf, Entry e){

}

int lookup_and_remove_entry(CBF* cbf, Entry e){
    return 0;
}

void remove_entry(CBF* cbf, Entry e){

}


//(struct CBF*) malloc(sizeof(CBF)*blist->len);
