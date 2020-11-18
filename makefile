FLAGS			= -Wall
LDFLAGS			= -lpthread -D_REENTRANT
DEBUGFLGS		= -g -o
CC				= cc
PROG_SERVER		= servidor 
OBJS_SERVER 	= externas.h servidor.c 

all: ${PROG_SERVER}
 
clean:
		rm -f ${PROG_SERVER}
 
${PROG_SERVER}: ${OBJS_SERVER}
		${CC}  ${FLAGS} ${DEBUGFLGS} ${OBJS_SERVER} -o $@ ${LDFLAGS}
 
.c.o:
		${CC} ${DEBUGFLGS} ${FLAGS} $< -c
