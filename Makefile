#
#       $Id: onvlibrary,v 0.1 2012/09/05 09:00:00 goldman2 Exp $
#       #       $Revision: 0.1 $
#
##       ONV Libray 
#
#
CC = gcc 
CMD_AR = ar -cru
CMD_RANLIB =  ranlib
#ONVLIB_OBJS =  config_parser.o  log.o  misclib.o onvmysql.o onvsock.o
ONVLIB_OBJS =  config_parser.o  log.o  misclib.o 
#LIB=  -L/usr/lib64/mysql -lmysqlclient_r -lm -lz -lpthread
#CFLAGS = -g  -I/usr/include/mysql  -DENABLE_DEBUG
CFLAGS = -g  -DENABLE_DEBUG
TARGET_LIB =  libonv.a

#SRCS = $(OBJS:.o=.c)
all: onvlib

#onvlib: config_parser log misclib onvsock onvmysql
onvlib: config_parser log misclib
	        rm -f *.core
		$(CMD_AR) $(TARGET_LIB) $(ONVLIB_OBJS)
		$(CMD_RANLIB) $(TARGET_LIB) 
config_parser: log config_parser.h config_parser.c
	$(CC) -c $(CFLAGS) $(LIB) config_parser.c
log: log.h log.c
	$(CC) -c $(CFLAGS) $(LIB) log.c
misclib: misclib.h misclib.c
	$(CC) -c $(CFLAGS) $(LIB) misclib.c

#onvsock: onvsock.h onvsock.c
#	$(CC) -c $(CFLAGS) $(LIB) onvsock.c

#onvmysql: log onvmysql.c onvmysql.h
#	$(CC) -c $(CFLAGS) $(LIB) onvmysql.c

clean:
	        rm -f *.bak
		rm -f *.map
		rm -f *.o
		rm -f *.a
#		@echo "파일을 삭제했습니다."

