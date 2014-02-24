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
#include "wizd.h"
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
    total = 1;
    String = (char*)new char[1];
}
//---------------------------------------------------------------------------
//文字列コンストラクタ
wString::wString(const char *str)
{
    //初期化
    len = strlen(str);
    total = len+1;
    String = (char*)new char[total];
    strcpy(String, str);
}
//---------------------------------------------------------------------------
//コピーコンストラクタ
wString::wString(const wString& str)
{
    //初期化
    len   = str.len;
    total = str.total;
    String = new char[str.total];
    memcpy(String,str.String,total);
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
    memcpy( src->String, dst->String,dst->total);
    src->len = dst->len;
}
//---------------------------------------------------------------------------
//ディープコピーメソッド
void wString::copy(wString* src,const wString& dst)
{
    src->myrealloc(dst.total);
    memcpy( src->String, dst.String,dst.total);
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
    temp->myrealloc(newLen+1);
    strcpy(temp->String, str1);
    strcat(temp->String, str2.String);
    temp->len = newLen;
    return *temp;
}
//---------------------------------------------------------------------------
void wString::operator+=(const wString& str)
{
    unsigned int newLen = len+str.len;
    myrealloc(newLen+1);
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
    myrealloc(newLen+1);
    strcpy( String+len, str );
    len = newLen;
    return;
}
//---------------------------------------------------------------------------
void wString::operator+=(char ch)
{
    myrealloc(len+16);
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
    myrealloc(str.total);
    memcpy( String, str.String,str.total );
    len = str.len;
    return;
}
//---------------------------------------------------------------------------
void wString::operator=(const char* str)
{
    int newLen = strlen(str);
    myrealloc(newLen+1);
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
    myrealloc(num);
    return;
}
//---------------------------------------------------------------------------
// 比較
//---------------------------------------------------------------------------
int wString::compare(const wString& str) const
{
    size_t minlen = (len>str.len)?str.len+1:len+1;
    return memcmp( String, str.String, minlen );    
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
    wString* temp = NextSac();
    if( mylen>0){
        temp->SetLength( mylen+1 );
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
    int newLen = len-index;
    if( newLen > 0 ){
        temp->myrealloc(newLen+1);
        memcpy(temp->String, String+index,newLen);
        temp->String[newLen] =0;
        temp->len = newLen;
    }
    return *temp;
}
//---------------------------------------------------------------------------
// substr
//---------------------------------------------------------------------------
wString& wString::substr(int index, int length) const
{
    wString* temp = NextSac();
    int newLen = length;
    if( newLen > (int)len-index ){
        newLen = len-index;
    }
    if( newLen>0){
        temp->myrealloc(newLen+1);
        memcpy(temp->String,String+index,newLen);
        temp->String[newLen]=0;
        temp->len = newLen;
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
    //copy(tmp,str.SubString(0,pos+1));
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
    struct dirent **namelist;
    int n;
    wString* ret;
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
void wString::myrealloc(int newsize)
{
    if( len>=total){
       printf( "not good %d %d",len,total );
       exit( 1);
    }
    if( (int)total<newsize){
        total = newsize;
        char* tmp = new char[total];
        memcpy(tmp,String,len);
        tmp[len]=0;
        delete[] String;
        String = tmp;
    }else{
        //指定サイズが元より小さいので何もしない
    }
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
            code  = htoc(String[++i])<<4;
            code += htoc(String[++i]);
            if( flag && code == '\"' ){
                dst += '\\';
            }
            dst += code;
        }else if ( String[i] == '+' ){
            dst += ' ';
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
void wString::headerPrint(int socket,int endflag)
{
    //printf( "%s", String );
    write( socket, String, len );
    if( endflag ){
        write( socket, HTTP_END, strlen( HTTP_END));
        //printf( "%s", HTTP_END );
    }
    //debug_log_output( "%s", String );
    //debug_log_output( "%s %d", HTTP_END, strlen( HTTP_END));
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
    int count = 0;
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
    if (mp3_id3v2_tag_read(mp3_filename) != 0) {
        if( mp3_id3v1_tag_read(mp3_filename) != 0 ){
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
    return true;
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
    char      *out;
    out = new char[len*3];
    memset(nkf_option, '\0', sizeof(nkf_option));
    //=====================================================================
    // in_flag, out_flagをみて、libnkfへのオプションを組み立てる。
    //=====================================================================
    switch( in_flag )
    {
    case CODE_SJIS:
        strncpy(nkf_option, "S", sizeof(nkf_option));
        break;
    case CODE_EUC:
        strncpy(nkf_option, "E", sizeof(nkf_option));
        break;
    case CODE_UTF8:
        strncpy(nkf_option, "W", sizeof(nkf_option));
        break;
    case CODE_UTF16:
        strncpy(nkf_option, "W16", sizeof(nkf_option));
        break;
    case CODE_AUTO:
    default:
        strncpy(nkf_option, "", sizeof(nkf_option));
        break;
    }
    switch( out_flag )
    {
    case CODE_EUC:
        strncat(nkf_option, "e", sizeof(nkf_option) - strlen(nkf_option) );
        break;
    case CODE_SJIS:
        strncat(nkf_option, "s", sizeof(nkf_option) - strlen(nkf_option) );
        break;
    case CODE_UTF8:
    default:
        strncat(nkf_option, "w", sizeof(nkf_option) - strlen(nkf_option) );
        break;
    }
    //=================================================
    // libnkf 実行
    //=================================================
    nkf((char*)in, (char*)out, len, (char*)nkf_option);
    strcpy( in, out );
    delete []out;
    //memcpy((char*)in,(char*)out,len);
    return;
}

