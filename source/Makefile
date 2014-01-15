################################################################################
# wizd makefile
#						$Revision: 1.6 $
#						$Date: 2004/02/25 19:16:51 $
################################################################################
#CROSS=mipsel-linux-uclibc-
GCC=$(CROSS)gcc
GXX=$(CROSS)g++
CC=$(CROSS)gcc
AS=$(CROSS)as
AR=$(CROSS)ar
STRIP=$(CROSS)strip
LD=$(CROSS)ld
RANLIB=$(CROSS)ranlib
#CFLAGS=-O3 -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Dlinux -D_DEBUG -I./
CFLAGS= -g -Wall -Dlinux -D_DEBUG
#CFLAGS= -pg -Wall -Dlinux -D_DEBUG
#CFLAGS= -O3 -Wall -Dlinux
#CFLAGS+=-mips2
#LDFLAGS=-L/usr/local/mipsel-linux-uclibc/lib -L/usr/local/mipsel-linux-uclibc/mipsel-linux-uclibc/lib
#LIBS=-lpthread -lgcc
#LIBS=-lpthread 

all:	wizdlive

wizdlive:	Makefile		\
	wizd_main.cpp	\
	wizd_param.cpp	\
	wizd.h 		\
	wizd_listen.cpp	\
	wizd_http.cpp	\
	wizd_menu.cpp	\
	wizd_send_file.cpp	\
	wizd_send_vob.cpp	\
	wizd_proxy.cpp	\
	wizd_base64.cpp	\
	wizd_cgi.cpp	\
	wizd_tools.h	\
	wizd_tools.cpp	\
        wizd_detect.cpp \
        wizd_aviread.cpp\
	wizd_String.cpp	\
	TinyJS.cpp	\
	TinyJS_Functions.cpp	\
	TinyJS_MathFunctions.cpp	\
	config.h	\
        utf8tbl.cpp	\
	libnkf.cpp
	$(GXX) $(CFLAGS) $(INCS) $(LDFLAGS) $(LIBS) \
		-o wizdlive  		\
                wizd_main.cpp  	\
		wizd_param.cpp	\
		wizd_listen.cpp	\
		wizd_http.cpp	\
		wizd_menu.cpp	\
		wizd_send_file.cpp	\
		wizd_send_vob.cpp	\
		wizd_proxy.cpp	\
		wizd_base64.cpp	\
		wizd_cgi.cpp	\
		wizd_tools.cpp	\
                wizd_detect.cpp \
                wizd_aviread.cpp\
		wizd_String.cpp	\
		TinyJS.cpp	\
		TinyJS_Functions.cpp	\
		TinyJS_MathFunctions.cpp	\
		utf8tbl.cpp\
		libnkf.cpp
	cp ./wizdlive ../wizdlive


clean:
	rm -f wizdlive
