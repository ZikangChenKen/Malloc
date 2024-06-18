CC      = cc
CFLAGS  = -std=gnu11 -Wall -Wextra -Werror -g -O2
LDLIBS  = -lm

OBJS    = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

mdriver: ${OBJS}
	${CC} ${CFLAGS} -o mdriver ${OBJS} ${LDLIBS}

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

clean:
	${RM} *.o mdriver core.[1-9]*

.PHONY: clean
