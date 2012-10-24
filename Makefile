CC = gcc
CFLAGS = -O3 -Wall -g -D_BSD_SOURCE
PROGNAME = ale
TESTNAME = testale
SRCS = tcpdump.c main.c readtraces.c
INCS =   main.h ale.h cbfList.h cbf.h pktHeaders.h readtraces.h tcpdump.h hashfunctions.h

OBJS = ${SRCS:.c=.o}
all: $(OBJS) test

	gcc $(CFLAGS) -lm -lpcap -o $(PROGNAME) $(OBJS)
test: test.c
	gcc $(CFLAGS) -o $(TESTNAME) test.c

#readtraces.o ::
#	gcc $(CFLAGS) -c -o readtraces.o readtraces.c


$(OBJS):: $(INCS)

clean:
	- rm $(OBJS) *.core $(TESTNAME) $(PROGNAME) *~ *#* 

print:
	a2ps $(SRCS) $(INCS) -o tmp.ps
	pdq tmp.ps
	rm tmp.ps
