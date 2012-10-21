CC = gcc
CFLAGS = -O3 -Wall -g -D_BSD_SOURCE
PROGNAME = ale
SRCS = tcpdump.c main.c readtraces.c
INCS =   main.h ale.h cbfList.h cbf.h pktHeaders.h readtraces.h tcpdump.h hashfunctions.h

OBJS = ${SRCS:.c=.o}

all: $(OBJS)
	gcc $(CFLAGS) -lm -lpcap -o $(PROGNAME) $(OBJS)

#readtraces.o ::
#	gcc $(CFLAGS) -c -o readtraces.o readtraces.c


$(OBJS):: $(INCS)

clean:
	- rm $(OBJS) *.core $(PROGNAME) *~ *#* 

print:
	a2ps $(SRCS) $(INCS) -o tmp.ps
	pdq tmp.ps
	rm tmp.ps
