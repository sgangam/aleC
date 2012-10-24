#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <error.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "ale.h"
#include "main.h"


void test_cbf() {
    CBF cbf;
    create_cbf(&cbf, 120);
    //Add numbers 0 to 19
    u_int i = 0;
    for (i = 0 ; i < 20 ; i++)
        add_cbf_entry(&cbf, i);
    
    // Check if they belong in the set
    u_int allFound = 1, found = 0;
    for (i = 0 ; i < 20 ; i++) {
        u_int del_entry = 0;
        found = lookup_and_remove_cbf_entry(&cbf, i, del_entry);
        allFound &= found;
    }
    assert(allFound);

    // Remove odd numbers
    allFound = 1, found = 0;
    for (i = 1 ; i < 20 ; i++) {
        u_int del_entry = 1;
        found = lookup_and_remove_cbf_entry(&cbf, i, del_entry);
        allFound &= found;
        i++;
    }
    assert(allFound);

    // Check if odd numbers are missing
    u_int allNotFound = 1, notFound = 0;
    for (i = 1 ; i < 20 ; i++) {
        u_int del_entry = 0;
        notFound = !(lookup_and_remove_cbf_entry(&cbf, i, del_entry));
        allNotFound &= notFound;
        i++;
    }
    assert(allNotFound);

    // Check if even numbers are still present
    allFound = 1, found = 0;
    for (i = 0 ; i < 20 ; i++) {
        u_int del_entry = 0;
        found = lookup_and_remove_cbf_entry(&cbf, i, del_entry);
        allFound &= found;
        i++;
    }
    assert(allFound);

    // Check if other numbers 20 to 39 are present
    allNotFound = 1, notFound = 0;
    for (i = 20 ; i < 40 ; i++) {
        u_int del_entry = 0;
        notFound = !(lookup_and_remove_cbf_entry(&cbf, i, del_entry));
        allNotFound &= notFound;
    }
    assert(allNotFound);
    cleanup_cbf(&cbf);
}

int
main(int argc, char *argv[])
{
    test_cbf();
    return 0;
}
