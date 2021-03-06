// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_tools.c
//											$Revision: 1.14 $
//											$Date: 2004/07/04 06:40:52 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#include  <ctype.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <string.h>
#include  <limits.h>
#include  <errno.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <time.h>

#ifdef linux
#include  <fcntl.h>
#include  <unistd.h>
#include  <sys/time.h>
#include  <sys/socket.h>
#include  <sys/un.h>
#include  <netdb.h>
#include  <pthread.h>
#else
#include  <errno.h>
#include  <io.h>
#include  <process.h>
#endif
#include  "wizd.h"
#include  "const.h"
#include  "wizd_tools.h"

static char	debug_log_filename[FILENAME_MAX];	// デバッグログ出力ファイル名(フルパス)
static char	debug_log_initialize_flag  = (1);	// デバッグログ初期化フラグ
static void cut_before_n_length(char *sentence,  unsigned int n);
static void cut_after_n_length(char *sentence,  unsigned int n);
/********************************************************************************/
// sentence文字列内のkey文字列をrep文字列で置換する。
/********************************************************************************/
void replace_character(char *sentence, const char *key, const char *rep)
{
    #if 0
    int sentence_buf_size = 4096;
    int malloc_size;
    char       *p, *buf;
    if ( strlen(key) == 0 ){
        return;
    }
    malloc_size = strlen(sentence) * 4;
    buf = mymalloc(malloc_size);
    if ( buf == NULL )
    return;
    p = strstr(sentence, key);
    while (p != NULL){
        *p = '\0';
        strncpy(buf, p+strlen(key), malloc_size );
        strncat(sentence, rep, sentence_buf_size - strlen(sentence) );
        strncat(sentence, buf, sentence_buf_size - strlen(sentence) );
        p = strstr(p+strlen(rep), key);
    }
    myfree(buf);
    return;
    
    #else
    char* p;
    char* q;
    char* str;
    int klen=strlen(key);
    int rlen=strlen(rep);
    int slen=strlen(sentence);
    int num;
    if ( klen == 0 || slen == 0){
        return;
    }
    p = strstr(sentence, key);
    if( klen == rlen ){
        while (p != NULL){
            memcpy( (char*)p,(char*)rep,rlen);
            p = strstr(p+rlen, key);
        }
        //前詰め置換そのままコピーすればいい
    }else if( klen > rlen ){
        num = klen-rlen;
        while (p != NULL){
            q = p;
            while( 1 ){
                *(char*)q = *(char*)(q+num);
                if( *(char*)(q+num) == 0 ){
                    break;
                }
                q++;
            }
            memcpy( p,rep,rlen);
            p = strstr(p+rlen, key);
            //slen -= num;
        }
        //置換文字が長いので後詰めする
    }else{
        while (p != NULL){
            num = rlen-klen;
            //pからrlen-klenだけのばす
            for( str = sentence+slen+num ; str-num >= p ; str-- ){
                *str = *(str-num);
            }
            memcpy( p,rep,rlen);
            p = strstr(p+rlen, key);
            slen += num;
        }
    }
    //myfree(buf);
    return;
    #endif
}
/********************************************************************************/
// sentence文字列内の最初のkey文字列をrep文字列で置換する。
/********************************************************************************/
void replace_character_first(char *sentence,const char *key, const char *rep)
{
    char* p;
    char* str;
    int klen=strlen(key);
    int rlen=strlen(rep);
    int slen=strlen(sentence);
    int num;
    if ( klen == 0 || slen == 0){
        return;
    }
    p = strstr(sentence, key);
    if( klen == rlen ){
        memcpy( p,rep,rlen);
        //前詰め置換そのままコピーすればいい
    }else if( klen > rlen ){
        num = klen-rlen;
        strcpy( p,(p+num));
        memcpy( p,rep,rlen);
        //置換文字が長いので後詰めする
    }else{
        num = rlen-klen;
        //pからrlen-klenだけのばす
        for( str = sentence+slen+num ; str-num >= p ; str-- ){
            *str = *(str-num);
        }
        memcpy( p,rep,rlen);
    }
    return;
}
//***************************************************************************
// sentence文字列より、cut_charから後ろを削除
//      見つからなければ何もしない。
// 入力  : char* sentence 入力文字列
//         char  cut_char 切り取り文字
// 戻り値: char* 切り取った後ろの文字列
//***************************************************************************
char*   cut_after_character(char *sentence, char cut_char)
{
    char       *symbol_p;
    // 削除対象キャラクターがあった場合、それから後ろを削除。
    symbol_p = strchr(sentence, cut_char);
    if (symbol_p != NULL){
        *symbol_p++ = '\0';
    }
    return symbol_p;
}
//***************************************************************************
// sentence文字列の、cut_charが最初に出てきた所から前を削除
// もし、cut_charがsentence文字列に入っていなかった場合、文字列全部削除
//***************************************************************************
void    cut_before_character(char *sentence, char cut_char)
{
    #if 0
    char* p;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // 削除対象キャラクターが最初に出てくる所を探す。
    p = strchr(sentence, cut_char);
    if( p ){
        // 削除対象キャラクターの後ろから最後までの文字列をコピー
        strcpy(sentence,++p);
        return;
    }else{
        //ない場合全部削除
        *sentence = 0;
    }
    return;
    #else
    char       *symbol_p;
    char       *malloc_p;
    int                 sentence_len;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    sentence_len = strlen(sentence);
    // 削除対象キャラクターが最初に出てくる所を探す。
    symbol_p = strchr(sentence, cut_char);
    if (symbol_p == NULL){
        // 発見できなかった場合、文字列全部削除。
        strncpy(sentence, "", sentence_len);
        return;
    }
    symbol_p++;
    // テンポラリエリアmalloc.
    malloc_p = (char*)malloc(sentence_len + 10);
    if (malloc_p == NULL){
        return;
    }
    // 削除対象キャラクターの後ろから最後までの文字列をコピー
    strncpy(malloc_p, symbol_p, sentence_len + 10);
    // sentence書き換え
    strncpy(sentence, malloc_p, sentence_len);
    free(malloc_p);
    return;
    #endif
}
//************************************************************************
// sentence文字列の、cut_charが最後に出てきた所から前を削除
// もし、cut_charがsentence文字列に入っていなかった場合、なにもしない。
//************************************************************************
void    cut_before_last_character(char *sentence, char cut_char)
{
    #if 1
    char* p;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // 削除対象キャラクターが最後に出てくる所を探す。
    p = strrchr(sentence, cut_char);
    if( p++ ){
        // 削除対象キャラクターの後ろから最後までの文字列をコピー
        while( *p ){
            *sentence++=*p++;
        }
        *sentence = 0;
    }
    #else
    char       *symbol_p;
    char       *malloc_p;
    int                         sentence_len;
    if (sentence == NULL ||*sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // 削除対象キャラクターが最後に出てくる所を探す。
    symbol_p = strrchr(sentence, cut_char);
    if (symbol_p == NULL){
        // 発見できなかった場合、なにもしない。
        return;
    }
    symbol_p++;
    // テンポラリエリアmalloc.
    malloc_p = (char*)mymalloc(sentence_len + 10);
    if (malloc_p == NULL){
        return;
    }
    // 削除対象キャラクターの後ろから最後までの文字列をコピー
    strncpy(malloc_p, symbol_p, sentence_len + 10);
    // sentence書き換え
    strncpy(sentence, malloc_p, sentence_len);
    myfree(malloc_p);
    return;
    #endif
}
//************************************************************************
// sentence文字列の、cut_charが最後に出てきた所から後ろをCUT
// もし、cut_charがsentence文字列に入っていなかった場合、文字列全部削除。
//************************************************************************
void 	cut_after_last_character(char *sentence, char cut_char)
{
    char       *symbol_p;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // 削除対象キャラクターが最後に出てくる所を探す。
    symbol_p = strrchr(sentence, cut_char);
    if (symbol_p == NULL){
        // 発見できなかった場合、文字列全部削除。
        *sentence = '\0';
        return;
    }
    *symbol_p = '\0';
}
//******************************************************************
// sentenceの、後ろ n byteを残して削除。
//******************************************************************
void    cut_before_n_length(char *sentence,  unsigned int n)
{
#if 0
    unsigned int len;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    len = strlen(sentence);
    // sentence が、nよりも同じか短いならばreturn
    if (len <= n ){
        return;
    }
    strncpy( sentence, (sentence+len-n),n+1);
#else
    char       *malloc_p;
    char       *work_p;
    unsigned int        sentence_len;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    sentence_len = strlen(sentence);
    // sentence が、nよりも同じか短いならばreturn
    if ( sentence_len <= n ){
        return;
    }
    // テンポラリエリアmalloc.
    malloc_p = (char*)malloc(sentence_len + 10);
    if (malloc_p == NULL){
        return;
    }
    work_p = sentence;
    work_p += sentence_len;
    work_p -= n;
    strncpy(malloc_p, work_p, sentence_len + 10);
    strncpy(sentence, malloc_p, sentence_len);
    free(malloc_p);
    return;
#endif
}
//******************************************************************
// sentenceの、後ろ n byteを削除
//  全長がn byteに満たなかったら、文字列全部削除
//******************************************************************
void    cut_after_n_length(char *sentence,  unsigned int n)
{
    char       *work_p;
    unsigned int        sentence_len;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    sentence_len = strlen(sentence);
    // sentence が、nよりも同じか短いならば、全部削除してreturn;
    if ( sentence_len <= n ){
        *sentence = 0;
        //strncpy(sentence, "", sentence_len);
        return;
    }
    // 後ろ n byteを削除
    work_p = sentence;
    work_p += sentence_len;
    work_p -= n;
    *work_p = '\0';
    return;
}
//******************************************************************
// sentence文字列の、cut_charを抜く。
//******************************************************************
void    cut_character(char *sentence, char cut_char)
{
    #if 1
    char       *work1;
    char       *work2;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    work1 = sentence;
    work2 = sentence;
    // 処理ループ。
    while (*work1){
        // 削除対象のキャラクターがいたら、それを飛ばす。
        if (*work1 == cut_char){
            work1++;
        }else{
            *work2++ = *work1++;
        }
    }
    // '\0' をコピー。
    *work2 = '\0';
    return;
    #else
    char       *symbol_p;
    char       *malloc_p;
    char       *work_p;
    int                         sentence_len;
    if (sentence == NULL || *sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // テンポラリエリアmalloc.
    malloc_p = mymalloc(sentence_len + 10);
    if (malloc_p == NULL)
    return;
    symbol_p = sentence;
    work_p = malloc_p;
    // 処理ループ。
    while (*symbol_p != '\0'){
        // 削除対象のキャラクターがいたら、それを飛ばす。
        if (*symbol_p == cut_char){
            symbol_p++;
        }else   {// 削除対象キャラクター意外だったら、コピー。
            *work_p = *symbol_p;
            work_p++;
            symbol_p++;
        }
    }
    // '\0' をコピー。
    *work_p = *symbol_p;
    // sentence書き換え
    strncpy(sentence, malloc_p, sentence_len);
    myfree(malloc_p);
    return;
    #endif
}
//******************************************************************
// sentence文字列の、頭にcut_charがいたら、抜く。
//******************************************************************
void    cut_first_character(char *sentence, char cut_char)
{
    #if 1
    char* p = sentence;
    if (sentence == NULL || *sentence == 0 ){
        return;
    }
    // 削除対象キャラクターがあるかぎり進める。
    while ((*p == cut_char) && *p){
        p++;
    }
    // sentence書き換え
    if( p != sentence ){
        while( *p ){
            *sentence++ = *p++;
        }
        *sentence = *p;
        //string corupped bug.
        //strcpy((char*)sentence, (char*)p);
    }
    return;
    #else
    char       *malloc_p;
    char       *work_p;
    int                         sentence_len;
    if (sentence == NULL || *sentence == 0 )
    return;
    sentence_len = strlen(sentence);
    // テンポラリエリアmalloc.
    malloc_p = (char*)mymalloc(1024);//sentence_len + 10);
    if (malloc_p == NULL){
        return;
    }
    strncpy(malloc_p, sentence, sentence_len + 10);
    work_p = malloc_p;
    // 削除対象キャラクターがあるかぎり進める。
    while ((*work_p == cut_char) && (*work_p != '\0')){
        work_p++;
    }
    // sentence書き換え
    strncpy(sentence, work_p, sentence_len);
    myfree(malloc_p);
    return;
    #endif
}
// ***************************************************************************
// sentence文字列の行末に、cut_charがあったとき、削除
// ***************************************************************************
void    cut_character_at_linetail(char *sentence, char cut_char)
{
    char        *source_p;
    int         length, i;
    if (sentence == NULL || *sentence == 0)
    return;
    length = strlen(sentence);  // 文字列長Get
    source_p = sentence;
    source_p += length;         // ワークポインタを文字列の最後にセット。
    for (i=0; i<length; i++)    {// 文字列の数だけ繰り返し。
        source_p--;                     // 一文字ずつ前へ。
        if (*source_p == cut_char)      {// 削除キャラ ヒットした場合削除
            *source_p = '\0';
        }else                                           {// 違うキャラが出てきたところで終了。
            break;
        }
    }
    return;
}
/********************************************************************************/
// sentence文字列内のunique_charが連続しているところを、unique_char1文字だけにする。
/********************************************************************************/
void duplex_character_to_unique(char *sentence, char unique_char)
{
    #if 1
    char       *p1;
    char       *p2;
    int                 unique_char_count = 0;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    p1 = sentence;
    p2 = sentence;
    // sensense文字列から、unique_char以外をワークへコピー。
    while (*p1){
        // unique_char発見
        if (*p1 == unique_char){
            // 最初の一つならコピー。それ以外ならスキップ。
            if (unique_char_count == 0) {
                *p2++ = *p1++;
            }else{
                p1++;
            }
            unique_char_count++;
            // unique_char 以外ならコピー。
        }else{
            unique_char_count=0;
            *p2++ = *p1++;
        }
    }
    *p2 = '\0';
    return;
    #else
    char       *source_p, *work_p;
    char       *work_malloc_p;
    char       unique_char_count = 0;
    int                         org_sentence_len;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // オリジナル文字列長を保存。
    org_sentence_len = strlen(sentence);
    // ワークバッファ確保。
    work_malloc_p = (char*)mymalloc( org_sentence_len+10 );
    if ( work_malloc_p == NULL ){
        return;
    }
    source_p = sentence;
    work_p = work_malloc_p;
    // sensense文字列から、unique_char以外をワークへコピー。
    while (*source_p != '\0'){
        if (*source_p == unique_char)   {// unique_char発見
            if (unique_char_count == 0) {// 最初の一つならコピー。それ以外ならスキップ。
                *work_p = *source_p;
                work_p++;
            }
            unique_char_count++;
        }else   {// unique_char 以外ならコピー。
            unique_char_count = 0;
            *work_p = *source_p;
            work_p++;
        }
        source_p++;
    }
    *work_p = '\0';
    // ワークから、sentenceへ、結果を書き戻す。
    strncpy(sentence, work_malloc_p, org_sentence_len );
    myfree( work_malloc_p );    // Mem Free.
    return;
    #endif
}
//*********************************************************
// sentence文字列より、最初に出て来たcut_charの前後を分割。
//
//      sentence        (IN) 分割対象の文字列を与える。
//      cut_char        (IN) 分割対象の文字を入れる。
//      split1          (OUT)カットされた前の部分が入る。sentenceと同等のサイズが望ましい。
//      split2          (OUT)カットされた後ろの部分が入る。sentenceと同等のサイズが望ましい。
//
//
// return
//              0:                      正常終了。
//              それ以外：      エラー。分割失敗などなど。
//*********************************************************
int sentence_split(char *sentence, char cut_char, char *split1, char *split2)
{
    #if 1
    char *p = sentence;
    char *pos;
    // エラーチェック。
    if (sentence == NULL|| *sentence == 0 ||
    split1 == NULL ||
    split2 == NULL ){
        return 1;       // 引数にNULLまじり。
    }
    pos  = strchr( sentence, cut_char );
    // 分割文字発見できず。
    if( pos == NULL ){
        return 1;
    }
    //比較しながら複写
    while( *p ){
        //分割文字より前半
        if( p < pos ){
            *split1++ = *p++;
            //分割文字より後半
        }else if ( p > pos ){
            *split2++ = *p++;
            //分割文字位置
        }else{
            p++;
        }
    }
    //糸止め
    *split1 = 0;
    *split2 = 0;
    return 0; // 正常終了。
    #else
    char       *p;
    char       *malloc_p;
    int                         sentence_len;
    // エラーチェック。
    if (sentence == NULL|| *sentence == 0 ||
    split1 == NULL ||
    split2 == NULL ){
        return 1;       // 引数にNULLまじり。
    }
    // sentence の長さをGet.
    sentence_len = strlen(sentence);
    // ワーク領域malloc.
    malloc_p = mymalloc(sentence_len + 10);
    if (malloc_p == NULL){
        // malloc 失敗。エラー。
        return 1;
    }
    // sentence文字列をワークにコピー。
    strncpy(malloc_p, sentence, sentence_len + 10);
    // sentence 内に、cut_char が有るかチェック。無ければエラー。
    p = strchr(malloc_p, cut_char);
    if (p == NULL){
        myfree(malloc_p);
        return 1;       // 分割文字発見できず。
    }
    // cut_charより、後ろをカット。
    *p = '\0';
    // 前半部分をコピー。
    strncpy(split1, malloc_p, sentence_len);
    // 後半部分をコピー。
    p++;
    strncpy(split2, p, sentence_len);
    myfree(malloc_p);
    return 0; // 正常終了。
    #endif
}
char* ExtractFileExtension( char* filename )
{
    static char buf[4];
    filename_to_extension(filename,buf,sizeof(buf));
    return buf;
}
//******************************************************************
// filenameから、拡張子を取り出す('.'も消す）
// '.'が存在しなかった場合、拡張子が長すぎた場合は、""が入る。
//******************************************************************
void filename_to_extension(char *filename, char *extension_buf, unsigned int extension_buf_size)
{
    char       *p;
    // 拡張子の存在チェック。
    p = strrchr(filename, '.' );
    if (( p == NULL ) || ( strlen(p) > extension_buf_size )){
        *extension_buf = 0;
        //strncpy(extension_buf, "", extension_buf_size );
        return;
    }
    // 拡張子を切り出し。
    p++;
    strncpy( extension_buf, p, extension_buf_size );
    return;
}
// **************************************************************************
// text_buf から、CR/LF か'\0'が現れるまでを切り出して、line_bufにcopy。
// (CR/LFはcopyされない)
// 次の行の頭のポインタをreturn。
// Errorか'\0'が現れたらNULLが戻る。
// **************************************************************************
char *buffer_distill_line(char *text_buf_p, char *line_buf_p, unsigned int line_buf_size )
{
    char       *p;
    unsigned int                counter = 0;
    p = text_buf_p;
    // ------------------
    // CR/LF '\0'を探す
    // ------------------
    while ( 1 ){
        if ( *p == '\r' ){ // CR
            p++;
            continue;
        }
        if ( *p == '\n' )       {// LF
            p++;
            break;
        }
        if ( *p == '\0' ){
            break;
        }
        p++;
        counter++;
    }
    // --------------------------------------------------
    // 数えた文字数だけ、line_buf_p に文字列をコピー
    // --------------------------------------------------
    memset(line_buf_p , '\0', line_buf_size );
    if ( counter >= line_buf_size ){
        counter = (line_buf_size -1);
    }
    strncpy(line_buf_p, text_buf_p, counter);
    if ( *p == '\0' ){
        return NULL;            // バッファの最後
    }else{
        return p;               // バッファの途中
    }
}
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
int uri_encode(char *dst,  unsigned int dst_len, const char *src, unsigned int src_len)
{
    unsigned int idx_src;
    unsigned int idx_dst;
    int cnt;
    // 引数チェック
    if((dst == NULL) || (dst_len < 1) || (src == NULL) || (src_len < 1)){
        return 0;
    }
    cnt = 0;
    for (idx_src = idx_dst = 0 ; (idx_src < src_len) && (idx_dst < dst_len) && (src[idx_src] != '\0'); idx_src++){
        /* ' '(space) はちと特別扱いにしないとまずい */
        if ( src[idx_src] == ' ' ){
            //dst[idx_dst++] = '+';
            dst[idx_dst++] = '%';
            dst[idx_dst++] = '2';
            dst[idx_dst++] = '0';
        }
        /* エンコードしない文字全員集合 */
        else if ( strchr("!$()*,-./:;?@[]^_`{}~", src[idx_src]) != NULL ){
            dst[idx_dst] = src[idx_src];
            idx_dst += 1;
        }
        /* アルファベットと数字はエンコードせずそのまま */
        else if ( isalnum( src[idx_src] ) ){
            dst[idx_dst] = src[idx_src];
            idx_dst += 1;
        }
        /* \マークはエンコード */
        else if ( strchr(DELIMITER, src[idx_src]) != NULL ){
            dst[idx_dst++] = '%';
            dst[idx_dst++] = '5';
            dst[idx_dst++] = 'C';
        }
        /* それ以外はすべてエンコード */
        else{
            if ((idx_dst + 3) > dst_len)
            break;
            idx_dst += sprintf(&dst[idx_dst],"%%%2X",(unsigned char)(src[idx_src]));
        }
        cnt++;
        if ((idx_dst + 1) < dst_len){
            dst[idx_dst] = '\0';
        }
    }
    return cnt;
    // 2004/10/01 Update end
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
int uri_decode(char *dst, unsigned int dst_len, const char *src, unsigned int src_len)
{
    unsigned int    idx_src;
    unsigned int    idx_dst;
    int             cnt;
    char            work[3];
    //const char    *ptr_stop;
    char            *strtol_end_ptr;
    int             code;
    // 引数チェック
    if ((dst == NULL) || (dst_len < 1) || (src == NULL) || (src_len < 1)){
        return 0;
    }
    cnt = 0;
    // =================
    // メインループ
    // =================
    for (idx_src = idx_dst = 0; (idx_src < src_len) && (idx_dst < dst_len) && (src[idx_src] != '\0'); idx_dst++ , cnt++){
        if (src[idx_src] == '%'){
            if (idx_src + 2 > src_len){
                break;
            }
            work[0] = src[idx_src+1];
            work[1] = src[idx_src+2];
            work[2] = '\0';
            code = strtol(work, &strtol_end_ptr, 16);
            //ptr_stop = &src[idx_src + (strtol_end_ptr - work) + 1];
            if (code == LONG_MIN || code == LONG_MAX){
                break;
            }
            if (strtol_end_ptr != NULL){
                if (*strtol_end_ptr != '\0'){
                    break;
                }
            }
            dst[idx_dst] = (char)code;
            idx_src += 3;
        }else if ( src[idx_src] == '+' ){
            dst[idx_dst] = ' ';
            idx_src += 1;
            //ptr_stop++;
        }else{
            dst[idx_dst] = src[idx_src];
            idx_src += 1;
            //ptr_stop++;
        }
        if (idx_dst + 1 < dst_len){
            dst[idx_dst + 1] = '\0';
        }
    }
    return cnt;
}
/********************************************************************************/
// "YYYY/MM/DD HH:MM:SS" 形式の現在の日時の文字列を生成する。
/********************************************************************************/
void make_datetime_string(char *sentence)
{
    time_t                       now;
    struct tm                   *tm_p;
    // 現在時刻Get.
    time(&now);
    tm_p = localtime(&now);
    sprintf(sentence,    "%04d/%02d/%02d %02d:%02d:%02d",
    tm_p->tm_year+1900  ,       // 年
    tm_p->tm_mon+1      ,               // 月
    tm_p->tm_mday       ,               // 日
    tm_p->tm_hour       ,               // 時刻
    tm_p->tm_min        ,               // 分
    tm_p->tm_sec                );      // 秒
    return;
}
//*******************************************************************
// デバッグ出力初期化(ファイル名セット)関数
// この関数を最初に呼ぶまでは、デバッグログは一切出力されない。
//*******************************************************************
void debug_log_initialize(const char *set_debug_log_filename)
{
    // 引数チェック
    if (set_debug_log_filename == NULL){
        return;
    }
    if ( strlen(set_debug_log_filename) == 0 ){
        return;
    }
    // デバッグログファイル名をセット。
    strncpy(debug_log_filename, set_debug_log_filename,	sizeof(debug_log_filename) );
    // デバッグログ 初期化完了フラグを0に。
    debug_log_initialize_flag = 0;
    return;
}
//*************************************************
// デバッグ出力用関数。
// printf() と同じフォーマットにて使用する。
//*************************************************
void debug_log_output(const char *fmt, ...)
{
    #ifdef _DEBUG
    int        fd;
    char       buf[1024*5+1]={0};
    char       work_buf[1024*4+1]={0};
    char       date_and_time[32];
    char       replace_date_and_time[256]={0};
#ifdef linux
    struct timeval tv;
#endif
    va_list     arg;
    int         len;
    // =========================================
    // デバッグログ 初期化フラグをチェック
    // =========================================
    if (debug_log_initialize_flag != 0 ){
        return;
    }
    // =========================================
    // Debug出力文字列生成。
    // 行頭に、date_and_time を挿入しておく
    // =========================================
    //memset(buf, '\0', sizeof(buf));
    //memset(work_buf, '\0', sizeof(work_buf));
    // 引数で与えられた文字列を展開。
    va_start(arg, fmt);
    vsnprintf(work_buf, sizeof(work_buf), fmt, arg);
    va_end(arg);
    // work_bufの一番最後に'\n'がついていたら削除。
    len = strlen(work_buf);
    if (len > 0 && work_buf[len-1] == '\n'){
        work_buf[len-1] = '\0';
    }
    // 挿入用文字列生成( "\ndate_and_time" になる)
    make_datetime_string(date_and_time);
#ifdef linux
    gettimeofday(&tv,NULL);
    snprintf(replace_date_and_time, sizeof(replace_date_and_time), "\n%s.%06d[%d] ", date_and_time, (int)tv.tv_usec, getpid() );
#else
    snprintf(replace_date_and_time, sizeof(replace_date_and_time), "\n%s.%03d[%d] ", date_and_time, (::GetTickCount() % 1000),GetCurrentThreadId() );
#endif
    
    // 出力文字列生成開始。
    //QueryPerformanceCounter((LARGE_INTEGER*)&num);
#ifdef linux
    snprintf((char*)buf, sizeof(buf), "%s.%06d[%d] %s", date_and_time, (int)tv.tv_usec, getpid(), work_buf);
#else
    snprintf(buf, sizeof(buf), "%s.%03d[%d] %s", date_and_time, (::GetTickCount() % 1000),getpid(), work_buf);
#endif
    //last = num;
    replace_character(buf, "\n", replace_date_and_time); // \nの前にdate_and_timeを挿入
    // 一番最後に'\n'をつける。
    strncat(buf, "\n", sizeof(buf)-strlen(buf) );
    // =====================
    // ログファイル出力
    // =====================
    fd = myopen(debug_log_filename, O_CREAT | O_APPEND | O_WRONLY | O_BINARY , S_IREAD | S_IWRITE );
    if ( fd < 0 ){
        return;
    }
    // 出力
    write(fd, buf, strlen(buf));    // メッセージ実体を出力
    // ファイルクローズ
    close( fd );
    return;
//DEBUGが定義されてない場合ログ出力しない
    #else
    IGNORE_PARAMETER(fmt);
    #endif
}
// **************************************************************************
// 拡張子変更処理。追加用。
//      extension_convert_listに従い、org → rename への変換を行う。
//
// 例) "hogehoge.m2p" → "hogehoge.m2p.mpg"
// **************************************************************************
void extension_add_rename(char *rename_filename_p, size_t rename_filename_size)
{
    int i;
    char       ext[FILENAME_MAX];
    if ( rename_filename_p == NULL )
    return;
    filename_to_extension(rename_filename_p, ext, sizeof(ext));
    if (strlen(ext) <= 0) return;
    for ( i=0; extension_convert_list[i].org_extension != NULL; i++ ){
        debug_log_output("org='%s', rename='%s'"
        , extension_convert_list[i].org_extension
        , extension_convert_list[i].rename_extension);
        // 拡張子一致？
        if ( strcasecmp(ext, extension_convert_list[i].org_extension) == 0 ){
            debug_log_output(" HIT!!!" );
            // 拡張子を「追加」
            strncat(rename_filename_p, "."
            , rename_filename_size - strlen(rename_filename_p));
            strncat(rename_filename_p
            , extension_convert_list[i].rename_extension
            , rename_filename_size - strlen(rename_filename_p));
            debug_log_output("rename_filename_p='%s'", rename_filename_p);
            break;
        }
    }
    return;
}
// **************************************************************************
// 拡張子変更処理。削除用。
//      extension_convert_listに従い、rename → org への変換を行う。
//
// 例) "hogehoge.m2p.mpg" → "hogehoge.m2p"
// **************************************************************************
void extension_del_rename(char *rename_filename_p)
{
    int i;
    char       renamed_ext[FILENAME_MAX];
    char       ext[FILENAME_MAX];
    if ( rename_filename_p == NULL ){
        return;
    }
    for ( i=0; extension_convert_list[i].org_extension != NULL; i++ ){
        debug_log_output("org='%s', rename='%s'"
        , extension_convert_list[i].org_extension
        , extension_convert_list[i].rename_extension);
        snprintf(renamed_ext, sizeof(renamed_ext), ".%s.%s"
        , extension_convert_list[i].org_extension
        , extension_convert_list[i].rename_extension);
        // 比較する拡張子と同じ長さにそろえる。
        strncpy(ext, rename_filename_p, sizeof(ext));
        cut_before_n_length(ext, strlen(renamed_ext));
        // 拡張子一致？
        if ( strcasecmp(ext, renamed_ext) == 0 ){
            debug_log_output(" HIT!!!" );
            // 拡張子を「削除」
            cut_after_n_length(rename_filename_p, strlen(extension_convert_list[i].rename_extension) + 1);
            debug_log_output("rename_filename_p='%s'", rename_filename_p);
            break;
        }
    }
    return;
}
// **************************************************************************
// * PNGフォーマットファイルから、画像サイズを得る。
// **************************************************************************
void png_size(char *png_filename, unsigned int *x, unsigned int *y)
{
    int         fd;
    unsigned char       buf[255]={0};
    ssize_t     read_len;
    *x = 0;
    *y = 0;
    fd = myopen(png_filename, O_BINARY|O_RDONLY);
    if ( fd < 0 ){
        return;
    }
    // ヘッダ+サイズ(0x18byte)  読む
    //memset(buf, 0, sizeof(buf));
    read_len = read(fd, (char*)buf, 0x18);
    if ( read_len == 0x18){
        *x =    (buf[0x10] << 24)       +
        (buf[0x11] << 16)       +
        (buf[0x12] << 8 )       +
        (buf[0x13]);
        *y =    (buf[0x14] << 24)       +
        (buf[0x15] << 16)       +
        (buf[0x16] << 8 )       +
        (buf[0x17]);
    }
    close( fd );
    return;
}
// **************************************************************************
// * GIFフォーマットファイルから、画像サイズを得る。
// **************************************************************************
void gif_size(char *gif_filename, unsigned int *x, unsigned int *y)
{
    int         fd;
    unsigned char       buf[255]={0};
    ssize_t     read_len;
    *x = 0;
    *y = 0;
    fd = myopen(gif_filename, O_BINARY|O_RDONLY);
    if ( fd < 0 ){
        return;
    }
    // ヘッダ+サイズ(10byte)  読む
    //memset(buf, 0, sizeof(buf));
    read_len = read(fd, (char*)buf, 10 );
    if ( read_len == 10){
        *x = buf[6] + (buf[7] << 8);
        *y = buf[8] + (buf[9] << 8);
    }
    close( fd );
    return;
}
// **************************************************************************
// * JPEGフォーマットファイルから、画像サイズを得る。
// **************************************************************************
void  jpeg_size(char *jpeg_filename, unsigned int *x, unsigned int *y)
{
    int                 fd;
    unsigned char       buf[255];
    ssize_t             read_len;
    off_t               length;
    *x = 0;
    *y = 0;
    //debug_log_output("jpeg_size: '%s'.", jpeg_filename);
    fd = myopen(jpeg_filename,  O_BINARY|O_RDONLY);
    if ( fd < 0 ){
        return;
    }
    while ( 1 ){
        // マーカ(2byte)  読む
        read_len = read(fd, buf, 2);
        if ( read_len != 2){
            //debug_log_output("fraed() EOF.\n");
            break;
        }
        // Start of Image.
        if (( buf[0] == 0xFF ) && (buf[1] == 0xD8)){
            continue;
        }
        // Start of Frame 検知
        if (( buf[0] == 0xFF ) && ( buf[1] >= 0xC0 ) && ( buf[1] <= 0xC3 )){ // SOF 検知
            //debug_log_output("SOF0 Detect.");
            // sof データ読み込み
            memset(buf, 0, sizeof(buf));
            read_len = read(fd, buf, 0x11);
            if ( read_len != 0x11 ){
                debug_log_output("fraed() error.\n");
                break;
            }
            *y = (buf[3] << 8) + buf[4];
            *x = (buf[5] << 8) + buf[6];
            break;
        }
        // SOS検知
        if (( buf[0] == 0xFF ) && (buf[1] == 0xDA)){ // SOS 検知
            //debug_log_output("Start Of Scan.\n");
            // 0xFFD9 探す。
            while ( 1 ){
                // 1byte 読む
                read_len = read(fd, buf, 1);
                if ( read_len != 1 ){
                    //debug_log_output("fraed() error.\n");
                    break;
                }
                // 0xFFだったら、もう1byte読む
                if ( buf[0] == 0xFF ){
                    buf[0] = 0;
                    read(fd, buf, 1);
                    // 0xD9だったら 終了
                    if ( buf[0] == 0xD9 ){
                        //debug_log_output("End Of Scan.\n");
                        break;
                    }
                }
            }
            continue;
        }
        // length 読む
        memset(buf, 0, sizeof(buf));
        read(fd, buf, 2);
        length = (buf[0] << 8) + buf[1];
        // length分とばす
        lseek(fd, length-2, SEEK_CUR );
    }
    close(fd);
    return;
}
// **************************************************************************
char *my_strcasestr(const char *p1, const char *p2)
{
    size_t len;
    len = strlen(p2);
    if (len == 0) return (char*)p1;
    while (*p1) {
        if (!strncasecmp(p1, p2, len)) {
            return (char*)p1;
        }
        p1++;
    }
    return NULL;
}
//---------------------------------------------------------------------------
char *path_sanitize(char *orig_dir, size_t dir_size)
{
    IGNORE_PARAMETER(dir_size);
#ifdef linux
    char *p;
    char *q;
    char *dir;
    char *buf;
    size_t malloc_len;
    if (orig_dir == NULL) return NULL;
    malloc_len = strlen(orig_dir) * 2;
    buf = (char*)malloc(malloc_len);
    buf[0] = '\0';
    p = buf;
    dir = q = orig_dir;
    while (q != NULL) {
        dir = q;
        while (*dir == '/') dir ++;
        q = strchr(dir, '/');
        if (q != NULL) {
            *q++ = '\0';
        }
        if (!strcmp(dir, "..")) {
            p = strrchr(buf, '/');
            if (p == NULL) {
                free(buf);
                dir[0] = '\0';
                return NULL; //  not allowed.
            }
            *p = '\0';
        } else if (strcmp(dir, ".")) {
            p += snprintf(p, malloc_len - (p - buf), "/%s", dir);
        }
    }
    if (buf[0] == '\0') {
        strncpy(orig_dir, "/", dir_size);
    } else {
        strncpy(orig_dir, buf, dir_size);
    }
    free(buf);
#endif
    return orig_dir;
}
static int myMkdir( wString FileName );
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//階層フォルダを作成する
//引数フォルダ名(最後が\で終わる）またはフルパス名（ファイル名含む）
int myMkdir( wString FileName )
{
    while( FileName.length() > 0 && FileName[FileName.length()-1] != '/' ){
        FileName = FileName.SubString( 0, FileName.length() - 1 );
    }
    if( FileName.length() > 0 ){
        FileName = FileName.SubString( 0, FileName.length() - 1 );
    }else{
        FileName = "/";
    }
    if( wString::DirectoryExists(FileName) ){
        return true;
    }else{
        //１つ上のフォルダを作って
        if( FileName.Pos("/") != (int)wString::npos  && 
              myMkdir(FileName.SetLength(FileName.Length()-1)) == true){
            //自分のフォルダ作成
            wString::CreateDir( FileName );
        }else{
            return false;
        }
    }
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
#define HTTP_BUF_SIZE (1024*10)
//HTTPDownload
//引数
//char* src:読み取り元URL
//char* dst:保存先ファイル名
//char* proxy:未実装
//off_t offset:０なら全取得,サイズ指定ならサイズに満たない場合、超えた場合全取得
//戻り値 1:成功 2:サイズ同じ　false:失敗

int HTTPDownload(char* src, char* dst, off_t offset)
{
    int         rbgn_time = time(NULL)+NO_RESPONSE_TIMEOUT;
    
    char        *buf;                           //バッファ
    int         recv_len;                       //読み取り長さ
    int         content_length;
    int         len;
    char*       work1;
    char*       work2;
    int         fd=-1;                          //ファイルディスクリプタ
    //int         num;
    char        host[256]={0};
    char        server[256];
    SOCKET      server_socket;                  //サーバーソケット
    int         status = true;
    int         server_port = HTTP_SERVER_PORT;
    //出力ファイルの設定
    // ================
    // 実体転送開始
    // ================
    
    //準備
    //領域の取得
    buf = mycalloc(HTTP_BUF_SIZE,1);
    //ホスト名の設定
    strncpy( host, src, sizeof( host));
    //先頭のHTTP://を抜く
    work2 = strstr(host, "://" )+3;
    if( work2 != NULL ){
        strcpy(host,work2);
    }
    //次の'/'からが本体
    work1 = strstr(host, "/" );
    strcpy( server, work1);
    //'/'の所で切る
    *work1 = 0;
    //ポートがあれば取得
    work1 = strstr(host,":");
    if( work1 ){
        server_port = atoi(work1+1);
        *work1 = 0;
    }
    
    //strcpy( host, work2 );
    //ソケット作成と接続
    server_socket = wString::sock_connect(host, server_port);
    if ( ! SERROR( server_socket ) ){
        //元ファイルがあった場合
        if( offset != 0 ){
            //コネクションクローズしない
            sprintf(buf , "HEAD %s HTTP/1.0\r\n"
            "Accept: */*\r\n"
            "User-Agent: %s\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n" ,
            wString(server).uri_encode(),
            //"Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
            USERAGENT,
            host );
            //サーバに繋がってheadをとった
            if( send( server_socket, buf, strlen( buf ) , 0) != SOCKET_ERROR ){
                //初回分からヘッダを削除
                recv_len = recv(server_socket, buf, HTTP_BUF_SIZE, 0);
                int num = atoi(strchr(buf,' ')+1);
                if(  200 <= num && num < 300 ){
                    //len = 0;
                    buf[recv_len] = 0;
                    //\r\n\r\nを探す
                    work1 = strstr(buf,"Content-Length:" );
                    if( work1 ){
                        work1+=16;
                        content_length = atoi(work1);
                        //PHPとかで中身がない。
                    }else{
                        sClose(server_socket);
                        myfree( buf );
                        return false;
                    }
                }else if( num == 302 ){
                    //Location:----\r\n
                    work1 = strstr(buf,"Location:")+10;
                    if( work1 ){
                        work2 = work1;
                        while( work2[0] != '\r' || work2[1] != '\n' ){
                            work2++;
                        }
                        *work2 = 0;
                        sClose(server_socket);
                        status = HTTPDownload(work1,dst,offset);
                        myfree( buf );
                        return status;
                    }
                    sClose(server_socket);
                    myfree( buf );
                    return false;
                }
                //サーバから返答なし
            }else{
                sClose(server_socket);
                myfree( buf );
                return false;
            }
            sClose(server_socket);
            server_socket = wString::sock_connect(host, server_port);
            if ( SERROR( server_socket ) ){

                myfree( buf );
                return false;
            }
            //HTTP1.0 GET発行 レンジ付き
            if( offset<content_length ){   //range発行
                sprintf(buf , "GET %s HTTP/1.0\r\n"
                "Accept: */*\r\n"
#ifdef linux
                "User-Agent: %s\r\nHost: %s\r\nRange: bytes=%zu-\r\nConnection: close\r\n\r\n" ,
#else
                "User-Agent: %s\r\nHost: %s\r\nRange: bytes=%llu-\r\nConnection: close\r\n\r\n" ,
#endif
                wString(server).uri_encode() ,
                //"Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
                USERAGENT,
                host,
                offset);
                //                               GetAuthorization(void),
                myMkdir(wString(dst));
                fd = open(dst, O_WRONLY | O_APPEND | O_BINARY, 0777);
                //HTTP1.0 GET発行 ファイルが変なので全部取得
            }else if( offset > content_length ){
                sprintf(buf , "GET %s HTTP/1.0\r\n"
                "Accept: */*\r\n"
                "User-Agent: %s\r\nHost: %s\r\nConnection: close\r\n\r\n" ,
                wString(server).uri_encode() ,
                //Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
                USERAGENT,
                host);
                //                               GetAuthorization(void),
                myMkdir(wString(dst));
                fd = open(dst, O_WRONLY | O_TRUNC | O_BINARY, 0777);
                //取得済み
            }else{
                sClose(server_socket);
                myfree( buf );
                return 2;
            }
            
            
            //ファイルはありません。
        }else{
            sprintf(buf , "GET %s HTTP/1.0\r\n"
            "Accept: */*\r\n"
            "User-Agent: %s\r\nHost: %s\r\nConnection: close\r\n\r\n" ,
            wString(server).uri_encode(),
            //"Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)",
            USERAGENT,
            host);
            //                         GetAuthorization(void),
            
            
            
            
            myMkdir(wString(dst));
            fd = open(dst, O_WRONLY | O_CREAT | O_BINARY, 0777);
        }
        //ファイルがないならエラー
        if ( fd < 0 ){
            sClose(server_socket);
            myfree( buf );
            debug_log_output("open() error.");
            return ( false );
        }
        //サーバに繋がった
        if( send( server_socket, buf, strlen( buf ) , 0) != SOCKET_ERROR ){
            //初回分からヘッダを削除
            recv_len = recv(server_socket, buf, HTTP_BUF_SIZE, 0);
            int num = atoi(strchr(buf,' ')+1);
            if( num == 200 || num == 206 ){
                len = 0;
                //コンテンツ長さ
                content_length = atoi(strstr(buf,"Content-Length:" )+16);
                //\r\n\r\nを探す
                work1 = strstr(buf,HTTP_DELIMITER )+4;//sizeof( HTTP_DELIMITER );//実体の先頭
                recv_len -= (work1-buf);
                memcpy( buf, work1,recv_len );           //移動
                write(fd,buf, recv_len);                //書き込めないことはないと
                len += recv_len;
                rbgn_time = time(NULL)+NO_RESPONSE_TIMEOUT;
                while(loop_flag ){
                    recv_len = recv(server_socket, buf, HTTP_BUF_SIZE, 0);
                    if ( recv_len < 0 ){
                        break;
                    }else if( recv_len > 0 ){
                        write(fd,buf, recv_len);            //書き込めないことはないと
                        //新興
                        //ここを更新しないと１０秒で書き込みが終わる。
                        rbgn_time = time(NULL)+NO_RESPONSE_TIMEOUT;
                        //buf += recv_len;
                        len += recv_len;
                        //指定時刻書き込めなかったら落ちる
                    }else if( len == content_length ){
                        status = 1;
                        break;
                    }else if( time( NULL ) > rbgn_time ){
                        status = false;
                        break;
                    }
                }
            }else if ( num ==  302 ){
                work1 = strstr(buf,"Location:")+10;
                if( work1 ){
                    work2 = work1;
                    while( work2[0] != '\r' || work2[1] != '\n' ){
                        work2++;
                    }
                    *work2 = 0;
                    sClose(server_socket);
                    close(fd);
                    status = HTTPDownload(work1,dst,offset);
                    myfree( buf );
                    return status;
                }
            }else{
                close(fd);
                fd = -1;
                unlink(dst);
                status = false;
            }
        }
        sClose(server_socket);
    }
    // スレッド終了
    myfree( buf );
    if( fd != -1 ){
        close(fd);
    }
    //ExitThread(TRUE);
    return status;
}
char* GetAuthorization(void)
{
    #if 0
    static char work[256];
    if( AuthorizedString[0] ){
        sprintf( work,"Authorization: Basic %s", IDPW64 );
        return work;
    }else{
        return "";
    }
    #else
    return (char*)"";
    #endif
}

#ifdef linux
//---------------------------------------------------------------------------
int send(int fd,const char* buffer, unsigned int length, int mode)
{
    return write(fd,buffer,length);
}
//---------------------------------------------------------------------------
int recv(int fd,char* buffer, unsigned int length, int mode)
{
    return read(fd,buffer,length);
}
//---------------------------------------------------------------------------
int getTargetFile( const char *LinkFile, char *TargetFile )
{
    return FALSE;
}
//---------------------------------------------------------------------------
void Sleep(unsigned int milliseconds)
{
    unsigned long sec = milliseconds/1000;
    milliseconds %= 1000;
    //linuxでは秒タイマー
    if( sec ){
        sleep(sec);
    }
    //linuxではμ秒タイマー
    if( milliseconds ){
        usleep(milliseconds*1000);
    }
}
#endif
char* mymalloc(size_t size)
{
    return( new char[size] );
}
char* mycalloc(size_t size1, int num)
{
    char* tmp = new char[size1*num];
    memset( tmp, 0, size1*num);
    return tmp;
}
void myfree(char* ptr)
{
    delete [] ptr;
}
// ソケットを作成し、相手に接続するラッパ. 失敗 = -1
//---------------------------------------------------------------------------
SOCKET sock_connect(char *host, int port)
{
    SOCKET sock;
    struct sockaddr_in sockadd={0};     //ＳＯＣＫＥＴ構造体
    struct hostent *hent;
    debug_log_output("sock_connect: %s:%d", host, port);
    //ＳＯＣＫＥＴ作成
    if (SERROR(sock = socket(PF_INET, SOCK_STREAM, 0))){
        debug_log_output("sock_connect_error:");
        return INVALID_SOCKET;
    }
    debug_log_output("sock: %d", sock);
    if (NULL == (hent = gethostbyname(host))) {
        sClose( sock );
        return INVALID_SOCKET;
    }
    debug_log_output("hent: %p", hent);
    //ソケット構造体へアドレス設定
    memcpy(&sockadd.sin_addr, hent->h_addr, hent->h_length);
    //ソケット構造体へポート設定
    sockadd.sin_port = htons((u_short)port);
    //ＩＰＶ４アドレスファミリを設定
    sockadd.sin_family = AF_INET;
    //接続
    if (SERROR(connect(sock, (struct sockaddr*)&sockadd, sizeof(sockadd)))) {
        debug_log_output("connect: error Content=%s\n",host);
        sClose( sock );
        return INVALID_SOCKET;
    }
    debug_log_output("Sock Connected\n");
    set_nonblocking_mode(sock, 0);    /* blocking */
    return sock;
}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int open(unsigned char *pathname, int flags)
{
    return open((char*)pathname,flags);
}
int open(unsigned char *pathname, int flags, mode_t mode)
{
    return open((char*)pathname,flags,mode);
}
/********************************************************************************/
// 日本語文字コード変換。
// (libnkfのラッパー関数)
//
//      サポートされている形式は以下の通り。
//              in_flag:        CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//              out_flag:       CODE_SJIS, CODE_EUC
/********************************************************************************/
void convert_language_code(const char *in, char *out, size_t len, int in_flag, int out_flag)
{
    char       nkf_option[8];
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
    *out = 0;
    nkf((const char*)in, out, len, (const char*)nkf_option);
    return;
}

#ifdef linux
//ファイルディスクリプタブロック設定
//1:ノンブロッキング 0:ブロッキング
void set_nonblocking_mode(int fd, int flag)
{
    int res, nonb = 0;
    nonb |= O_NONBLOCK;
    if ((res = fcntl(fd, F_GETFL, 0)) == -1) {
        debug_log_output("fcntl(fd, F_GETFL) failed");
    }
    if (flag) {
        res |= O_NONBLOCK;
    } else {
        res &= ~O_NONBLOCK;
    }
    if (fcntl(fd, F_SETFL, res) == -1) {
        debug_log_output("fcntl(fd, F_SETFL, nonb) failed");
    }
}
#endif
//linux/windows共用fopen
//multi threadではsystem関数等とバッティングするのでなるべく使わない
FILE* myfopen(const char* filename,const char* mode)
{
#ifdef linux
    return fopen(filename,mode);
#else
    char work[1024];
    strcpy( work, filename);
    int ptr=0;
    while( work[ptr] ){
        if( work[ptr] == '/' ){
            work[ptr] = '\\';
        }
        ptr++;
    }
    return fopen(work,mode);
#endif
}
//linux/windows共用オープン
//追加: O_CREAT | O_APPEND | O_WRONLY(またはO_RDWR) | (O_BINARY) , S_IREAD | S_IWRITE
//新規: O_CREAT | O_TRUNC  | O_WRONLY(またはO_RDWR) | (O_BINARY) , S_IREAD | S_IWRITE
//読込: O_RDONLY                                     | (O_BINARY) 
int myopen(const char* filename,int amode, int option)
{
#ifdef linux
    if( option != 0 ){
        return open(filename,amode,option);
    }else{
    return open(filename,amode);
    }
#else
    char work[1024];
    strcpy( work, filename);
    int ptr=0;
    while( work[ptr] ){
        if( work[ptr] == '/' ){
            work[ptr] = '\\';
        }
        ptr++;
    }
    if( option != 0 ){
        return open(work,amode,option);
    }else{
        return open(work,amode);
    }
#endif
}
int sClose(SOCKET socket)
{
    int ret;
#ifdef linux
    ret = shutdown( socket , SD_BOTH );
    if( ret != 0 ) debug_log_output("shutdown error=%s,%d,%d",strerror(errno),errno,socket);
    ret = close(socket);
    if( ret != 0 ) debug_log_output("close error=%s,%d,%d",strerror(errno),errno,socket);
#else
    ret = shutdown( socket , SD_BOTH );
    if( ret != 0 ) {
       debug_log_output("shutdown error=%s,%d,%d",strerror(errno),errno,socket);
    }
    ret = closesocket(socket);
    if( ret != 0 ){
         debug_log_output("close error=%s,%d,%d",strerror(errno),errno,socket);
    }
#endif
    return ret;
}
#ifdef linux
void ExitThread(DWORD dwExitCode)
{
    return;
}
#endif
// **************************************************************************
// fdから、１行(CRLFか、LF単独が現れるまで)受信
// CRLFは削除する。
// 受信したサイズをreturnする。
// **************************************************************************
int readLine(int fd, char *line_buf_p, int line_max)
{
    char byte_buf;
    int  line_len=0;
    int	 recv_len;
    // １行受信実行
    while ( 1 ){
        recv_len = read(fd, &byte_buf, 1);
        if ( recv_len != 1 ){ // 受信失敗チェック
            return ( -1 );
        }
        // CR/LFチェック
        if       ( byte_buf == '\r' ){
            continue;
        }else if ( byte_buf == '\n' ){
            *line_buf_p = 0;
            break;
        }
        // バッファにセット
        *line_buf_p++ = byte_buf;
        // 受信バッファサイズチェック
        if ( ++line_len >= line_max){
            // バッファオーバーフロー検知
            return ( -1 );
        }
    }
    return line_len;
}
