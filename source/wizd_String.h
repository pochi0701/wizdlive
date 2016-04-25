#ifndef WIZDSTRINGH
#define WIZDSTRINGH
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>
#ifdef linux
#define SOCKET_ERROR (-1)
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <netdb.h>
#include <string>
#else
#include <sys/stat.h>
#include <process.h>
#include <dir.h>
#include <direct.h>
#include <io.h>
#endif
#include "const.h"
#include "wizd.h"

#define MAXSAC 100
#define HTTP_STR_BUF_SIZE (1024*2)

class wString
{
    public:
    wString(void);
    ~wString(void);
    wString(const char* str);
    wString(const wString& str);
    static void     copy(wString* src,const wString* dst);
    static void     copy(wString* src,const wString& dst);
    //Initialze SAC
    static int      wStringInit(void);
    static void     wStringEnd(void);
    static wString* NextSac(void);
    void   myrealloc(const int newSize);
    //OPERATION OVERLOAD
    wString& operator+(const wString& str) const;
    wString& operator+(const char* str) const;
    friend wString& operator+(const char* str1, wString str2 );
    void     operator=(const wString& str);
    void     operator=(const char* str);
    bool     operator==(wString& str) const;
    bool     operator==(const char* str) const;
    bool     operator!=(wString& str) const;
    bool     operator!=(const char* str) const;
    bool     operator>=(wString& str) const;
    bool     operator>=(const char* str) const;
    bool     operator<=(wString& str) const;
    bool     operator<=(const char* str) const;
    bool     operator>(wString& str) const;
    bool     operator>(const char* str) const;
    bool     operator<(wString& str) const;
    bool     operator<(const char* str) const;
    void     operator+=(const char ch);
    void     operator+=(const wString& str);
    void     operator+=(const char* str);
    char     operator[](unsigned int index) const;
    //STRING FUNCTION
    char            at(unsigned int index) const;
    wString&        SubString(int start, int len) const;
    wString&        substr(int index, int length) const;
    wString&        substr(int index) const;
    int             compare(const wString& str) const;
    int             compare(const char* str ) const;
    void            clear(void);
    int             Pos(const char* pattern);
    int             Pos(wString& pattern);
    int             Pos(const char* pattern,int pos);
    int             Pos(wString& pattern,int pos);
    size_t          copy( char *str, size_t slen, size_t index ) const;
    wString&        replace( size_t index, size_t len, const wString& repstr);

    size_t          size(void) const;
    size_t          length(void) const;
    int             Length(void) const ;
    int             Total(void) const;
    char*           c_str(void) const;
    wString&        SetLength(const unsigned int num);

    wString&        Trim(void);
    wString&        RTrim(void);
    wString&        LTrim(void);
    int             sprintf(const char* format, ... );
    int             cat_sprintf(const char* format, ... );
    int             LastDelimiter(const char* delim) const;
    wString&        strsplit(const char* delimstr);
    size_t          find( const wString& str, size_t index=0) const;
    size_t          find( const char* str, size_t index=0 ) const;
    size_t          find( char ch, size_t index = 0) const;
    //URL and encode
    static char*    LinuxFileName(char* FileName);
    static char*    WindowsFileName(char* FileName);
    char*           uri_encode(void);
    char*           uri_decode();
    char*           htmlspecialchars(void);
    wString&        addSlashes(void);
    //ANISSTRING
    bool            SetListString(wString& src,int pos);
    bool            SetListString(const char* src,int pos);
    wString&        GetListString(int pos);
    int             CalcCount(void);
    int             Count(void);
    void            ResetLength(unsigned int num);
    void            Add(wString& str);
    void            Add(const char* str);
    //HEADER
    void            headerInit(size_t content_length, int expire=1, const char* mime_type="text/html");
    void            headerPrint(int socket,int endflag);
    wString&        headerPrintMem(void);
    int             header(const char* str,int flag=true, int status=0);
    //FILE OPERATION
    static bool     RenameFile(wString& src, wString& dst);
    static int      FileCopy(const char* fname_r, const char* fname_w);
    static int      DeleteFile(const wString& str);
    static int      DirectoryExists(char* str);
    static int      DirectoryExists(wString& str);
    static wString& FileStats(const char* str, int mode = 0 );
    static wString& FileStats(wString& str, int mode = 0 );
    static int      FileExists(char* str);
    static int      FileExists(wString& str);
    static wString& ExtractFileDir(wString& str);
    static wString& ExtractFileName(const char* str, const char* delim = DELIMITER);
    static wString& ExtractFileName(const wString& str, const char* delim = DELIMITER);
    static wString& ExtractFileExt(const wString& str );
    static int      CreateDir(wString& str);
    static wString& ChangeFileExt(wString& str, const char* ext);
    static unsigned long FileSizeByName(char* str);
    static unsigned long FileSizeByName(wString& str);
    static wString& EnumFolder(wString& Path);
    static wString& EnumFolderjson(wString& Path);
    static bool     checkUrl( wString& url );
    int             LoadFromFile(const char* FileName);
    void            LoadFromFile(wString& str);
    void            LoadFromCSV(const char* FileName);
    void            LoadFromCSV(wString& str);
    int             SaveToFile(const char* FileName);
    int             SaveToFile(wString& str);
    wString& nkfcnv(const wString& option);
    //HTTP接続用
    static int      HTTPSize(wString& url);
    static SOCKET   sock_connect(wString& host, int port);
    static SOCKET   sock_connect(char *host, int port);
    static wString& HTTPGet(wString& url, off_t offset);
    static wString& HTTPGet(char* url, off_t offset);
    static char*    GetLocalAddress(void);
    //AVI
    static wString& mp3_id3_tag(const char* filename);
    static wString& mp3_id3_tag(wString& filename);
    static size_t npos;
private:
    unsigned int len;	//実際の長さ
    unsigned int total;	//保存領域の長さ
    int count;
    char* String;
    void replace_character_len(const char *sentence,int slen,const char* p,int klen,const char *rep);
    void replace_str(const char *sentence,int slen,const char* p,int klen,const char *rep);
    #ifndef va_copy
    int tsprintf_string(char* str);
    int tsprintf_char(int ch);
    int tsprintf_decimal(signed long val,int zerof,int width);
    int tsprintf_decimalu(unsigned long val,int zerof,int width);
    int tsprintf_octadecimal(unsigned long val,int zerof,int width);
    int tsprintf_hexadecimal(unsigned long val,int capital,int zerof,int width);
    int vtsprintf(const char* fmt,va_list arg);
    #endif
    static wString* sac[MAXSAC];
    static int sacPtr;
    static int init;
    static size_t id3v2_len(unsigned char *buf);
    static int    mp3_id3v1_tag_read(unsigned char *mp3_filename );
    static int    mp3_id3v2_tag_read(unsigned char *mp3_filename );
    static int    mp3_id3_tag_read(unsigned char *mp3_filename );
    static void   rtrim(char *sentence, char cut_char);
    static void convert_language_code(const unsigned char *in, size_t len, int in_flag, int out_flag);
    static unsigned char   mp3_id3v1_flag;         // MP3 タグ 存在フラグ
    static unsigned char   mp3_id3v1_title[128];   // MP3 曲名
    static unsigned char   mp3_id3v1_album[128];   // MP3 アルバム名
    static unsigned char   mp3_id3v1_artist[128];  // MP3 アーティスト
    static unsigned char   mp3_id3v1_year[128];    // MP3 制作年度
    static unsigned char   mp3_id3v1_comment[128]; // MP3 コメント
};
#endif
