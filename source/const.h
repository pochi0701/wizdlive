#ifndef CONSTH
#define CONSTH
//カレントバージョン

#define makeit

#ifdef linux
#define DELIMITER "/"
#else
#define DELIMITER "\\"
#endif

#ifndef MYFILENAME_MAX
#define MYFILENAME_MAX 1024
#endif

#define HTTP_DELIMITER "\r\n\r\n"
#define USERAGENT "Make-it/wizdLiveClF0V1.00"
#define HTTP_SERVER_PORT   (80)
#define FILE_PATH "/"WIZDLIVEUSER"/server.txt"
#define CURRENT_VERSION "VER061228AA"
#endif
