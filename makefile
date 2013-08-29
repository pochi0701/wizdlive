################################################################################
# wizd makefile
#						$Revision: 1.7 $
#						$Date: 2004/07/03 15:09:32 $
################################################################################
all:	wizd

wizd:	makefile		\
	source/wizd_main.c	\
	source/wizd_param.c	\
	source/wizd.h 		\
	source/wizd_listen.c	\
	source/wizd_http.c	\
	source/wizd_menu.c	\
	source/wizd_send_file.c	\
	source/wizd_send_svi.c	\
	source/wizd_send_vob.c	\
	source/wizd_proxy.c	\
	source/wizd_base64.c	\
	source/wizd_cgi.c	\
	source/wizd_aviread.c	\
	source/wizd_detect.c	\
	source/wizd_tools.h	\
	source/wizd_tools.c	\
	source/config.h		\
	source/libnkf.c		\
	source/utf8tbl.c
	gcc 	-O2 -Wall   		\
		-D_FILE_OFFSET_BITS=64 	\
		-DUTF8_INPUT_ENABLE 	\
		-D_LARGEFILE_SOURCE 	\
		-o wizd  		\
		source/wizd_main.c  	\
		source/wizd_param.c	\
		source/wizd_listen.c	\
		source/wizd_http.c	\
		source/wizd_menu.c	\
		source/wizd_send_file.c	\
		source/wizd_send_svi.c	\
		source/wizd_send_vob.c	\
		source/wizd_proxy.c	\
		source/wizd_base64.c	\
		source/wizd_cgi.c	\
		source/wizd_aviread.c	\
		source/wizd_detect.c	\
		source/wizd_tools.c	\
		source/libnkf.c		\
		source/utf8tbl.c
	strip 	wizd

wizd.exe:	makefile		\
	source/wizd_main.c	\
	source/wizd_param.c	\
	source/wizd.h 		\
	source/wizd_listen.c	\
	source/wizd_http.c	\
	source/wizd_menu.c	\
	source/wizd_send_file.c	\
	source/wizd_send_svi.c	\
	source/wizd_send_vob.c	\
	source/wizd_proxy.c	\
	source/wizd_base64.c	\
	source/wizd_cgi.c	\
	source/wizd_aviread.c	\
	source/wizd_detect.c	\
	source/wizd_tools.h	\
	source/wizd_tools.c	\
	source/config.h		\
	source/libnkf.c		\
	source/utf8tbl.c
	i386-cygwin32-gcc 	-O2 -Wall   		\
		-D_FILE_OFFSET_BITS=64 	\
		-DUTF8_INPUT_ENABLE 	\
		-D_LARGEFILE_SOURCE 	\
		-o wizd.exe  		\
		source/wizd_main.c  	\
		source/wizd_param.c	\
		source/wizd_listen.c	\
		source/wizd_http.c	\
		source/wizd_menu.c	\
		source/wizd_send_file.c	\
		source/wizd_send_svi.c	\
		source/wizd_send_vob.c	\
		source/wizd_proxy.c	\
		source/wizd_base64.c	\
		source/wizd_cgi.c	\
		source/wizd_aviread.c	\
		source/wizd_detect.c	\
		source/wizd_tools.c	\
		source/libnkf.c		\
		source/utf8tbl.c
	i386-cygwin32-strip 	wizd.exe


noutf8:
	gcc 	-O2 -Wall   		\
		-D_FILE_OFFSET_BITS=64 	\
		-D_LARGEFILE_SOURCE 	\
		-o wizd  		\
		source/wizd_main.c  	\
		source/wizd_param.c	\
		source/wizd_listen.c	\
		source/wizd_http.c	\
		source/wizd_menu.c	\
		source/wizd_send_file.c	\
		source/wizd_send_svi.c	\
		source/wizd_send_vob.c	\
		source/wizd_proxy.c	\
		source/wizd_base64.c	\
		source/wizd_detect.c	\
		source/wizd_tools.c	\
		source/libnkf.c
	strip 	wizd


clean:
	rm -f wizd
