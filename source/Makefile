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
#CFLAGS= -g -O3 -Wall -Dlinux -pthread
CFLAGS= -g -O3 -Wall -Dlinux -D_DEBUG -pthread
#CFLAGS= -O1 -Wall -Dlinux -pthread
#CFLAGS= -g -Wall -Dlinux -D_DEBUG

all:	wizdlive

wizdlive:	Makefile		\
	wizd_main.cpp	\
	wizd_param.cpp	\
	wizd.h 		\
	wizd_listen.cpp	\
	wizd_http.cpp	\
	wizd_send_file.cpp	\
	wizd_proxy.cpp	\
	wizd_base64.cpp	\
	wizd_cgi.cpp	\
	wizd_tools.h	\
	wizd_tools.cpp	\
        wizd_detect.cpp \
	wizd_String.cpp	\
	TinyJS.cpp	\
	TinyJS_Functions.cpp	\
	TinyJS_MathFunctions.cpp	\
	config.h	\
        utf8tbl.cpp	\
	libnkf.cpp	\
	dregex.cpp
	$(GXX) $(CFLAGS) $(INCS) $(LDFLAGS) $(LIBS) \
		-o wizdlive  		\
                wizd_main.cpp  	\
		wizd_param.cpp	\
		wizd_listen.cpp	\
		wizd_http.cpp	\
		wizd_send_file.cpp	\
		wizd_proxy.cpp	\
		wizd_base64.cpp	\
		wizd_cgi.cpp	\
		wizd_tools.cpp	\
                wizd_detect.cpp \
		wizd_String.cpp	\
		TinyJS.cpp	\
		TinyJS_Functions.cpp	\
		TinyJS_MathFunctions.cpp	\
		utf8tbl.cpp	\
		libnkf.cpp	\
		dregex.cpp
	cp ./wizdlive ../wizdlive
#	strip wizdlive

clean:
	rm -f wizdlive
