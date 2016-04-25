#ifndef CONSTH
#define CONSTH
//カレントバージョン


#define BIRDLAND
/////////////////////////////////////////////////////
#ifdef BIRDLAND
#define WIZDROOT "birdland.co.jp"
#define WIZDROOT2 "www.birdland.co.jp"
#define DOWNLOADFOLDER "/"
#define ROOTPAGE "/index.html"
#define WIZDLIVEUSER     "~wizdlive"
#endif

#define MYSERVERLIST "config.dnl"
// 2004/07/12 Add start
#ifdef linux
#define DELIMITER "/"
#else
#define DELIMITER "/"//"\\"
#endif

#define HTTP_DELIMITER "\r\n\r\n"
#define USERAGENT "Birdland/wizdLiveClF0V1.00"
#define HTTP_SERVER_PORT   (80)
#define FILE_PATH "/"WIZDLIVEUSER"/server.txt"
#define MACADDR  (MAC_ADDR)

#ifdef MAINVAR
char CURRENT_VERSION[] = "VER080215AA";
char MAC_ADDR[256]={0};
#else
extern const char JigsawFile[];
extern char MAC_ADDR[256];
extern char CURRENT_VERSION[];
#endif
#endif