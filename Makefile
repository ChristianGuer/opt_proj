INCLUDES        = -I. -I/usr/include

LIBS		= libsocklib.a -ldl -lpthread -lm

COMPILE_FLAGS   = ${INCLUDES} -c
COMPILE         = gcc ${COMPILE_FLAGS}
LINK            = gcc -o

C_SRCS		= \
		passivesock.c \
		connectsock.c \
		qclient.c \
		qfuncs.c \
		qserver.c

SOURCE          = ${C_SRCS}

OBJS            = ${SOURCE:.c=.o}

EXEC		= qclient qserver

.SUFFIXES       :       .o .c .h

all		:	library qclient qserver

.c.o            :	${SOURCE}
			@echo "    Compiling $< . . .  "
			@${COMPILE} $<

library		:	passivesock.o connectsock.o
			ar rv libsocklib.a passivesock.o connectsock.o

qserver		:	qserver.o qfuncs.o
			${LINK} $@ qserver.o qfuncs.o ${LIBS}

qclient		:	qclient.o
			${LINK} $@ qclient.o ${LIBS}

clean           :
			@echo "    Cleaning ..."
			rm -f tags core *.out *.o *.lis *.a ${EXEC} libsocklib.a
