typedef uint32_t Entry ;

typedef struct _CBF{
    u_int C; //counter Size
    uint8_t* array;
} CBF;

void reset_cbf(CBF* cbf){

}

void create_cbf(CBF* cbf, u_int C) {
    cbf = (CBF*) malloc(sizeof(CBF));
    cbf->C = C;
    cbf->array = (uint8_t*) calloc(C, sizeof(uint8_t));
    reset_cbf(cbf);
}

void cleanup_cbf(CBF* cbf) {
    free(cbf->array) ;
    free(cbf);
}

void add_cbf_entry(CBF* cbf, Entry e){

}

int lookup_and_remove_cbf_entry(CBF* cbf, Entry e){
    return 0;
}

void remove_cbf_entry(CBF* cbf, Entry e){

}


//(struct CBF*) malloc(sizeof(CBF)*blist->len);
