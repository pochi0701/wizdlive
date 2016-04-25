#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#ifdef linux
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#else
#include <process.h>
#include <dir.h>
#include <direct.h>
#include <io.h>
#endif
#include "wizd_String.h"
#include "wizd.h"
#include "wizd_tools.h"
//---------------------------------------------------------------------------
//source
//---------------------------------------------------------------------------
wString* wString::sac[MAXSAC];
int wString::sacPtr=0;
size_t wString::npos=(size_t)(-1);
//---------------------------------------------------------------------------
int wString::wStringInit(void)
{
    for( int i = 0 ; i < MAXSAC ; i++ ){
        sac[i] = new wString();
        sac[i]->SetLength(512);
    }
    sacPtr = 0;
    return 1;
}
//---------------------------------------------------------------------------
void wString::wStringEnd(void)
{
    for( int i = 0 ; i < MAXSAC ; i++ ){
        delete sac[i];
    }
}
//---------------------------------------------------------------------------
//リングストリングバッファ領域を取得
wString* wString::NextSac(void)
{
    wString* ptr = sac[sacPtr++];
    sacPtr = sacPtr%MAXSAC;
    //新文字列は長さ０
    ptr->len = 0;
    ptr->count = 0;
    if( ptr->String ){
        //memset(ptr->String,0,ptr->total);
        *ptr->String = 0;
    }
    return ptr;
}
//---------------------------------------------------------------------------
//通常コンストラクタ
wString::wString(void)
{
    //初期化
    len = 0;
    total = 1;
    count = 0;
    String = (char*)new char[1];
    *String = 0;
}
//---------------------------------------------------------------------------
//文字列コンストラクタ
wString::wString(const char *str)
{
    //初期化
    count = 0;
    len = strlen(str);
    if( len ){
        total = len+1;
        String = (char*)new char[total];
        strcpy(String, str);
    }else{
        total = 1;
        String = (char*)new char[1];
        String[0] = 0;
    }
}
//---------------------------------------------------------------------------
//コピーコンストラクタ
wString::wString(const wString& str)
{
    //初期化
    len   = str.len;
    count = str.count;
    if( str.len ){
        total = str.total;
        String = new char[str.total];
        memcpy(String,str.String,str.total);
        //*String = 0;
    }else{
        total = 1;
        String = (char*)new char[1];
        String[0] = 0;
    }
}
//---------------------------------------------------------------------------
//デストラクタ
wString::~wString()
{
    delete [] String;
}
//---------------------------------------------------------------------------
//ディープコピーメソッド
void wString::copy(wString* src,const wString* dst)
{
    src->myrealloc(dst->total);
    memcpy( src->String, dst->String, dst->total);
    src->len = dst->len;
    src->count = dst->count;
}
//---------------------------------------------------------------------------
//ディープコピーメソッド
void wString::copy(wString* src,const wString& dst)
{
    src->myrealloc(dst.total);
    memcpy( src->String, dst.String, dst.total);
    src->len = dst.len;
    src->count = dst.count;
}
//---------------------------------------------------------------------------
wString& wString::operator+(const wString& str) const
{
    wString* temp = NextSac();
    copy(temp,this);
    *temp += str;
    return *temp;
}
//---------------------------------------------------------------------------
wString& wString::operator+(const char* str) const
{
    wString* temp = NextSac();
    copy(temp,this);
    *temp +=str;
    return *temp;
}
//---------------------------------------------------------------------------
wString& operator+(const char* str1, wString str2 )
{
    wString* temp = wString::NextSac();
    size_t newLen = strlen(str1)+str2.len;
    temp->myrealloc(newLen);
    strcpy(temp->String, str1);
    strcat(temp->String, str2.String);
    temp->len = newLen;
    return *temp;
}
//---------------------------------------------------------------------------
void wString::operator+=(const wString& str)
{
    count += str.count;
    unsigned int newLen = len+str.len;
    myrealloc(newLen);
    memcpy(String+len,str.String,str.len);
    String[newLen] =0;
    len    = newLen;
    return;
}
//---------------------------------------------------------------------------
void wString::operator+=(const char* str)
{
    unsigned int slen = strlen(str);
    unsigned int newLen = slen+len;
    myrealloc(newLen);
    strcpy( String+len, str );
    len = newLen;
    return;
}
//---------------------------------------------------------------------------
void wString::operator+=(const char ch)
{
    int tmpl=len>>4;
    tmpl <<= 4;
    tmpl += 64;
    myrealloc(tmpl);
    String[len] = ch;
    String[len+1] = 0;
    len+=1;
    return;
}
//---------------------------------------------------------------------------
bool wString::operator==(wString& str) const
{
    if( len  != str.len ){
        return false;
    }
    return( strncmp( String, str.String, len) == 0);
}
//---------------------------------------------------------------------------
bool wString::operator==(const char* str) const
{
    unsigned int mylen = strlen(str);
    if( len != mylen ){
        return false;
    }
    return( strncmp( String, str, len) == 0);
}
//---------------------------------------------------------------------------
bool wString::operator!=(wString& str) const
{
    if( len != str.len ){
        return true;
    }
    return( strncmp( String, str.String, len) != 0);
}
//---------------------------------------------------------------------------
bool wString::operator!=(const char* str) const
{
    unsigned int mylen = strlen(str);
    if( len != mylen ){
        return true;
    }
    return( strncmp( String, str, len) != 0);
}
//---------------------------------------------------------------------------
bool wString::operator>=(wString& str) const
{
    unsigned int maxlen = (len>str.len)?len:str.len;
    return( strncmp( String, str.String, maxlen) >= 0);
}
//---------------------------------------------------------------------------
bool wString::operator>=(const char* str) const
{
    unsigned int mylen = strlen(str);
    unsigned int maxlen = (len>mylen)?len:mylen;
    return( strncmp( String, str, maxlen) >= 0);
}
//---------------------------------------------------------------------------
bool wString::operator<=(wString& str) const
{
    unsigned int maxlen = (len>str.len)?len:str.len;
    return( strncmp( String, str.String, maxlen) <= 0);
}
//---------------------------------------------------------------------------
bool wString::operator<=(const char* str) const
{
    unsigned int mylen = strlen(str);
    unsigned int maxlen = (len>mylen)?len:mylen;
    return( strncmp( String, str, maxlen) <= 0);
}
//---------------------------------------------------------------------------
bool wString::operator>(wString& str) const
{
    unsigned int maxlen = (len>str.len)?len:str.len;
    return( strncmp( String, str.String, maxlen) > 0);
}
//---------------------------------------------------------------------------
bool wString::operator>(const char* str) const
{
    unsigned int mylen = strlen(str);
    unsigned int maxlen = (len>mylen)?len:mylen;
    return( strncmp( String, str, maxlen) > 0);
}
//---------------------------------------------------------------------------
bool wString::operator<(wString& str) const
{
    unsigned int maxlen = (len>str.len)?len:str.len;
    return( strncmp( String, str.String, maxlen) < 0);
}
//---------------------------------------------------------------------------
bool wString::operator<(const char* str) const
{
    unsigned int mylen = strlen(str);
    unsigned int maxlen = (len>mylen)?len:mylen;
    return( strncmp( String, str, maxlen) < 0);
}
//---------------------------------------------------------------------------
void wString::operator=(const wString& str)
{
    count = str.count;
    myrealloc(str.total);
    memcpy( String, str.String,str.total );
    len = str.len;
    return;
}
//---------------------------------------------------------------------------
void wString::operator=(const char* str)
{
    count = 0;
    int newLen = strlen(str);
    myrealloc(newLen);
    strcpy( String, str);
    len = newLen;
    return;
}
//---------------------------------------------------------------------------
char wString::operator[](unsigned int index) const
{
    if( index < len ){
        return String[index];
    }else{
        perror( "out bound");
        return -1;
    }
}
//---------------------------------------------------------------------------
char wString::at(unsigned int index) const
{
    if( index < len ){
        return String[index];
    }else{
        perror( "out bound");
        return -1;
    }
}
//---------------------------------------------------------------------------
wString& wString::SetLength(const unsigned int num)
{
    myrealloc(num);
    return *this;
}
//---------------------------------------------------------------------------
// 比較
//---------------------------------------------------------------------------
int wString::compare(const wString& str) const
{
    return strcmp( String, str.String );
    //size_t minlen = (len>str.len)?str.len+1:len+1;
    //return memcmp( String, str.String, minlen );
}
//---------------------------------------------------------------------------
// 比較
//---------------------------------------------------------------------------
int wString::compare(const char* str ) const
{
    return strcmp(String,str);
}
//---------------------------------------------------------------------------
// クリア
//---------------------------------------------------------------------------
void  wString::clear(void)
{
    len = 0;
    if( String ){
        String[0] = 0;
    }
}
//---------------------------------------------------------------------------
// 部分文字列
//---------------------------------------------------------------------------
wString&  wString::SubString(int start, int mylen) const
{
    wString* temp = NextSac();
    if( mylen>0){
        temp->myrealloc( mylen );
        memcpy( temp->String, String+start,mylen);
        temp->String[mylen] = 0;
        //長さ不定。数えなおす
        temp->len = mylen;//strlen(temp->String);
    }
    return *temp;
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
size_t wString::find( const wString& str, size_t index) const
{
    char* ptr = strstr(String+index, str.String);
    if( ptr == NULL ){
        return npos;
    }else{
        return (size_t)(ptr-String);
    }
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
size_t wString::find( const char* str, size_t index ) const
{
    char* ptr = strstr(String+index, str);
    if( ptr == NULL ){
        return npos;
    }else{
        return (size_t)(ptr-String);
    }
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
size_t wString::find( char ch, size_t index ) const
{
    char str[2];
    str[0] = ch;
    str[1] = 0;
    char* ptr = strstr(String+index, str);
    if( ptr == NULL ){
        return npos;
    }else{
        return (size_t)(ptr-String);
    }
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
int wString::Pos(const char* pattern)
{
    char* ptr = strstr(String,pattern);
    if( ptr == NULL ){
        return npos;
    }else{
        return (int)(ptr-String);
    }
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
int wString::Pos(const char* pattern,int pos)
{
    char* ptr = strstr(String+pos,pattern);
    if( ptr == NULL ){
        return npos;
    }else{
        return (int)(ptr-String);
    }
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
int wString::Pos(wString& pattern)
{
    return Pos(pattern.String);
}
//---------------------------------------------------------------------------
// 位置
//---------------------------------------------------------------------------
int wString::Pos(wString& pattern, int pos)
{
    return Pos(pattern.String,pos);
}
//---------------------------------------------------------------------------
//　Size
//---------------------------------------------------------------------------
size_t  wString::size(void) const
{
    return (size_t)len;
}
//---------------------------------------------------------------------------
//　Size
//---------------------------------------------------------------------------
size_t  wString::length(void) const
{
    return (size_t)len;
}
//---------------------------------------------------------------------------
// ファイル読み込み
//---------------------------------------------------------------------------
void wString::LoadFromFile(wString& str)
{
    LoadFromFile(str.String);
}
//---------------------------------------------------------------------------
// ファイル読み込み
//---------------------------------------------------------------------------
int wString::LoadFromFile(const char* FileName)
{
    long flen;
    int  handle;
    if( strncmp( FileName,"http://",7) == 0 ){
        wString tmp;
        tmp = wString::HTTPGet((char*)FileName,0);
        *this = tmp;
    }else{
        #ifdef linux
        handle  = open(FileName,O_RDONLY | S_IREAD );
        #else
        handle  = myopen(FileName,O_RDONLY | O_BINARY | S_IREAD );
        #endif
        if( handle<0 ){
            return -1;
        }
        flen = lseek(handle,0,SEEK_END);
        lseek(handle,0,SEEK_SET);
        SetLength(flen+1);
        len = read( handle, String, flen);
        close(handle);
        String[len] = 0;
        //\0がある場合を考えればstrlenとってはいけない
        //len = strlen(String);
        CalcCount();
    }
    return 0;
}
//---------------------------------------------------------------------------
// ファイル読み込み
//---------------------------------------------------------------------------
void wString::LoadFromCSV(wString& str)
{
    LoadFromCSV(str.String);
}
int isNumber(char* str)
{
    for( int i = strlen(str)-1; i >= 0 ; i-- ){
        if( (! isdigit(str[i]) ) && str[i] != '.' ){
            return 0;
        }
    }
    return 1;
}
            
//---------------------------------------------------------------------------
// ファイル読み込み
//---------------------------------------------------------------------------
void wString::LoadFromCSV(const char* FileName)
{
    int  fd;
    char s[1024];
    char t[1024];;
    int ret;
    int first=1;
    fd = myopen(FileName, O_RDONLY | O_BINARY );
    if( fd < 0 ){
        printf( "%sファイルが開けません\n", FileName );
        return;
    }
    *this = "[";
    //１行目はタイトル
    while(true){
        ret = readLine( fd, s, sizeof(s) );
        if( ret < 0 ) break;
        //分解する
        char *p = strtok(s,",");
        int ptr=0;
        if( p ){
            if( isNumber(p) ){ 
                ptr += ::sprintf( t+ptr,"%s", p );
            }else{
                ptr += ::sprintf( t+ptr,"\"%s\"", p );
            }
        }
        while( p = strtok(NULL,",") ){
            if( isNumber(p) ){ 
                ptr += ::sprintf( t+ptr,",%s", p );
            }else{
                ptr += ::sprintf( t+ptr,",\"%s\"", p );
            }
        }
        if( first ){
            first = 0;
        }else{
            *this += ",";
        }
        *this += wString("[")+t+"]";
    }
    *this += "]";
    close( fd );
    return;
}
//---------------------------------------------------------------------------
// ファイル書き込み
//---------------------------------------------------------------------------
int wString::SaveToFile(wString& str)
{
    return SaveToFile(str.String);
}
//---------------------------------------------------------------------------
// ファイル書き込み
//---------------------------------------------------------------------------
int wString::SaveToFile(const char* FileName)
{
    #ifdef linux
    int handle = myopen(FileName,O_CREAT| O_TRUNC | O_RDWR, S_IREAD| S_IWRITE);
    #else
    int handle = myopen(FileName,O_CREAT| O_TRUNC | O_RDWR| O_BINARY, S_IREAD| S_IWRITE);
    #endif
    if( handle < 0 ){
        return handle;
    }
    write( handle,String, len);
    close( handle);
    return 0;
}
//---------------------------------------------------------------------------
// トリム
//---------------------------------------------------------------------------
wString& wString::Trim(void)
{
    wString* temp = NextSac();
    copy(temp,this);
    if( len ){
        //先頭の空白等を抜く
        while( *temp->String && *temp->String <= ' ' ){
            #ifdef linux
            char* ptr=temp->String;
            while( *(ptr+1) ){
                *ptr = *(ptr+1);
                ptr++;
            }
            #else
            strcpy( (char*)temp->String, (char*)(temp->String+1) );
            #endif
            temp->len--;
        }
        //末尾の空白等を抜く
        while( temp->len && String[temp->len-1] <= ' ' ){
            temp->String[--temp->len] = 0;
        }
    }
    return *temp;
}
//---------------------------------------------------------------------------
// トリム
//---------------------------------------------------------------------------
wString& wString::RTrim(void)
{
    wString* temp = NextSac();
    copy(temp,this);
    if( len ){
        //末尾の空白等を抜く
        while( temp->len && String[temp->len-1] <= ' ' ){
            temp->String[--temp->len] = 0;
        }
    }
    return *temp;
}
//---------------------------------------------------------------------------
// トリム
//---------------------------------------------------------------------------
wString& wString::LTrim(void)
{
    wString* temp = NextSac();
    copy(temp,this);
    if( len ){
        //先頭の空白等を抜く
        while( *temp->String && *temp->String <= ' ' ){
            char* ptr=temp->String;
            while( *(ptr+1) ){
                *ptr = *(ptr+1);
                ptr++;
            }
            temp->len--;
        }
    }
    return *temp;
}
//---------------------------------------------------------------------------
// substr
//---------------------------------------------------------------------------
wString& wString::substr(int index) const
{
    wString* temp = NextSac();
    int newLen = len-index;
    if( newLen>0 ){
        temp->myrealloc(newLen);
        memcpy(temp->String, String+index,newLen);
        temp->String[newLen] =0;
        temp->len = newLen;
    }
    return *temp;
}
//---------------------------------------------------------------------------
// substr
//---------------------------------------------------------------------------
wString& wString::substr(int index, int mylen) const
{
    wString* temp = NextSac();
    int newLen = mylen;
    if( newLen > (int)len-index ){
        newLen = len-index;
    }
    if( newLen>0 ){
        temp->myrealloc(newLen);
        memcpy(temp->String,String+index,newLen);
        temp->String[newLen] = 0;
        temp->len = newLen;
    }
    return *temp;
}
//--------------------------------------------------------------------
wString& wString::FileStats(const char* str,int mode)
{
    struct stat      stat_buf;
    wString* buf=NextSac();
    if (stat(str, &stat_buf) == 0 && mode == 0 ) {
        /* ファイル情報を表示 */
        buf->sprintf( "{\"permission\":\"%o\",\"size\":%d,\"date\":\"%s\"}",stat_buf.st_mode, stat_buf.st_size, ctime(&stat_buf.st_mtime));
        //printf("デバイスID : %d\n",stat_buf.st_dev);
        //printf("inode番号 : %d\n",stat_buf.st_ino);
        //printf("アクセス保護 : %o\n",stat_buf.st_mode );
        //printf("ハードリンクの数 : %d\n",stat_buf.st_nlink);
        //printf("所有者のユーザID : %d\n",stat_buf.st_uid);
        //printf("所有者のグループID : %d\n",stat_buf.st_gid);
        //printf("デバイスID（特殊ファイルの場合） : %d\n",stat_buf.st_rdev);
        //printf("容量（バイト単位） : %d\n",stat_buf.st_size);
        //printf("ファイルシステムのブロックサイズ : %d\n",stat_buf.st_blksize);
        //printf("割り当てられたブロック数 : %d\n",stat_buf.st_blocks);
        //printf("最終アクセス時刻 : %s",ctime(&stat_buf.st_atime));
        //printf("最終修正時刻 : %s",ctime(&stat_buf.st_mtime));
        //printf("最終状態変更時刻 : %s",ctime(&stat_buf.st_ctime));
    }else{
        //date
        if( mode == 1 ){
            //char s[128] = {0};
            //time_t timer;
            //struct tm *timeptr;
            //timer = time(NULL);
            //timeptr = localtime(&stat_buf.st_mtime);
            //strftime(s, 128, "%Y/%m/%d %H:%M:%S", timeptr);
            buf->sprintf( "%d",stat_buf.st_mtime );
        }
    }
    return *buf;
}
//---------------------------------------------------------------------------
wString& wString::FileStats(wString& str, int mode)
{
    return FileStats(str.String,mode);
}

//---------------------------------------------------------------------------
int wString::FileExists(char* str)
{
    int  flag=0;
    #ifdef linux
    struct stat send_filestat;
    int  result = stat(str, &send_filestat);
    if ( ( result == 0 ) && ( S_ISREG(send_filestat.st_mode) == 1 ) ){
        flag = 1;
    }
    #else
    struct ffblk send_filestat;
    int result = findfirst(str,&send_filestat, 0);
    if( result == 0 && ( send_filestat.ff_attrib & FA_DIREC ) == 0){
        flag = 1;
    }
    #endif
    return flag;
}
//---------------------------------------------------------------------------
int wString::FileExists(wString& str)
{
    return FileExists(str.String);
}
//---------------------------------------------------------------------------
//パス部分を抽出
wString& wString::ExtractFileDir(wString& str)
{
    int ptr;
    //todo SJIS/EUC対応するように
    wString* temp = NextSac();
    copy(temp,&str);
    ptr = temp->LastDelimiter( DELIMITER );
    temp->len = ptr;
    temp->String[ptr] = 0;
    return *temp;
}
//---------------------------------------------------------------------------
int wString::CreateDir(wString& str)
{
    int flag=0;
    if( ! DirectoryExists(str.String) ){ 
        #ifdef linux
        //0x777ではちゃんとフォルダできない
        flag = (mkdir( str.String,0777 ) != -1 );
        #else
        flag = (mkdir( str.String ) != -1 );
        #endif
    }
    return flag;
}
//---------------------------------------------------------------------------
//　TStringList対策
//  行数を返す
//---------------------------------------------------------------------------
void wString::ResetLength(unsigned int num)
{
    assert(total>(unsigned int)num);
    String[num] = 0;
    len = num;
}
//---------------------------------------------------------------------------
int wString::Count(void)
{
    return count;
}
//---------------------------------------------------------------------------
char* wString::c_str(void) const
{
    return String;
}
//---------------------------------------------------------------------------
int wString::Length(void) const
{
    return len;
}
//---------------------------------------------------------------------------
int wString::Total(void) const
{
    return total;
}
//---------------------------------------------------------------------------
int wString::LastDelimiter(const char* delim) const
{
    int pos = -1;
    int dlen = strlen( delim );
    for( int i = len-dlen ;i > 0 ; i--){
        if( strncmp( String+i, delim, dlen ) == 0 ){
            pos = i;
            break;
        }
    }
    return pos;
}
//---------------------------------------------------------------------------
bool wString::RenameFile(wString& src, wString& dst)
{
    if( rename(src.c_str(), dst.c_str()) >= 0 ){
        return true;
    }else{
        return false;
    }
}
//---------------------------------------------------------------------------
unsigned long wString::FileSizeByName(char* str)
{
    unsigned long pos;
    int handle;
    #ifdef linux
    handle = open(str,0);
    #else
    handle = open(str,O_BINARY);
    #endif
    pos = lseek( handle,0,SEEK_END);
    close( handle );
    return pos;
}
//---------------------------------------------------------------------------
unsigned long wString::FileSizeByName(wString& str)
{
    return FileSizeByName(str.String);
}
//---------------------------------------------------------------------------
wString& wString::ExtractFileName(const char* str, const char* delim)
{
    wString* tmp;
    tmp = NextSac();
    *tmp = str;
    return ExtractFileName(*tmp, delim);
}
//---------------------------------------------------------------------------
wString& wString::ExtractFileName(const wString& str, const char* delim)
{
    int pos = str.LastDelimiter(delim);
    wString* tmp = NextSac();
    copy(tmp,str.SubString(pos+1,str.Length()-pos+1));
    return *tmp;
}
//---------------------------------------------------------------------------
wString& wString::ExtractFileExt(const wString& str)
{
    int pos = str.LastDelimiter(".");
    wString* tmp=NextSac();
    copy(tmp,str.SubString(pos+1,str.length()-pos-1));
    return *tmp;
}

//---------------------------------------------------------------------------
wString& wString::ChangeFileExt(wString& str, const char* ext)
{
    int pos = str.LastDelimiter(".");
    wString* tmp=NextSac();
    copy(tmp,str.SubString(0,pos+1));
    *tmp += ext;
    return *tmp;
}
//---------------------------------------------------------------------------
int wString::DeleteFile(const wString& str)
{
    int flag=0;
    #ifdef linux
    flag = (unlink(str.String)==0);
    #else
    if( FileExists(str.String) ){
        if( unlink(str.String)==0){
            flag = 1;
        }
    }
    #endif
    return flag;
}
//---------------------------------------------------------------------------
int wString::DirectoryExists(char* str)
{
    int flag=0;
    #ifdef linux
    struct stat send_filestat;
    int  result = stat(str, &send_filestat);
    if ( ( result == 0 ) && ( S_ISDIR(send_filestat.st_mode) == 1 ) ){
        flag = 1;
    }
    #else
    struct ffblk send_filestat;
    int result = findfirst(str,&send_filestat, FA_DIREC );
    if( result >= 0){
        if ((send_filestat.ff_attrib & FA_DIREC) == FA_DIREC){
            flag = 1;
        }
    }
    #endif
    return flag;
}
//---------------------------------------------------------------------------
int wString::DirectoryExists(wString& str)
{
    return DirectoryExists(str.String);
}
/********************************************************************************/
// sentence文字列内のkey文字列をrep文字列で置換する。
// sentence:元文字列
// slen:元文字列の長さ
// p:置換前の位置
// klen:置換前の長さ
// rep:置換後文字列
/********************************************************************************/
void wString::replace_character_len(const char *sentence,int slen,const char* p,int klen,const char *rep)
{
    char* str;
    int rlen=strlen((char*)rep);
    int num;
    if( klen == rlen ){
        memcpy( (void*)p,rep,rlen);
        //前詰め置換そのままコピーすればいい
    }else if( klen > rlen ){
        num = klen-rlen;
        strcpy( (char*)p,(char*)(p+num));
        memcpy( (void*)p,rep,rlen);
        //置換文字が長いので後詰めする
    }else{
        num = rlen-klen;
        //pからrlen-klenだけのばす
        for( str = (char*)(sentence+slen+num) ; str > p+num ; str-- ){
            *str = *(str-num);
        }
        memcpy( (void*)p,rep,rlen);
    }
    return;
}
//---------------------------------------------------------------------------
// フォルダのファイルを数える
// 引数　wString Path:
// 戻値　ファイルリスト。ほっておいていいが、コピーしてほしい
wString& wString::EnumFolderjson(wString& Path)
{
    DIR                  *dir;
    struct dirent        *ent;
    wString              temp;
    wString              Path2;
    std::vector<wString> list;
    Path2 = Path;
    //Directoryオープン
    if ((dir = opendir(Path.String)) != NULL){
        //ファイルリスト
        while ((ent = readdir(dir)) != NULL){
            if( strcmp(ent->d_name,"." ) != 0 &&
            strcmp(ent->d_name,"..") != 0 ){
                list.push_back("\""+Path2+DELIMITER+ent->d_name+"\"");
            }
        }
        closedir(dir);
        
        //sort
        if( list.size()>0){
            for( unsigned int ii = 0 ; ii < list.size()-1; ii++){
                for( unsigned int jj = ii+1 ; jj < list.size(); jj++){
                    if( strcmp(list[ii].c_str(),list[jj].c_str())> 0 ){
                        std::swap(list[ii],list[jj]);
                    }
                }
            }
        }        temp = "[";
        for( unsigned int i = 0 ; i < list.size() ; i++ ){
            if( i ) temp += ",";
            temp += list[i];
        }
        temp += "]";
    }else{
        perror("ディレクトリのオープンエラー");
        exit(1);
    }
    wString* ret = NextSac();
    *ret = temp;
    return *ret;
}
//---------------------------------------------------------------------------
// フォルダのファイルを数える
// 引数　wString Path:
// 戻値　ファイルリスト。ほっておいていいが、コピーしてほしい
wString& wString::EnumFolder(wString& Path)
{
    #ifdef linux
    struct dirent **namelist;
    int n;
    wString temp;
    wString Path2;
    Path2 = Path;
    int first=1;

    n = scandir(Path.String, &namelist, NULL , alphasort);
    if (n < 0){
        temp = "";
    }
    else
    {
        temp = "[";
        for( int i = 0 ; i < n ; i++){
            if( first ){
                first = 0;
            }else{
                temp += ",";
            }
            temp += "\""+Path2+DELIMITER+namelist[i]->d_name+"\"" ;
            free(namelist[i]);
        }
        temp += "]";
        free(namelist);
    }
    #else
    DIR                  *dir;
    struct dirent        *ent;
    wString              temp;
    wString              Path2;
    std::vector<wString> list;
    Path2 = Path;
    //Directoryオープン
    if ((dir = opendir(Path.String)) != NULL){
        //ファイルリスト
        while ((ent = readdir(dir)) != NULL){
            if( strcmp(ent->d_name,"." ) != 0 &&
            strcmp(ent->d_name,"..") != 0 ){
                list.push_back( Path2+DELIMITER+ent->d_name );
            }
        }
        closedir(dir);
        if( list.size()>0){
            for( unsigned int ii = 0 ; ii < list.size()-1; ii++){
                for( unsigned int jj = ii+1 ; jj < list.size(); jj++){
                    if( strcmp(list[ii].c_str(),list[jj].c_str())> 0 ){
                        std::swap(list[ii],list[jj]);
                    }
                }
            }
        }
        for( unsigned int i = 0 ; i < list.size() ; i++ ){
            temp.Add(list[i]);
        }
    }else{
        perror("ディレクトリのオープンエラー");
        exit(1);
    }
    #endif
    wString* ret = NextSac();
    *ret = temp;
    return *ret;
}
#ifndef va_copy
int wString::vtsprintf(const char* fmt,va_list arg){
    int len;
    int size;
    int zeroflag,width;

    size = 0;
    len = 0;

    while(*fmt){
        if(*fmt=='%'){        /* % に関する処理 */
        zeroflag = width = 0;
        fmt++;

        if (*fmt == '0'){
            fmt++;
            zeroflag = 1;
        }
        while ((*fmt >= '0') && (*fmt <= '9')){
            width*=10;
            width+= *(fmt++) - '0';
        }
        
        /* printf ("zerof = %d,width = %d\n",zeroflag,width); */
        
        //lluもluもuも同じ
        while(*fmt == 'l' || *fmt == 'z'){
            *fmt++;
        }
        
        switch(*fmt){
            case 'd':        /* 10進数 */
            size = tsprintf_decimal(va_arg(arg,signed long),zeroflag,width);
            break;
            case 'u':        /* 10進数 */
            size = tsprintf_decimalu(va_arg(arg,unsigned long),zeroflag,width);
            break;
            case 'o':        /* 8進数 */
            size = tsprintf_octadecimal(va_arg(arg,unsigned long),zeroflag,width);
            break;
            case 'x':        /* 16進数 0-f */
            size = tsprintf_hexadecimal(va_arg(arg,unsigned long),0,zeroflag,width);
            break;
            case 'X':        /* 16進数 0-F */
            size = tsprintf_hexadecimal(va_arg(arg,unsigned long),1,zeroflag,width);
            break;
            case 'c':        /* キャラクター */
            size = tsprintf_char(va_arg(arg,int));
            break;
            case 's':        /* ASCIIZ文字列 */
            size = tsprintf_string(va_arg(arg,char*));
            break;
            default:        /* コントロールコード以外の文字 */
            /* %%(%に対応)はここで対応される */
            len++;
            *this += *fmt;
            break;
        }
        len += size;
        fmt++;
    } else {
        *this += *(fmt++);
        len++;
    }
}
va_end(arg);
return (len);
}


/*
数値 => 10進文字列変換
*/
int wString::tsprintf_decimal(signed long val,int zerof,int width)
{
    //末尾０を保証
    char tmp[22]={0};
    char* ptmp = tmp + 20;
    int len = 0;
    int minus = 0;
    
    if (!val){        /* 指定値が0の場合 */
    *(ptmp--) = '0';
    len++;
} else {
    /* マイナスの値の場合には2の補数を取る */
    if (val < 0){
        val = ~val;
        val++;
        minus = 1;
    }
    while (val){
        /* バッファアンダーフロー対策 */
        if (len >= 19){
            break;
        }
        
        *ptmp = (char)((val % 10) + '0');
        val /= 10;
        ptmp--;
        len++;
    }
    
}

/* 符号、桁合わせに関する処理 */
if (zerof){
    if (minus){
        width--;
    }
    while (len < width){
        *(ptmp--) =  '0';
        len++;
    }
    if (minus){
        *(ptmp--) = '-';
        len++;
    }
} else {
    if (minus){
        *(ptmp--) = '-';
        len++;
    }
    while (len < width){
        *(ptmp--) =  ' ';
        len++;
    }
}

*this += (ptmp+1);

return (len);
}
/*
数値 => 10進文字列変換
*/
int wString::tsprintf_decimalu(unsigned long val,int zerof,int width)
{
    char tmp[22]={0};
    char* ptmp = tmp + 20;
    int len = 0;
    int minus = 0;
    
    if (!val){        /* 指定値が0の場合 */
    *(ptmp--) = '0';
    len++;
} else {
    while (val){
        /* バッファアンダーフロー対策 */
        if (len >= 19){
            break;
        }
        
        *ptmp = (char)((val % 10) + '0');
        val /= 10;
        ptmp--;
        len++;
    }
    
}

/* 符号、桁合わせに関する処理 */
if (zerof){
    if (minus){
        width--;
    }
    while (len < width){
        *(ptmp--) =  '0';
        len++;
    }
    if (minus){
        *(ptmp--) = '-';
        len++;
    }
} else {
    while (len < width){
        *(ptmp--) =  ' ';
        len++;
    }
}

*this += (ptmp+1);
return (len);
}
/*
数値 => 8進文字列変換
*/
int wString::tsprintf_octadecimal(unsigned long val,int zerof,int width)
{
    char tmp[22]={0};
    char* ptmp = tmp + 20;
    int len = 0;
    int minus = 0;
    
    if (!val){        /* 指定値が0の場合 */
    *(ptmp--) = '0';
    len++;
} else {
    while (val){
        /* バッファアンダーフロー対策 */
        if (len >= 19){
            break;
        }
        
        *ptmp = (char)((val % 8) + '0');
        val /= 8;
        ptmp--;
        len++;
    }
    
}

/* 符号、桁合わせに関する処理 */
if (zerof){
    if (minus){
        width--;
    }
    while (len < width){
        *(ptmp--) =  '0';
        len++;
    }
    if (minus){
        *(ptmp--) = '-';
        len++;
    }
} else {
    while (len < width){
        *(ptmp--) =  ' ';
        len++;
    }
}

*this += (ptmp+1);
return (len);
}
/*
数値 => 16進文字列変換
*/
int wString::tsprintf_hexadecimal(unsigned long val,int capital,int zerof,int width)
{
    char tmp[22]={0};
    char* ptmp = tmp + 20;
    int len = 0;
    char str_a;
    
    /* A～Fを大文字にするか小文字にするか切り替える */
    if (capital){
        str_a = 'A';
    } else {
        str_a = 'a';
    }
    
    if (!val){        /* 指定値が0の場合 */
    *(ptmp--) = '0';
    len++;
} else {
    while (val){
        /* バッファアンダーフロー対策 */
        if (len >= 18){
            break;
        }
        
        *ptmp = (char)(val % 16);
        if (*ptmp > 9){
            *ptmp += (char)(str_a - 10);
        } else {
            *ptmp += '0';
        }
        
        val >>= 4;        /* 16で割る */
        ptmp--;
        len++;
    }
}
while (len < width){
    *(ptmp--) =  zerof ? '0' : ' ';
    len++;
}

*this += (ptmp+1);
return(len);
}

/*
数値 => 1文字キャラクタ変換
*/
int wString::tsprintf_char(int ch)
{
    *this += (char)ch;
    return(1);
}

/*
数値 => ASCIIZ文字列変換
*/
int wString::tsprintf_string(char* str)
{
    *this += str;
    return( strlen(str) );
}
#endif
//---------------------------------------------------------------------------
//wString 可変引数
int wString::sprintf(const char* format, ... )
{
    #ifndef va_copy
    String[0] = 0;
    len = 0;
    va_list ap;
    va_start(ap, format);
    vtsprintf(format, ap);
    va_end(ap);
    return len;
    #else
    int stat;
    //可変引数を２つ作る
    va_list ap1,ap2;
    va_start(ap1, format);
    va_copy (ap2, ap1);
    //最初はダミーで文字列長をシミュレート
    stat = vsnprintf(String, 0, format, ap1);
    SetLength( stat+1 );
    //実際に出力
    stat = vsprintf(String, format, ap2);
    va_end(ap1);
    va_end(ap2);
    len = stat;
    return stat;
    #endif
}
//---------------------------------------------------------------------------
//wString 可変引数
int wString::cat_sprintf(const char* format, ... )
{
    #ifndef va_copy
    int status;
    va_list ap;
    va_start(ap, format);
    status = this->vtsprintf(format, ap);
    va_end(ap);
    return status;
    #else
    int stat;
    //可変引数を２つ作る
    va_list ap1,ap2;
    va_start(ap1, format);
    va_copy (ap2, ap1);
    //最初はダミーで文字列長をシミュレート
    stat = vsnprintf(String, 0, format, ap1);
    SetLength( stat+len+1 );
    //実際に出力
    stat = vsprintf(String+len, format, ap2);
    va_end(ap1);
    va_end(ap2);
    len += stat;
    return stat;
    #endif
}
//---------------------------------------------------------------------------
//文字列をデリミタで切って、デリミタ後の文字を返す
//引数
//wString str           :入力文字列
//const char* delimstr  :切断文字列
//戻り値
//wString&              :切断後の文字列
//見つからない場合は長さ０の文字列
wString& wString::strsplit(const char* delimstr)
{
    
    wString* tmp = NextSac();
    int delimlen = strlen(delimstr);
    int pos = Pos(delimstr);
    if( pos >= 0 ){
        *tmp = SubString(pos+delimlen,len-pos-delimlen);
    }
    return *tmp;
}
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void wString::myrealloc(const int newsize)
{
    if( len>=total){
        printf( "not good %d %d",len,total );
        exit( 1);
    }
    if( (int)total<=newsize){
        total = newsize+1;
        char* tmp = new char[total];
        memcpy(tmp,String,len);
        tmp[len]=0;
        delete[] String;
        String = tmp;
    }else{
        //指定サイズが元より小さいので何もしない
    }
}
//---------------------------------------------------------------------------
char* wString::htmlspecialchars(void)
{
    unsigned int is;
    // 引数チェック
    if(len == 0 ){
        //０バイト
        return (char*)"";
    }
    wString* temp = NextSac();
    for ( is = 0 ; is < len ; is++){
        if       ( String[is] == '&' ){
            *temp += "&amp;";
        }else if ( String[is] == '\"' ){
            *temp += "&quot;";
            //}else if ( String[is] == '\'' ){
            //    *temp += "&#39;";
        }else if ( String[is] == '<' ){
            *temp += "&lt;";
        }else if ( String[is] == '>' ){
            *temp += "&gt;";
        }else{
            *temp += String[is];
        }
    }
    return temp->String;
}
//---------------------------------------------------------------------------
// **************************************************************************
//  URIエンコードを行います.
//  機能 : URIデコードを行う
//  書式 : int uri_encode
//  (char* dst,size_t dst_len,const char* src,int src_len);
//  引数 : dst 変換した文字の書き出し先.
//                 dst_len 変換した文字の書き出し先の最大長.
//                 src 変換元の文字.
//                 src_len 変換元の文字の長さ.
//  返値 : エンコードした文字の数(そのままも含む)
// **************************************************************************
char* wString::uri_encode(void)
{
    unsigned int is;
    wString dst;
    char work[8];
    // 引数チェック
    if(len == 0 ){
        //０バイト
        return (char*)"";
    }
    for ( is = 0 ; is < len ; is++){
        /* ' '(space) はちと特別扱いにしないとまずい */
        if ( String[is] == ' ' ){
            dst += "%20";
            /* エンコードしない文字全員集合 */
            //        }else if ( strchr("!$()*,-./:;?@[]^_`{}~", String[is]) != NULL ){
        }else if ( strchr("!$()*,-.:;/?@[]^_`{}~", String[is]) != NULL ){
            dst += String[is];
            /* アルファベットと数字はエンコードせずそのまま */
        }else if ( isalnum( String[is] ) ){
            dst += String[is];
        }
        /* \マークはエンコード */
        else if ( String[is] == '\\' ){
            dst += "%5c";
            /* それ以外はすべてエンコード */
        }else{
            ::sprintf(work,"%%%2X",(unsigned char)String[is]);
            dst += work;
        }
    }
    wString* temp = NextSac();
    *temp = dst;
    return temp->String;
}
int   wString::FileCopy(const char* fname_r, const char* fname_w)
{
    int fpr;
    int fpw;
    int size;
    unsigned char buf[8000];

    fpr = myopen( fname_r, O_RDONLY | O_BINARY );
    if( fpr < 0 ){
        return -1;
    }
    fpw = myopen( fname_w, O_CREAT | O_TRUNC  | O_WRONLY | O_BINARY , S_IREAD | S_IWRITE );
    if( fpw < 0 ){
        close( fpr );
        return -1;
    }
    while( 1 ){
        size = read( fpr , buf, sizeof(buf) );
        if( size <= 0 ){
            break;
        }
        write( fpw, buf, size );
    }
    close( fpr );
    close( fpw );
    
    return 0;
}
//---------------------------------------------------------------------------
wString& wString::addSlashes(void)
{
    wString* tmp = NextSac();
    for ( unsigned int i=0 ; i < len; i++) {
        if( String[i] == '\'' || String[i] == '\"' || String[i] == '\\' || String[i] == 0 ){
            *tmp += '\\';
        }
        *tmp += String[i];
    }
    return *tmp;
}
// **************************************************************************
// URIデコードを行います.
//  機能 : URIデコードを行う
//  引数 : dst 変換した文字の書き出し先.
//                dst_len 変換した文字の書き出し先の最大長.
//                src 変換元の文字.
//                src_len 変換元の文字の長さ.
// 返値 : デコードした文字の数(そのままも含む)
// **************************************************************************
inline unsigned char htoc(unsigned char x)
{
    if( '0'<=x && x<='9') return (unsigned char)(x-'0');
    if( 'a'<=x && x<='f') return (unsigned char)(x-'a'+10);
    if( 'A'<=x && x<='F') return (unsigned char)(x-'A'+10);
    return 0;
}
char* wString::uri_decode()
{
    size_t          i;
    unsigned char   code;
    // 引数チェック
    if (len==0){
        return String;
    }
    wString dst;
    // =================
    // メインループ
    // =================
    for (i = 0; i < len  && String[i] != '\0'; i++){
        if (String[i] == '%'){
            if (i + 2 >= len){
                break;
            }
            code  = (unsigned char)(htoc(String[++i])<<4);
            code += htoc(String[++i]);
            //            if( flag && code == '\"' ){
            //                dst += '\\';
            //            }
            dst += code;
        }else if ( String[i] == '+' ){
            dst += ' ';
        }else{
            //            if( flag && String[i] == '\"' ){
            //                dst += '\\';
            //            }
            dst += String[i];
        }
    }
    wString* temp = NextSac();
    *temp = dst;
    return temp->String;
}

void wString::headerInit(size_t content_length, int expire, const char* mime_type)
{
    sprintf( "%s", HTTP_OK );
    cat_sprintf( "%s", HTTP_CONNECTION );
    cat_sprintf( HTTP_SERVER_NAME,SERVER_NAME);
    cat_sprintf( HTTP_CONTENT_TYPE, mime_type);
    //Date
    time_t timer;
    time(&timer);
    struct tm *utc;
    utc = gmtime(&timer);
    char work[80];
    char you[7][4]={"Sun", "Mon","Tue", "Wed", "Thu", "Fri", "Sat"};
    char mon[12][4]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    ::sprintf( work, "%s, %d %s %d %02d:%02d:%02d",
    you[utc->tm_wday],utc->tm_mday,mon[utc->tm_mon],utc->tm_year+1900, utc->tm_hour, utc->tm_min, utc->tm_sec);
    this->cat_sprintf( "Date: %s GMT\r\n", work);
    //expire
    if( expire ){
        timer += 60*60;
        utc = gmtime(&timer);
        ::sprintf( work, "%s, %d %s %d %02d:%02d:%02d",
        you[utc->tm_wday],utc->tm_mday,mon[utc->tm_mon],utc->tm_year+1900, utc->tm_hour, utc->tm_min, utc->tm_sec);
        this->cat_sprintf( "Expires: %s GMT\r\n", work);
    }
    if( content_length ){
        this->cat_sprintf( HTTP_CONTENT_LENGTH, content_length );
    }
}
void wString::headerPrint(int socket,int endflag)
{
    send( socket , String , len , 0 );
    if( endflag ){
        send( socket , HTTP_END , strlen( HTTP_END) , 0 );
    }
}
wString& wString::headerPrintMem(void)
{
    wString* temp = NextSac();
    *temp = *this;
    *temp += HTTP_END;
    return *temp;
}
int wString::header(const char* str,int flag, int status)
{
    char head[80]={0};
    char body[80]={0};
    if( *str ){
        if( ! flag ){
            Add(str);
            return 0;
        }
        if( strncmp( str, "HTTP/", 5) == 0 ){
            memcpy( head, str,5);
            head[5]=0;
            strcpy( body, str+5);
        }else{
            for( size_t i = 0 ; i < strlen( str ) ; i++ ){
                if( str[i] == ' ' ){
                    memcpy( head, str, i+1 );
                    head[i+1] = 0;
                    strcpy( body, str+i+1);
                    break;
                }
            }
        }
        if( *head && *body ){
            if( strcmp( head, "Location: ")==0 ){
                if( status == 301 ){
                    header( "HTTP/1.0 301 Moved Permanetry", true );
                }else{
                    header( "HTTP/1.0 302 Found", true );
                }
            }else{
                int count = CalcCount();
                wString str2;
                str2.sprintf( "%s%s", head, body );
                for( int i = 0 ; i < count ; i++ ){
                    wString tmp = GetListString(i);
                    if( strncmp(tmp.c_str(), head, strlen(head)) == 0 ){
                        SetListString( str2.c_str(), i );
                        return 0;
                    }
                }
                Add(str2);
                return 0;
            }
        }
    }
    return 1;
}
// MP3 ID3v1 タグ情報
unsigned char   wString::mp3_id3v1_flag;         // MP3 タグ 存在フラグ
unsigned char   wString::mp3_id3v1_title[128];   // MP3 曲名
unsigned char   wString::mp3_id3v1_album[128];   // MP3 アルバム名
unsigned char   wString::mp3_id3v1_artist[128];  // MP3 アーティスト
unsigned char   wString::mp3_id3v1_year[128];    // MP3 制作年度
unsigned char   wString::mp3_id3v1_comment[128]; // MP3 コメント
wString& wString::mp3_id3_tag(const char* filename)
{
    wString tmp;
    if( ! mp3_id3_tag_read((unsigned char*)filename) ){
        tmp = "{";
        tmp.cat_sprintf( "\"title\":\"%s\"",mp3_id3v1_title);
        tmp.cat_sprintf( ",\"album\":\"%s\"",mp3_id3v1_album);
        tmp.cat_sprintf( ",\"artist\":\"%s\"",mp3_id3v1_artist);
        tmp.cat_sprintf( ",\"year\":\"%s\"",mp3_id3v1_year);
        tmp.cat_sprintf( ",\"comment\":\"%s\"}",mp3_id3v1_comment);
    }else{
        tmp = "{\"title\":\"\",\"album\":\"\",\"artist\":\"\",\"year\":\"\",\"comment\":\"\"}";
    }
    wString* temp = NextSac();
    *temp = tmp;
    return *temp;
}
wString& wString::mp3_id3_tag(wString& filename)
{
    wString tmp;
    if( ! mp3_id3_tag_read((unsigned char*)filename.c_str()) ){
        tmp = "{";
        tmp.cat_sprintf( "\"title\":\"%s\"",mp3_id3v1_title);
        tmp.cat_sprintf( ",\"album\":\"%s\"",mp3_id3v1_album);
        tmp.cat_sprintf( ",\"artist\":\"%s\"",mp3_id3v1_artist);
        tmp.cat_sprintf( ",\"year\":\"%s\"",mp3_id3v1_year);
        tmp.cat_sprintf( ",\"comment\":\"%s\"}",mp3_id3v1_comment);
    }else{
        tmp = "{\"title\":\"\",\"album\":\"\",\"artist\":\"\",\"year\":\"\",\"comment\":\"\"}";
    }
    wString* temp = NextSac();
    *temp = tmp;
    return *temp;
}
// ***************************************************************************
// mp3ファイル解析
// ***************************************************************************
int  wString::mp3_id3_tag_read(unsigned char *mp3_filename )
{
    if( mp3_id3v1_tag_read(mp3_filename) != 0 ){
        if (mp3_id3v2_tag_read(mp3_filename) != 0) {
            return -1;
        }
    }
    if ( mp3_id3v1_flag > 0 ) {
        if ( mp3_id3v1_title[0] == '\0' ) {
            mp3_id3v1_flag = 0;
        } else {
            return 0;
        }
    }
    return -1;
}
// **************************************************************************
// * MP3ファイルから、ID3v1形式のタグデータを得る
// **************************************************************************
int  wString::mp3_id3v1_tag_read(unsigned char *mp3_filename )
{
    int fd;
    unsigned char       buf[128];
    off_t               length;
    memset(buf,                         '\0', sizeof(buf));
    fd = open((char*)mp3_filename,  O_RDONLY);
    if ( fd < 0 )
    {
        return -1;
    }
    // 最後から128byteへSEEK
    length = lseek(fd, -128, SEEK_END);
    if( length < 0 ){
        return -1;
    }
    // ------------------
    // "TAG"文字列確認
    // ------------------
    // 3byteをread.
    read(fd, buf, 3);
    //debug_log_output("buf='%s'", buf);
    // "TAG" 文字列チェック
    if ( strncmp( (char*)buf, "TAG", 3 ) != 0 )
    {
        //debug_log_output("NO ID3 Tag.");
        close(fd);
        return -1;              // MP3 タグ無し。
    }
    // ------------------------------------------------------------
    // Tag情報read
    //
    //  文字列最後に0xFFと' 'が付いていたら削除。
    //  client文字コードに変換。
    // ------------------------------------------------------------
    // 曲名
    memset((char*)buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    rtrim((char*)buf, 0xFF);
    rtrim((char*)buf, ' ');
    convert_language_code( buf,sizeof(buf),CODE_AUTO, CODE_UTF8);
    strcpy( (char*)mp3_id3v1_title, (char*)buf );
    
    // アーティスト
    memset((char*)buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    rtrim((char*)buf, 0xFF);
    rtrim((char*)buf, ' ');
    convert_language_code( buf,sizeof(buf),CODE_AUTO, CODE_UTF8);
    strcpy( (char*)mp3_id3v1_artist, (char*)buf );
    
    // アルバム名
    memset((char*)buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    rtrim((char*)buf, 0xFF);
    rtrim((char*)buf, ' ');
    convert_language_code( buf,sizeof(buf),CODE_AUTO, CODE_UTF8);
    strcpy( (char*)mp3_id3v1_album, (char*)buf );
    
    // 制作年度
    memset((char*)buf, '\0', sizeof(buf));
    read(fd, buf, 4);
    rtrim((char*)buf, 0xFF);
    rtrim((char*)buf, ' ');
    convert_language_code( buf,sizeof(buf),CODE_AUTO, CODE_UTF8);
    strcpy( (char*)mp3_id3v1_year, (char*)buf );
    // コメント
    memset((char*)buf, '\0', sizeof(buf));
    read(fd, buf, 28);
    rtrim((char*)buf, 0xFF);
    rtrim((char*)buf, ' ');
    convert_language_code( buf,sizeof(buf),CODE_AUTO, CODE_UTF8);
    strcpy( (char*)mp3_id3v1_comment, (char*)buf );
    // ---------------------
    // 存在フラグ
    // ---------------------
    mp3_id3v1_flag = 1;
    close(fd);
    return 0;
}
size_t wString::id3v2_len(unsigned char *buf)
{
    return buf[0] * 0x200000 + buf[1] * 0x4000 + buf[2] * 0x80 + buf[3];
}
// **************************************************************************
// * MP3ファイルから、ID3v2形式のタグデータを得る
// * 0: 成功  -1: 失敗(タグなし)
// **************************************************************************
int  wString::mp3_id3v2_tag_read(unsigned char *mp3_filename )
{
    int fd;
    unsigned char       buf[1024];
    unsigned char       *frame;
    off_t               len;
    struct _copy_list {
        unsigned char id[5];
        unsigned char *container;
        size_t maxlen;
    } copy_list[] = {
        { "TIT2", mp3_id3v1_title
        , sizeof(mp3_id3v1_title) },
        { "TPE1", mp3_id3v1_artist
        , sizeof(mp3_id3v1_artist) },
        { "TALB", mp3_id3v1_album
        , sizeof(mp3_id3v1_album) },
        { "TCOP", mp3_id3v1_year
        , sizeof(mp3_id3v1_year) },
        { "TYER", mp3_id3v1_year
        , sizeof(mp3_id3v1_year) },
        { "COMM", mp3_id3v1_comment
        , sizeof(mp3_id3v1_comment) },
    };
    int list_count = sizeof(copy_list) / sizeof(struct _copy_list);
    int i;
    int flag_extension = 0;
    memset(buf, '\0', sizeof(buf));
    fd = open((char*)mp3_filename,  O_RDONLY);
    if ( fd < 0 )
    {
        return -1;
    }
    // ------------------
    // "ID3"文字列確認
    // ------------------
    // 10byteをread.
    read(fd, buf, 10);
    // debug_log_output("buf='%s'", buf);
    // "ID3" 文字列チェック
    if ( strncmp( (char*)buf, "ID3", 3 ) != 0 )
    {
        /*
        *  ファイルの後ろにくっついてる ID3v2 タグとか
        *  ファイルの途中にあるのとか 面倒だから 読まないよ。
        */
        //debug_log_output("NO ID3v2 Tag.");
        close(fd);
        return -1;              // v2 タグ無し。
    }
    //debug_log_output("ID3 v2.%d.%d Tag found", buf[3], buf[4]);
    //debug_log_output("ID3 flag: %02X", buf[5]);
    if (buf[5] & 0x40) {
        //debug_log_output("ID3 flag: an extended header exist.");
        flag_extension = 1;
    }
    len = id3v2_len(buf + 6);
    if (flag_extension) {
        int exlen;
        if (read(fd, buf, 6) != 6) {
            close(fd);
            return -1;
        }
        exlen = id3v2_len(buf);
        //debug_log_output("ID3 ext. flag: len = %d", exlen);
        if (exlen < 6) {
            //debug_log_output("invalid ID3 ext. header.");
            close(fd);
            return -1;
        } else if (exlen > 6) {
            //debug_log_output("large ID3 ext. header.");
            lseek(fd, exlen - 6, SEEK_CUR);
        }
        len -= exlen;
    }
    // ------------------------------------------------------------
    // Tag情報read
    //
    //  client文字コードに変換。
    // ------------------------------------------------------------
    while (len > 0) {
        int frame_len;
        /* フレームヘッダを 読み込む */
        if (read(fd, buf, 10) != 10) {
            close(fd);
            return -1;
        }
        /* フレームの長さを算出 */
        frame_len = id3v2_len(buf + 4);
        /* フレーム最後まで たどりついた */
        unsigned long *ppp = (unsigned long*)buf;
        if (frame_len == 0 || *ppp == 0){
            break;
        }
        for (i=0; i<list_count; i++) {
            if (!strncmp((char*)buf, (char*)copy_list[i].id, 4)) break;
        }
        if (i < list_count) {
            // 解釈するタグ 発見
            // 存在フラグ
            mp3_id3v1_flag = 1;
            frame = (unsigned char*)malloc(frame_len + 1);
            memset(frame, '\0', frame_len + 1);
            if (read(fd, frame, frame_len) != frame_len) {
                //debug_log_output("ID3v2 Tag[%s] read failed", copy_list[i].id);
                free(frame);
                close(fd);
                return -1;
            }
            //debug_log_output("ID3v2 Tag[%s] found. '%s'", copy_list[i].id, frame + 1);
            rtrim((char*)frame + 1, ' ');
            //convert_language_code( frame+1,strlen(frame+1),CODE_AUTO, CODE_UTF8);
            strncpy( (char*)copy_list[i].container, (char*)(frame+1), copy_list[i].maxlen );
            convert_language_code( copy_list[i].container,copy_list[i].maxlen,CODE_AUTO, CODE_UTF8);
            free(frame);
        } else {
            /* マッチしなかった */
            buf[4] = '\0';
            //debug_log_output("ID3v2 Tag[%s] skip", buf);
            lseek(fd, frame_len, SEEK_CUR);
        }
        len -= (frame_len + 10); /* フレーム本体 + フレームヘッダ */
    }
    close(fd);
    return mp3_id3v1_flag ? 0 : -1;
}
// ***************************************************************************
// sentence文字列の行末に、cut_charがあったとき、削除
// ***************************************************************************
void wString::rtrim(char *sentence, char cut_char)
{
    char        *source_p;
    int         length, i;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    length = strlen(sentence);          // 文字列長Get
    source_p = sentence;
    source_p += length;                 // ワークポインタを文字列の最後にセット。
    for (i=0; i<length; i++)    {       // 文字列の数だけ繰り返し。
        source_p--;                     // 一文字ずつ前へ。
        if (*source_p == cut_char){     // 削除キャラ ヒットした場合削除
            *source_p = '\0';
        }else{                          // 違うキャラが出てきたところで終了。
            break;
        }
    }
    return;
}
/********************************************************************************/
// 日本語文字コード変換。
// (libnkfのラッパー関数)
//
//      サポートされている形式は以下の通り。
//              in_flag:        CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//              out_flag:       CODE_SJIS, CODE_EUC
/********************************************************************************/
void wString::convert_language_code(const unsigned char *in, size_t len, int in_flag, int out_flag)
{
    unsigned char       nkf_option[8];
    unsigned char      *out;
    out = (unsigned char*)(new char[len*3]);
    memset(nkf_option, '\0', sizeof(nkf_option));
    //=====================================================================
    // in_flag, out_flagをみて、libnkfへのオプションを組み立てる。
    //=====================================================================
    switch( in_flag )
    {
        case CODE_SJIS:
        strncpy((char*)nkf_option, "S", sizeof(nkf_option));
        break;
        case CODE_EUC:
        strncpy((char*)nkf_option, "E", sizeof(nkf_option));
        break;
        case CODE_UTF8:
        strncpy((char*)nkf_option, "W", sizeof(nkf_option));
        break;
        case CODE_UTF16:
        strncpy((char*)nkf_option, "W16", sizeof(nkf_option));
        break;
        case CODE_AUTO:
        default:
        strncpy((char*)nkf_option, "", sizeof(nkf_option));
        break;
    }
    switch( out_flag )
    {
        case CODE_EUC:
        strncat((char*)nkf_option, "e", sizeof(nkf_option) - strlen((char*)nkf_option) );
        break;
        case CODE_SJIS:
        strncat((char*)nkf_option, "s", sizeof(nkf_option) - strlen((char*)nkf_option) );
        break;
        case CODE_UTF8:
        default:
        strncat((char*)nkf_option, "w", sizeof(nkf_option) - strlen((char*)nkf_option) );
        break;
    }
    //=================================================
    // libnkf 実行
    //=================================================
    nkf((const char*)in, (char*)out, len, (const char*)nkf_option);
    strcpy( (char*)in, (char*)out );
    delete []out;
    //memcpy((char*)in,(char*)out,len);
    return;
}
/********************************************************************************/
// 日本語文字コード変換。
// (libnkfのラッパー関数)
//
//      サポートされている形式は以下の通り。
//              in_flag:        CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//              out_flag:       CODE_SJIS, CODE_EUC
/********************************************************************************/
wString& wString::nkfcnv(const wString& option)
{
    wString* ptr = NextSac();
    ptr->myrealloc( len*3);
    //=================================================
    // libnkf 実行
    //=================================================
    nkf((const char*)String, (char*)ptr->c_str(),len*3, (const char*)option.c_str());
    ptr->len = strlen( ptr->c_str() );
    return *ptr;
}
//---------------------------------------------------------------------------
void wString::Add(const char* str)
{
    *this += str;
    *this += "\r\n";
    count++;
}
//---------------------------------------------------------------------------
void wString::Add(wString& str)
{
    *this += str;
    *this += "\r\n";
    count++;
}

//---------------------------------------------------------------------------
//　TStringList対策
//  行数を返す
//---------------------------------------------------------------------------
int wString::CalcCount(void)
{
    count = 0;
    char* ptr = String;
    while( *ptr ){
        if( *ptr == '\r' ){
            ptr++;
            continue;
        }
        if( *ptr++ == '\n' ){
            count++;
        }
    }
    return count;
}

//---------------- -----------------------------------------------------------
//　TStringList対策
//  行数を返す
//　リエントラントでないことに注意
//　戻り値は使い捨て
//---------------- -----------------------------------------------------------
bool wString::SetListString(wString& src,int pos)
{
    bool flag;
    flag = SetListString(src.String,pos);
    return flag;
}
//---------------- -----------------------------------------------------------
//　TStringList対策
//  行数を返す
//　リエントラントでないことに注意
//　戻り値は使い捨て
//---------------- -----------------------------------------------------------
bool wString::SetListString(const char* dst,int pos)
{
    
    int ccount = 0;
    int ptr =0;
    int ptr0=0;
    int ptr1;
    int dstlen = strlen(dst);
    int srclen;
    
    //行数が多い
    if( pos > count ){
        return false;
    }
    //伸ばす
    //if( total < len+dstlen+1 ){
    //    total = len+dstlen+1;
    //    char* tmp = (char*)myrealloc(String,total);
    //    String = tmp;
    // }
    //debug_log_output("SetListString=%s(%d)%s(%d),%d\n",String,len,dst,dstlen,pos);
    
    //置換
    while( String[ptr] ){
        if( String[ptr++] == '\r' ){
            if( String[ptr++] == '\n' ){
                ptr1 = ptr0;
                ptr0 = ptr;
                if( ccount++ == pos ){
                    srclen = ptr0-ptr1-2;
                    if( dstlen>srclen ){
                        //伸びる分
                        myrealloc(len+(dstlen-srclen)+1);
                    }
                    replace_character_len(String,
                    len,
                    String+ptr1,
                    srclen,
                    dst);
                    len = strlen(String);
                    return true;
                }
            }
        }
    }
    len = total-1;
    return false;
}
//---------------------------------------------------------------------------
//　TStringList対策
//  行数を返す
//　リエントラントでないことに注意
//　戻り値は使い捨て
wString& wString::GetListString(int pos)
{
    int   ccount = 0;
    int   ptr =0;
    int   ptr0=0;
    int   ptr1;
    int   inc=0;
    //assert(ptr>=0&&String !=NULL);
    if( len == 0 ){
        return *NextSac();
    }
    while( String[ptr] ){
        if( String[ptr] == '\r' ){
            ptr++;
            inc++;
            continue;
        }
        if( String[ptr++] == '\n' ){
            ptr1 = ptr0;
            ptr0 = ptr;
            inc++;
            if( ccount++ == pos ){
                wString* work = NextSac();
                copy(work,SubString(ptr1,ptr0-ptr1-inc));
                return *work;
            }
        }
        inc = 0;
    }
    //０バイト
    return *NextSac();
}
//---------------------------------------------------------------------------
//HTTPより文字列データを取得
//引数
// char* src    :URL
// off_t offset :読み込みオフセット
//戻り値
// wString&     :読み込んだ文字列。失敗したときは長さ０
//---------------------------------------------------------------------------
wString& wString::HTTPGet(char* url, off_t offset)
{
    int         recv_len;                       //読み取り長さ
    wString     buf;
    wString     ptr;
    wString     host;                           //ホスト名
    wString     target;                         //ファイル名
    int         work1;
    int         work2;
    int         work3;
    SOCKET      server_socket;                  //サーバーソケット
    int         server_port = HTTP_SERVER_PORT;
    //出力ファイルの設定
    // ================
    // 実体転送開始
    // ================
    //buf = (char*)malloc(HTTP_BUF_SIZE);
    //ptr = buf;
    //準備
    //アドレスから、ホスト名とターゲットを取得
    ptr.SetLength(HTTP_STR_BUF_SIZE+1);
    buf = url;
    //ptr = 0;
    work1 = buf.Pos("://" )+3;
    work2 = buf.Pos("/", work1 );
    work3  = buf.Pos(":",work1);
    target = buf.SubString(work2,buf.len-work2);
    if( work3 >= 0 ){
        host   = buf.SubString(work1,work3-work1);
        server_port = atoi(buf.c_str()+work3+1);
    }else{
        host   = buf.SubString(work1,work2-work1);
    }
    //ソケット作成と接続
    server_socket = sock_connect(host.String, server_port);
    if ( ! SERROR(server_socket) ){
        //HTTP1.0 GET発行
        ptr.sprintf( "GET %s HTTP/1.0\r\n"
        "Accept: */*\r\n"
        "User-Agent: %s%s\r\nHost: %s\r\nRange: bytes=%llu-\r\nConnection: close\r\n\r\n" ,
        target.String ,
        //"Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
        USERAGENT,
        MACADDR,
        host.String,
        offset);
        //ptr.len = strlen(ptr.String);
        //サーバに繋がった
        if( send( server_socket, ptr.String, ptr.len , 0) != SOCKET_ERROR ){
            
            //初回分からヘッダを削除
            recv_len  = recv(server_socket, ptr.String, ptr.Total()-1, 0);
            ptr.String[recv_len] = 0;
            ptr.len = recv_len;
            //見つからない
            work1 = atoi(ptr.String+(ptr.Pos(" ")+1));
            if( work1 < 200 || 300 <= work1 ){
                sClose(server_socket);
                return *NextSac();
            }
            //content_length = atoi(buf.String+buf.Pos("Content-Length:" )+16);
            
            //\r\n\r\nを探す
            work1      = ptr.Pos(HTTP_DELIMITER )+4;//sizeof( HTTP_DELIMITER );//実体の先頭
            recv_len  -= work1;
            buf        = ptr.SubString(work1,recv_len);
            //転送する
            while(loop_flag){
                recv_len  = recv(server_socket, ptr.String, ptr.Total()-1, 0);
                if ( recv_len <= 0 ){
                    break;
                }
                //エラーにならない。
                ptr.len = recv_len;
                ptr.String[recv_len] = 0;
                buf += ptr;
            }
        }else{
            sClose(server_socket);
            return *NextSac();
        }
        sClose(server_socket);
    }else{
        return *NextSac();
    }
    wString* tmp = NextSac();
    copy(tmp,buf);
    tmp->CalcCount();
    //終了
    return *tmp;
}
//---------------------------------------------------------------------------
/* ソケットを作成し、相手に接続するラッパ. 失敗 = -1 */
//static int sock_connect(char *host, int port)
//---------------------------------------------------------------------------
SOCKET wString::sock_connect(wString& host, int port)
{
    return sock_connect(host.String,port);
}
//---------------------------------------------------------------------------
SOCKET wString::sock_connect(char *host, int port)
{
    //  int sock;
    SOCKET sock;
    struct sockaddr_in sockadd={0};     //ＳＯＣＫＥＴ構造体
    struct hostent *hent;
    debug_log_output("sock_connect: %s:%d", host, port);
    //ＳＯＣＫＥＴ作成
    if (SERROR(sock = socket(PF_INET, SOCK_STREAM, 0))){
        debug_log_output("sock_connect_error:");
        return INVALID_SOCKET;
    }
    //debug_log_output("sock: %d", sock);
    if (NULL == (hent = gethostbyname(host))) {
        //              close(sock);
        sClose( sock );
        return INVALID_SOCKET;
    }
    debug_log_output("hent: %p", hent);
    debug_log_output("hent: %p", hent);
    //ソケット構造体へアドレス設定
    memcpy(&sockadd.sin_addr, hent->h_addr, hent->h_length);
    //ソケット構造体へポート設定
    sockadd.sin_port = htons((u_short)port);
    //ＩＰＶ４アドレスファミリを設定
    sockadd.sin_family = AF_INET;
    //接続
    if (SERROR(connect(sock, (struct sockaddr*)&sockadd, sizeof(sockadd)))) {
        sClose( sock );
        return INVALID_SOCKET;
    }
    return sock;
}
//---------------------------------------------------------------------------
// 2004/11/17 Add start
// アクセス可能かチェックする
//形式はwww.make-it.co.jp/index.html
//2005/12/03 引数を壊していたので修正
bool wString::checkUrl( wString& url )
{
    wString buf;
    int     recv_len;
    bool    access_flag = false;
    wString host_name;
    wString file_path;
    int     ptr;
    //前処理
    ptr = url.Pos("/");
    // はじめに出てきた"/"の前後で分断
    host_name = url.SubString(0,ptr);
    file_path = url.SubString(ptr,url.Length()-ptr);
    //見つからなかった時
    if( file_path.Length() == 0 ){
        file_path = "/";
    }
    SOCKET server_socket = sock_connect(host_name.c_str(),HTTP_SERVER_PORT);//( PF_INET , SOCK_STREAM , 0 );
    if( ! SERROR(server_socket) ){
        buf.sprintf(  "HEAD %s HTTP/1.0\r\n"
        "User%cAgent: "
        USERAGENT
        "\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n" ,
        file_path.c_str() ,
        '-',
        host_name.c_str()
        );
        int dd = send( server_socket, buf.c_str(), buf.Length() , 0);
        if(  dd != SOCKET_ERROR ){
            buf.SetLength(1024);
            recv_len = recv(server_socket, buf.c_str(), buf.Total()-1, 0);
            buf.ResetLength(recv_len);
            //見つからない
            ptr = atoi(buf.String+(buf.Pos(" ")+1));
            // 受信データありならば(ファイル有り）、データを解析する。
            if( 200 <= ptr && ptr < 300){
                access_flag = true;
            }
        }
        sClose( server_socket );
    }
    return access_flag;
}
//---------------------------------------------------------------------------
//目標のサイズ取得
int wString::HTTPSize(wString& url)
{
    
    int         recv_len;                       //読み取り長さ
    wString     buf;
    int         work1;
    int         work2;
    int         work3;
    wString     host;
    wString     target;
    int         content_length=-1;
    SOCKET      server_socket;                          //サーバーソケット
    int         server_port = HTTP_SERVER_PORT;
    //出力ファイルの設定
    // ================
    // 実体転送開始
    // ================
    //準備
    //アドレスから、ホスト名とターゲットを取得
    work2 = url.Pos("://" )+3;
    work1 = url.Pos("/",work2 );
    work3 = url.Pos(":",work1);
    target= url.SubString(work1, url.Length()-work1);
    if( work3 >= 0 ){
        host = url.SubString(work2,work3-work2);
        server_port = atoi(url.c_str()+work3+1);
    }else{
        host = url.SubString(work2,work1-work2);
    }
    //strcpy( target, work1);
    //*work1 = 0;
    //strcpy( host, work2 );
    
    //ソケット作成と接続
    server_socket = sock_connect(host.c_str(), server_port);
    if ( ! SERROR( server_socket )){
        //HTTP1.0 GET発行
        buf.sprintf( "HEAD %s HTTP/1.0\r\n"
        "Accept: */*\r\n"
        "User-Agent: %s%s\r\n"
        "Host: %s\r\n"
        //                       "%s",
        "Connection: close\r\n\r\n" ,
        target.uri_encode() ,
        //"Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
        USERAGENT,
        MACADDR,
        host.c_str()
        //                        GetAuthorization(void),
        );
        //サーバに繋がった
        if( send( server_socket, buf.c_str(), buf.Length() , 0) != SOCKET_ERROR ){
            //初回分からヘッダを削除
            buf.SetLength(HTTP_STR_BUF_SIZE);
            recv_len  = recv(server_socket, buf.c_str(), buf.Total()-1, 0);
            //\r\n\r\nを探す
            buf.ResetLength(recv_len);      //糸止め
            work1 = atoi(buf.String+(buf.Pos(" ")+1));
            int pos = buf.Pos("Content-Length:" );
            if( pos>= 0 ){
                if( 200 <= work1 && work1 < 300 ){
                    //コンテンツ長さ TODO Content-Lengthがない場合
                    content_length = atoi(buf.c_str()+pos+16);
                }
            }else if ( work1 == 302 ){
                //Location:----\r\n
                work1 = buf.Pos("Location:");
                if( work1 ){
                    int num = buf.Pos("\r\n",work1)-work1-10;
                    buf = buf.SubString(work1+10,num);
                    content_length = HTTPSize(buf);
                }
            }
        }
        sClose(server_socket);
    }
    //終了
    return content_length;
}
//---------------------------------------------------------------------------
// linux用ファイル名に変換
//---------------------------------------------------------------------------
char* wString::WindowsFileName(char* FileName)
{
    #ifdef linux
    return FileName;
    #else
    char* work=FileName;
    int ptr=0;
    while( work[ptr] ){
        if( work[ptr] == '/' ){
            work[ptr] = '\\';
        }
        ptr++;
    }
    return work;
    #endif
}
//---------------------------------------------------------------------------
// Linux用ファイル名に変換
//---------------------------------------------------------------------------
char* wString::LinuxFileName(char* FileName)
{
    #ifdef linux
    return FileName;
    #else
    char* work = FileName;
    int ptr=0;
    while( work[ptr] ){
        if( work[ptr] == '\\' ){
            work[ptr] = '/';
        }
        ptr++;
    }
    return work;
    #endif
}
//---------------------------------------------------------------------------
// Linux用ファイル名に変換
//---------------------------------------------------------------------------
char* wString::GetLocalAddress(void)
{
    #ifdef linux
    return (char*)"neon.cx";
    #else
    //WSADATA wsadata;
    //if (WSAStartup(MAKEWORD(1, 1), &wsadata) != 0) {
    //    return NULL;
    //}
    
    //ホスト名を取得する
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return 0;
    }
    //puts(hostname);
    
    //ホスト名からIPアドレスを取得する
    HOSTENT* hostend = gethostbyname(hostname);
    if (hostend == NULL) {
        return 0;
    }
    IN_ADDR inaddr;
    memcpy(&inaddr, hostend->h_addr_list[0], 4);
    static char ip[256];
    strcpy(ip, inet_ntoa(inaddr));
    return ip;
    #endif
}
//---------------------------------------------------------------------------
size_t wString::copy( char *str, size_t slen, size_t index ) const
{
    strncpy( str, String+index,slen);
    str[slen] = 0;
    return slen;
}
wString& wString::replace( size_t index, size_t slen, const wString& repstr)
{
    size_t rlen = repstr.len;
    //同じ
    if( slen == rlen ){
        memcpy( (void*)(String+index), (void*)repstr.String, rlen );
        //前詰め置換そのままコピーすればいい
    }else if( slen > rlen ){
        size_t num = slen-rlen;
        char* p = String+index;
        char* q = p+num;
        while( *q ){
            *p++ = *q++;
        }
        memcpy( (void*)(String+index),(void*)(repstr.String),rlen);
        len -= num;
        String[len] = 0;
        //置換文字が長いので後詰めする
    }else{
        size_t num = rlen-slen;
        myrealloc( len+num+1 );
        for( char* p = (char*)(String+len+num) ; p > String+index+num ; p-- ){
            *p = *(p-num);
        }
        memcpy( (void*)(String+index),(void*)(repstr.String),rlen);
        len += num;
        String[len] = 0;
    }
    return *this;
}
