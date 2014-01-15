#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
//#include <iostream>
//#include <sstream>
#ifdef linux
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

//---------------------------------------------------------------------------
//source
//---------------------------------------------------------------------------
wString* wString::sac[MAXSAC];
int wString::sacPtr=0;
size_t wString::npos=(size_t)(-1);
//---------------------------------------------------------------------------
int  wString::wStringInit(void)
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
    total = 0;
    String = NULL;
}
//---------------------------------------------------------------------------
//文字列コンストラクタ
wString::wString(const char *str)
{
    //初期化
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
    if( str.len ){
        total = str.total;
        String = new char[str.total];
        strcpy(String,str.String);
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
//    for( int i = 0 ; i < MAXSAC ; i++ ){
//        if( String && String == sac[i]->String ){
//            int a = 1;
//            break;
//        }
//    }
    if( String ){
        len = 0;
        delete [] String;
        String = NULL;
    }
}
//---------------------------------------------------------------------------
//ディープコピーメソッド
void wString::copy(wString* src,const wString* dst)
{
    //コピー元に
    if( dst->len ){
        if( src->total <= dst->len ){
            //realloc処理
            char* tmp = myrealloc(src->String,dst->len+1);
            //printf("assert\n");
            assert(tmp != 0 );
            src->String = tmp;
            src->total = dst->len+1;
        }
        //ここで両者とも!= NULL
            //printf("assert\n");
        assert( src->String != 0);
        assert( dst->String != 0);
        strcpy( src->String, dst->String);
    //dstはNULL
    }else{
        if( src->len ){
            *src->String = 0;
        }
    }
    src->len = dst->len;
}
//---------------------------------------------------------------------------
//ディープコピーメソッド
void wString::copy(wString* src,const wString& dst)
{
    if( dst.len ){
        if( src->total <= dst.len ){
            char* tmp = myrealloc(src->String,dst.len+1);
            //printf("assert\n");
            assert(tmp != 0 );
            src->String = tmp;
            src->total = dst.len+1;
        }
        //ここで両者とも!= NULL
            //printf("assert\n");
        assert( src->String != 0);
        assert( dst.String != 0);
        strcpy( src->String, dst.String);
    //dstはNULL
    }else{
        if( src->len ){
            *src->String = 0;
        }
    }
    src->len = dst.len;
}
//---------------------------------------------------------------------------
wString& wString::operator+(const wString str) const 
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
    if( temp->total<=newLen ){
        char* tmp = wString::myrealloc(temp->String,newLen+1);
        temp->String = tmp;
        temp->total = newLen+1;
    }
    strcpy(temp->String, str1);
    strcat(temp->String, str2.String);
    temp->len = newLen;
    return *temp;
}
//---------------------------------------------------------------------------
void wString::operator+=(const wString& str)
{
    unsigned int num = len+str.len;
    //
    if( str.len ){
        //領域が十分ある
        if( total >= num+1 ){
            strcpy( String+len, str.String );
            len   = num;
        //コピー元が存在し領域不十分
        }else{
            //がばっと１６バイトふやしておく
            total = num+16;
            //realloc処理
            char* tmp = myrealloc(String,total);
            //printf("assert\n");
            assert( tmp != NULL );
            strcpy( tmp+len, str.String );
            len    = num;
            String = tmp;
        }
    }
    return;
}
//---------------------------------------------------------------------------
void wString::operator+=(const char* str)
{
    unsigned int snum = strlen(str);
    unsigned int num = len+snum;
    if( snum ){
        if( total >= num+1 ){
            strcpy( String+len, str );
            len   = num;
        }else{
            //がばっと１６バイト増やしておく
            total = num+16;
            //realloc処理
            char* tmp = myrealloc(String,total);
            //printf("assert\n");
            assert( tmp != NULL );
            strcpy( tmp+len, str );
            len    = num;
            String = tmp;
        }
    }
    return;
}
//---------------------------------------------------------------------------
void wString::operator+=(char ch)
{
    unsigned int num = len+1;
    //
    //領域が十分ある
    if( total > num ){
        String[len] = ch;
        String[len+1] = 0;
        len   = num;
    //コピー元が存在し領域不十分
    }else{
        //がばっと１６バイト増やしておく
        total = num+16;
        //realloc処理
        char* tmp = myrealloc(String,total);
        assert( tmp != NULL );
        tmp[len] = ch;
        tmp[len+1] = 0;
        len    = num;
        String = tmp;
    }
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
//    printf( "1--%s %d %d\n", String, (int)len, (int)total );
//    printf( "2--%s %d %d\n", str.String, (int)str.len, (int)str.total );
    //コピー元文字列の長さがコピー先文字列より短い時
    if( total >= str.len+1 ){
        len   = str.len;
        if( str.String ){
            memcpy( String, str.String, str.len+1 );
        }else{
            *String = 0;
        }
    //コピー元文字列の長さがコピー先文字列より長い時
    }else{
        //コピー元が存在するとき
        if( str.String ){
            len   = str.len;
            total = len+1;
            char* tmp = myrealloc(String,total,0);
            assert( tmp != NULL );
            String = tmp;
            memcpy(String, str.String, str.len+1 );
        //存在しない時
        }else{
            //自分も領域を持つならば消す
            if( len ){
                *String = 0;
            }
            len = 0;
        }
    }
    return;
}
//---------------------------------------------------------------------------
void wString::operator=(const char* str)
{
    //コピー元文字列の長さがコピー先文字列より短い時
    unsigned int num = str?strlen(str):0;
    if( total > num ){
        len   = num;
        strncpy( String, str, num );
        String[num] = 0;
    //コピー元文字列の長さがコピー先文字列より長い時
    }else{
        if( num ){
            len   = num;
            total = len+1;
            char* tmp = myrealloc( (char*)String, total,0);
            assert( tmp != NULL );
            strncpy( tmp, str, num );
            String = tmp;
            String[len] = 0;
        }else{
            //自分も領域を持つならば消す
            if( len ){
                *String = 0;
            }
            len = 0;
        }
    }
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
     if( 0<=index && index < len ){
          return String[index];
     }else{
          perror( "out bound");
          return -1;
     }
} 
//---------------------------------------------------------------------------
void wString::SetLength(unsigned int num)
{
    //文字列領域の余裕がある、又は共にサイズ０
    if( total > num ){
        len   = num;
        String[len] = 0;
    //領域を拡大する場合
    }else{
        //len   = num;
        total = num+1;
        char* tmp = myrealloc( String, total );
        //debug_log_output("String=[%p]\n", String );
        //致命的エラー
        assert( tmp != NULL );
        String = tmp;
        //String[len] = 0;
    }
    return;
}
//---------------------------------------------------------------------------
// 比較
//---------------------------------------------------------------------------
int wString::compare(const wString& str) const
{
    return strcmp( String, str.String );    
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
wString&  wString::SubString(int start, int mylen)
{
    if( mylen == 0 ){
        return *NextSac();
    }
    wString* temp = NextSac();
    temp->SetLength( mylen+1 );
    strncpy( temp->String, String+start,mylen);
    temp->String[mylen] = 0;
    //長さ不定。数えなおす
    temp->len = strlen(temp->String);
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
void wString::LoadFromFile(const char* FileName)
{
    long flen;
    int  handle;
    handle  = open(FileName,O_RDONLY | S_IREAD );
    flen = lseek(handle,0,SEEK_END);
           lseek(handle,0,SEEK_SET);
    SetLength(flen+1);
    len = read( handle, String, flen);
    close(handle);
    String[len] = 0;
    //\0がある場合を考えればstrlenとってはいけない
    //len = strlen(String);
}
//---------------------------------------------------------------------------
// ファイル書き込み
//---------------------------------------------------------------------------
int  wString::SaveToFile(wString& str)
{
    return SaveToFile(str.String);
}
//---------------------------------------------------------------------------
// ファイル書き込み
//---------------------------------------------------------------------------
int wString::SaveToFile(const char* FileName)
{
    int handle = open(FileName,O_CREAT| O_TRUNC | O_RDWR, S_IREAD| S_IWRITE);
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
        while( *temp->String && *temp->String < ' ' ){
            char* ptr=temp->String;
            while( *(ptr+1) ){
                *ptr = *(ptr+1);
                 ptr++;
            }
            temp->len--;
        }
        //末尾の空白等を抜く
        while( temp->len && String[temp->len-1] < ' ' ){
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
        while( temp->len && String[temp->len-1] < ' ' ){
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
        while( *temp->String && *temp->String < ' ' ){
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
    unsigned int newLen = len-index;
//    printf("newLen:len:%d index:%d newLen:%d", len, index, newLen );
    if( newLen>0 ){
        if( temp->total <= newLen ){
            temp->String = myrealloc(temp->String,newLen+1,0);
            temp->total = newLen+1;
        }
        temp->len = newLen;
        strncpy(temp->String,String+index,newLen);
        temp->String[newLen] = 0;     
    }else{
    }
    return *temp;
}
//---------------------------------------------------------------------------
// substr
//---------------------------------------------------------------------------
wString& wString::substr(int index, int length) const
{
    wString* temp = NextSac();
    unsigned int newLen = length;

    if( newLen > len-index ){
        newLen = len-index;
    }
//    printf("newLen:len:%d index:%d newLen:%d", len, index, newLen );
    if( newLen>0 ){
        if( temp->total <= newLen ){
            temp->String = myrealloc(temp->String,newLen+1,0);
            temp->total = newLen+1;
        }
        strncpy(temp->String,String+index,newLen);
        temp->String[newLen] = 0;
        temp->len = newLen;
    }else{
    }
    return *temp;
}
//--------------------------------------------------------------------
wString& wString::FileStats(const char* str)
{
    struct stat      stat_buf;
    wString* buf=NextSac();
    if (stat(str, &stat_buf) == 0) {
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
    }
    return *buf;
}
//---------------------------------------------------------------------------
wString& wString::FileStats(wString& str)
{
    return FileStats(str.String);
}

//---------------------------------------------------------------------------
bool wString::FileExists(char* str)
{
    bool flag=false;
#ifdef linux
    struct stat send_filestat;
    int  result = stat(str, &send_filestat);
    if ( ( result == 0 ) && ( S_ISREG(send_filestat.st_mode) == 1 ) ){
        flag = true;
    }
#else
    struct ffblk send_filestat;
    int result = findfirst(str,&send_filestat, 0);
    if( result == 0 && ( send_filestat.ff_attrib & FA_DIREC ) == 0){
        flag = true;
    }
#endif
    return flag;
}
//---------------------------------------------------------------------------
bool wString::FileExists(wString& str)
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
bool wString::CreateDir(wString& str)
{
    bool flag;
#ifdef linux
    //0x777ではちゃんとフォルダできない
    flag = (mkdir( str.String,0777 ) != -1 );
#else
    flag = (mkdir( str.String ) != -1 );
#endif
    return flag;
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
int wString::LastDelimiter(const char* delim)
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
    handle = open(str,0);
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
wString& wString::ExtractFileName(char* str, const char* delim)
{
    wString* tmp;
    tmp = NextSac();
    *tmp = str;
    return ExtractFileName(*tmp, delim);
}
//---------------------------------------------------------------------------
wString& wString::ExtractFileName(wString& str, const char* delim)
{
    int pos = str.LastDelimiter(delim);
    wString* tmp = NextSac();
    copy(tmp,str.SubString(pos+1,str.Length()-pos+1));
    return *tmp;
}
//---------------------------------------------------------------------------
wString& wString::ExtractFileExt(wString& str)
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
bool wString::DeleteFile(wString& str)
{
    bool flag;
    flag = (unlink(str.String)==0);
    return flag;
}
//---------------------------------------------------------------------------
bool wString::DirectoryExists(char* str)
{
    bool flag=false;
#ifdef linux
    struct stat send_filestat;
    int  result = stat(str, &send_filestat);
    if ( ( result == 0 ) && ( S_ISDIR(send_filestat.st_mode) == 1 ) ){
        flag = true;
    }
#else
    struct ffblk send_filestat;
    int result = findfirst(str,&send_filestat, FA_DIREC );
    if( result == 0 ){
        flag = true;
    }
#endif
    return flag;
}
//---------------------------------------------------------------------------
bool wString::DirectoryExists(wString& str)
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
void wString::replace_character_len(const char *sentence,
                                    int slen,
                                    const char* p,
                                    int klen,
                                    const char *rep)
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
wString& wString::EnumFolder(wString& Path)
{
    DIR *dir;
    struct dirent *ent;
    wString* ret;
    wString temp;
    wString Path2;
    Path2 = Path;
    int first=1;
    //Directoryオープン
    if ((dir = opendir(Path.String)) != NULL){
        //ファイルリスト
        temp = "[";
        while ((ent = readdir(dir)) != NULL){
		if( first ){
			first = 0;
		}else{
			temp += ",";
		}
                temp += "\""+Path2+DELIMITER+ent->d_name+"\"" ;
        }
        temp += "]";
        closedir(dir);
    }else{
        temp = "";
    }
    ret = NextSac();
    *ret = temp;
    return *ret;
}
//---------------------------------------------------------------------------
//wString 可変引数
int wString::sprintf(const char* format, ... )
{
   int stat;
   //可変引数を２つ作る
   va_list ap1,ap2;
   va_start(ap1, format);
   va_copy (ap2, ap1);
   //最初はダミーで文字列長をシミュレート
   stat = vsnprintf(String, 0, format, ap1);
   SetLength( stat );
   //実際に出力
   stat = vsprintf(String, format, ap2);
   va_end(ap1);
   va_end(ap2);
   len = stat;
   return stat;
}
//---------------------------------------------------------------------------
//wString 可変引数
int wString::cat_sprintf(const char* format, ... )
{
   int stat;
   //可変引数を２つ作る
   va_list ap1,ap2;
   va_start(ap1, format);
   va_copy (ap2, ap1);
   //最初はダミーで文字列長をシミュレート
   stat = vsnprintf(String, 0, format, ap1);
   SetLength( stat+len );
   //実際に出力
   stat = vsprintf(String+len, format, ap2);
   va_end(ap1);
   va_end(ap2);
   len += stat;
   return stat;
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
inline char* wString::myrealloc(char* ptr, size_t size,int dispose)
{
    char* tmp = new char[size];
    assert( size != 0 );
            //printf("assert\n");
    assert( tmp != NULL );
    if( ptr ){
        if( ! dispose ){
            memcpy( tmp, ptr, size );
        }
        delete [] ptr;
    }else{
        *tmp = 0;
    }
    return tmp;
}
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
        }else if ( String[is] == '\'' ){
            *temp += "&#39;";
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
    int cnt;
    // 引数チェック
    if(len == 0 ){
        //０バイト
        return (char*)"";
    }
    cnt = 0;
    for ( is = 0 ; is < len ; is++){
        /* ' '(space) はちと特別扱いにしないとまずい */
        if ( String[is] == ' ' ){
            dst += "%20";
        /* エンコードしない文字全員集合 */
//        }else if ( strchr("!$()*,-./:;?@[]^_`{}~", String[is]) != NULL ){
        }else if ( strchr("!$()*,-.:;?@[]^_`{}~", String[is]) != NULL ){
            dst += String[is];
        /* アルファベットと数字はエンコードせずそのまま */
        }else if ( isalnum( String[is] ) ){
            dst += String[is];
        }
        /* \マークはエンコード */
        else if ( String[is] == '\\' ){
            dst += "%5C";
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
    FILE *fpr;
    FILE *fpw;
    unsigned char buf[8000];
    int size;

    fpr = fopen( fname_r, "rb" );
    if( fpr == NULL ){
        return -1;
    }
    fpw = fopen( fname_w, "wb" );
    if( fpw == NULL ){
        fclose( fpr );
        return -1;
    }
    while( 1 ){
        size = fread( buf, sizeof( unsigned char ), sizeof(buf), fpr );
        if( size <= 0 ){
            break;
        }
        fwrite( buf, sizeof( unsigned char ), size, fpw );
    }
    fclose( fpr );
    fclose( fpw );

    return 0;
}
char* wString::strencode(void)
{
    unsigned int is;
    wString dst;
    for ( is = 0 ; is < len ; is++){
        if ( String[is] == '\"' ){
            dst += "\\\"";
        }else{
            dst += String[is];
        }
    }
    wString* temp = NextSac();
    *temp = dst;
    return temp->String;
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
//#define htoc(x) (('0'<= (x) && (x)<='9') ? ((x)-'0') : (('a'<= (x) && (x) <= 'f') ? ((x)-'a'+10) : (('A' <= (x) && (x) <= 'F') ? ((x)-'A'+10) : 0)))
inline int htoc(unsigned char x)
{
    if( '0'<=x && x<='9') return x-'0';
    if( 'a'<=x && x<='f') return x-'a'+10;
    if( 'A'<=x && x<='F') return x-'A'+10;
    return 0;
}
char* wString::uri_decode(int flag)
{
    size_t          i;
    int             cnt;
    unsigned char   code;
    // 引数チェック
    if (len==0){
        return String;
    }
    wString dst;
    cnt = 0;
    // =================
    // メインループ
    // =================
    for (i = 0; i < len  && String[i] != '\0'; i++){
        if (String[i] == '%'){
            if (i + 2 >= len){
                break;
            }
            code  = htoc(String[++i])<<4;
            code += htoc(String[++i]);
            if( flag && code == '\"' ){
                dst += '\\';
            }
            dst += code;
        }else{
            if( flag && String[i] == '\"' ){
                dst += '\\';
            }
            dst += String[i];
        }
    }
    wString* temp = NextSac();
    *temp = dst;
    return temp->String;
}

void wString::headerInit(size_t content_length, int expire, const char* mime_type)
{
    cat_sprintf( "%s", HTTP_OK );
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
void wString::headerPrint(int endflag)
{
    //printf( "%s", String );
    write( STDOUT_FILENO, String, len );
    if( endflag ){
        write( STDOUT_FILENO, HTTP_END, strlen( HTTP_END));
        //printf( "%s", HTTP_END );
    }
    debug_log_output( "%s", String );
    debug_log_output( "%s %d", HTTP_END, strlen( HTTP_END));
    //fflush( stdout );
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
             strncpy( head, str,5);
             head[5]=0;
             strcpy( body, str+5);
        }else{
            for( size_t i = 0 ; i < strlen( str ) ; i++ ){
                if( str[i] == ' ' ){
                     strncpy( head, str, i+1 );
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
                int count = this->GetListLength();
                wString str2;
                str2.sprintf( "%s%s", head, body );
                for( int i = 0 ; i < count ; i++ ){
                     wString tmp = this->GetListString(i);
                     if( strncmp(tmp.c_str(), head, strlen(head)) == 0 ){
                         this->SetListString( str2.c_str(), i );
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
void wString::Add(const char* str)
{
     *this += str;
     *this += "\r\n";
     return;
}
void wString::Add(wString& str)
{
     *this += str;
     *this += "\r\n";
     return;
}
int wString::GetListLength(void)
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
//---------------------------------------------------------------------------
//　TStringList対策
//  行数を返す
//　リエントラントでないことに注意
//　戻り値は使い捨て
wString& wString::GetListString(int pos)
{
    int   ccount = 0;
    int   ptr =0;
    int   end=0;
    int   start;
    int   inc=0;
    while( String[ptr] ){
        if( String[ptr] == '\r' ){
            ptr++;
            inc++;
            continue;
        }
        if( String[ptr++] == '\n' ){
            start = end;
            end = ptr;
            inc++;
            if( ccount++ == pos ){
                wString* work = NextSac();
                copy(work,SubString(start,end-start-inc));
                return *work;
            }
        }
        inc = 0;
    }
    //０バイト
    return *NextSac();
}
//---------------------------------------------------------------------------
//　TStringList対策
//  行数を返す
//　リエントラントでないことに注意
//　戻り値は使い捨て
int  wString::SetListString(const char* dst, int pos)
{
    int   ccount = 0;
    int   ptr =0;
    int   end=0;
    int   start;
    int   inc=0;
    while( String[ptr] ){
        if( String[ptr] == '\r' ){
            ptr++;
            inc++;
            continue;
        }
        if( String[ptr++] == '\n' ){
            start = end;
            end = ptr;
            inc++;
            if( ccount++ == pos ){
                SetLength(len-(end-start-inc)+strlen(dst));
                replace_character_len(String,
                                      len,
                                      String+start,
                                      end-start-inc,
                                      dst);
                len = strlen(String);
                return 0;
            }
        }
        inc = 0;
    }
    //０バイト
    return 1;
}
