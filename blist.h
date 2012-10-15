#include "cbf.h"

typedef struct _BucketList {
    unsigned int window_count;
    float W; //(in seconds)
    CBF* head;

} BucketList;

typedef struct _ReturnData {
    float rtt; //(in milli seconds)
    unsigned int valid;
} ReturnData;


