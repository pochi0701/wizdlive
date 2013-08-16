#ifndef CONSTH
#define CONSTH
//カレントバージョン

#define makeit
//#define makeit
//#define BANCTEC
//#define TEST

/////////////////////////////////////////////////////
#ifdef SGSGI
//#define WIZDROOT "make-it.co.jp"
//#define WIZDROOT2 "www.make-it.co.jp"
#define WIZDROOT "httptrans.jp"
#define WIZDROOT2 "www.httptrans.jp"
#define DOWNLOADFOLDER "domestic/"
#define ROOTPAGE "/domestic/page1.html"
#define WIZDLIVEUSER     "~wizdlive"
//#define WIZDROOT "221.245.230.123"
//#define WIZDROOT2 "221.245.230.123"
//#define DOWNLOADFOLDER "international/"
//#define ROOTPAGE "/international/page1.html"
//#define WIZDROOT "221.245.230.122"
//#define WIZDROOT2 "221.245.230.122"
//#define DOWNLOADFOLDER "test2/"
//#define ROOTPAGE "/test2/page1.html"
#endif
/////////////////////////////////////////////////////

//#define WIZDROOT "221.245.230.122"
//#define WIZDROOT2 "221.245.230.122"
//#define DOWNLOADFOLDER "domestic/"
//#define ROOTPAGE "/domestic/page1.html"
//#define WIZDLIVEUSER "~wizdlive"

//メイキット
#ifdef makeit
#define WIZDROOT "make-it.co.jp"
#define WIZDROOT2 "www.make-it.co.jp"
#define DOWNLOADFOLDER "/"
#define ROOTPAGE "/index.html"
#define WIZDLIVEUSER     "~wizdlive"
#endif

//BANCTEC
#ifdef BANCTEC
#define WIZDROOT "221.245.230.123"
#define WIZDROOT2 "221.245.230.123"
#define DOWNLOADFOLDER "contents/"
#define ROOTPAGE "/contents/page1.html"
#define WIZDLIVEUSER     "~banctec"
#endif

//TEST SGI WIZDLIVE
#ifdef TEST
#define WIZDROOT "221.245.230.123"
#define WIZDROOT2 "221.245.230.123"
#define DOWNLOADFOLDER "contents/"
#define ROOTPAGE "/contents/page1.html"
#define WIZDLIVEUSER     "~test"
#endif

//#define WIZDROOT "make-it.co.jp"        //ホスト名
//#define WIZDROOT2 "www.make-it.co.jp"   //ダウンロードターゲット
//#define DOWNLOADFOLDER "/"   //ダウンロードアドレス
//#define ROOTPAGE "/page1.html"
//#define WIZDLIVEUSER     "~work"


///#define WIZDROOT "birdland.co.jp"        //ホスト名
///#define WIZDROOT2 "www.birdland.co.jp"   //ダウンロードターゲット
///#define DOWNLOADFOLDER "contents/"   //ダウンロードアドレス
///#define ROOTPAGE "contents/page1.html"
///#define WIZDLIVEUSER     "~hoge"


#define MYSERVERLIST "/config.dnl"
// 2004/07/12 Add start
#ifdef linux
#define DELIMITER "/"
#else
#define DELIMITER "\\"
#ifndef MYFILENAME_MAX
#define MYFILENAME_MAX 1024
#endif
#endif

#define HTTP_DELIMITER "\r\n\r\n"
#define USERAGENT "Make-it/wizdLiveClF0V1.00"
#define HTTP_SERVER_PORT   (80)
#define FILE_PATH "/"WIZDLIVEUSER"/server.txt"
#define CURRENT_VERSION "VER061228AA"

#ifdef MAINVAR
const static char TargetVer[] = "http://"WIZDROOT2"/wizdlive.txt";
const static char TargetFile[] = "http://"WIZDROOT2"/wizdlive";
const static char SumFile[] = "http://"WIZDROOT2"/"WIZDLIVEUSER"/"DOWNLOADFOLDER"sum.txt";
const static char JigsawFile[]
                      = "http://"WIZDROOT2"/"WIZDLIVEUSER"/"DOWNLOADFOLDER"jigsaw.txt";
const static char JigsawFile2[] 
                      = "http://"WIZDROOT2"/"WIZDLIVEUSER"/"DOWNLOADFOLDER"rjigsaw.txt";
#else
//extern const char TargetVer[];
//extern const char TargetFile[];
extern const char JigsawFile[];
#endif
#endif
#define ROOTDIR "/opt/sybhttpd/localhost.localdrive/HARDDISK/"
#define WORKDIR ROOTDIR"work/"
#define SKINDIR ROOTDIR"skin/"
