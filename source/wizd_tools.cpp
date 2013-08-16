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
#include  <sys/types.h>
#include  <ctype.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <string.h>
#include  <limits.h>
#include  <time.h>
#include  <sys/stat.h>
#include  <errno.h>
#include  <sys/types.h>
#include  <fcntl.h>
#include  <unistd.h>
#include  "wizd.h"
#include  "wizd_tools.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <netdb.h>
static unsigned char		debug_log_filename[FILENAME_MAX];	// デバッグログ出力ファイル名(フルパス)
static unsigned char		debug_log_initialize_flag  = (1);	// デバッグログ初期化フラグ
/********************************************************************************/
// sentence文字列内のkey文字列をrep文字列で置換する。
/********************************************************************************/
void replace_character(unsigned char *sentence, const unsigned char *key, const unsigned char *rep)
{
#if 0
    int sentence_buf_size = 4096;
    int malloc_size;
    unsigned char       *p, *buf;
    if ( strlen(key) == 0 )
    return;
    malloc_size = strlen(sentence) * 4;
    buf = (unsigned char*)mymalloc(malloc_size);
    if ( buf == NULL )
    return;
    p = (unsigned char*)strstr(sentence, key);
    while (p != NULL){
        *p = '\0';
        strncpy(buf, p+strlen(key), malloc_size );
        strncat(sentence, rep, sentence_buf_size - strlen(sentence) );
        strncat(sentence, buf, sentence_buf_size - strlen(sentence) );
        p = (unsigned char*)strstr(p+strlen(rep), key);
    }
    myfree(buf);
    return;
#else
    unsigned char* p;
    unsigned char* str;
    int klen=strlen(key);
    int rlen=strlen(rep);
    int slen=strlen(sentence);
    int num;
    if ( klen == 0 || slen == 0){
        return;
    }
    p = (unsigned char*)strstr((char*)sentence, (char*)key);
    if( klen == rlen ){
        while (p != NULL){
            memcpy( (char*)p,(char*)rep,rlen);
            //strncpy( (char*)p,(char*)rep,rlen);
            p = (unsigned char*)strstr((char*)p+rlen, (char*)key);
        }
        //前詰め置換そのままコピーすればいい
    }else if( klen > rlen ){
        num = klen-rlen;
        while (p != NULL){
            strcpy( (char*)p,(char*)(p+num));
            memcpy( (char*)p,(char*)rep,rlen);
            p = (unsigned char*)strstr((char*)p+rlen, (char*)key);
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
            memcpy( (char*)p,(char*)rep,rlen);
            p = (unsigned char*)strstr((char*)p+rlen, (char*)key);
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
void replace_character_first(unsigned char *sentence,const unsigned char *key, const unsigned char *rep)
{
    unsigned char* p;
    unsigned char* str;
    int klen=strlen(key);
    int rlen=strlen(rep);
    int slen=strlen(sentence);
    int num;
    if ( klen == 0 || slen == 0){
        return;
    }
    p = (unsigned char*)strstr((char*)sentence, (char*)key);
    if( klen == rlen ){
        memcpy( (char*)p,(char*)rep,rlen);
        //前詰め置換そのままコピーすればいい
    }else if( klen > rlen ){
        num = klen-rlen;
        strcpy( (char*)p,(char*)(p+num));
        memcpy( (char*)p,(char*)rep,rlen);
        //置換文字が長いので後詰めする
    }else{
        num = rlen-klen;
        //pからrlen-klenだけのばす
        for( str = sentence+slen+num ; str-num >= p ; str-- ){
            *str = *(str-num);
        }
        memcpy( (char*)p,(char*)rep,rlen);
    }
    return;
}
// **************************************************************************
// sentence 文字列内の、start_key文字列とend_key文字列に挟まれた部分を削除する
// **************************************************************************
void cut_enclose_words(unsigned char *sentence, unsigned char *start_key, unsigned char *end_key)
{
    char* start_p;
    char* end_p;
    int elen;
    int slen;
    int len;
    if( sentence  == NULL || *sentence  == 0 ||
    start_key == NULL || *start_key == 0 ||
    end_key   == NULL || *end_key   == 0 ){
        return;
    }
    //end_keyの長さ
    elen= strlen(end_key);
    slen= strlen(sentence);
    for(;;){
        //start_keyとend_keyを探す
        start_p = strstr((char*)sentence,(char*)start_key);
        if( start_p != NULL ){
            end_p = strstr((char*)start_p,(char*)end_key);
            if( end_p == NULL ){
                //end_keyがない
                break;
            }
        }else{
            //start_keyがない
            break;
        }
        //移動
        len = slen-(int)((char*)end_p-(char*)sentence)-elen;
        strncpy((char*)start_p,(char*)(end_p+elen),len);
        start_p[len] = '\0';//sentence[len] = '\0';
    }
    return;
}
//***************************************************************************
// sentence文字列より、cut_charから後ろを削除
//	見つからなければ何もしない。
// 入力  : unsigned char* sentence 入力文字列
//         unsigned char  cut_char 切り取り文字
// 戻り値: unsigned char* 切り取った後ろの文字列
//***************************************************************************
char*   cut_after_character(unsigned char *sentence, unsigned char cut_char)
{
    unsigned char       *symbol_p;
    // 削除対象キャラクターがあった場合、それから後ろを削除。
    symbol_p = (unsigned char*)strchr((char*)sentence, cut_char);
    if (symbol_p != NULL){
        *symbol_p++ = '\0';
    }
    return (char*)symbol_p;
}
//***************************************************************************
// sentence文字列の、cut_charが最初に出てきた所から前を削除
// もし、cut_charがsentence文字列に入っていなかった場合、文字列全部削除
//***************************************************************************
void 	cut_before_character(unsigned char *sentence, unsigned char cut_char)
{
#if 1
    unsigned char* p;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // 削除対象キャラクターが最初に出てくる所を探す。
    p = (unsigned char*)strchr((char*)sentence, cut_char);
    if( p ){
        // 削除対象キャラクターの後ろから最後までの文字列をコピー
        strcpy((char*)sentence,(char*)++p);
        return;
    }else{
        //ない場合全部削除
        *sentence = 0;
    }
    return;
#else
    unsigned char       *symbol_p;
    unsigned char       *malloc_p;
    int                         sentence_len;
    if (sentence == NULL || *sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // 削除対象キャラクターが最初に出てくる所を探す。
    symbol_p = (unsigned char*)strchr((char*)sentence, cut_char);
    if (symbol_p == NULL){
        // 発見できなかった場合、文字列全部削除。
        strncpy((char*)sentence, "", sentence_len);
        return;
    }
    symbol_p++;
    // テンポラリエリアmalloc.
    malloc_p = (unsigned char*)mymalloc(sentence_len + 10);
    if (malloc_p == NULL)
    return;
    // 削除対象キャラクターの後ろから最後までの文字列をコピー
    strncpy((char*)malloc_p, (char*)symbol_p, sentence_len + 10);
    // sentence書き換え
    strncpy((char*)sentence, (char*)malloc_p, sentence_len);
    myfree(malloc_p);
    return;
#endif
}
//************************************************************************
// sentence文字列の、cut_charが最後に出てきた所から前を削除
// もし、cut_charがsentence文字列に入っていなかった場合、なにもしない。
//************************************************************************
void 	cut_before_last_character(unsigned char *sentence, unsigned char cut_char)
{
#if 1
    unsigned char* p;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // 削除対象キャラクターが最後に出てくる所を探す。
    p = (unsigned char*)strrchr((char*)sentence, cut_char);
    if( p ){
        // 削除対象キャラクターの後ろから最後までの文字列をコピー
        strcpy((char*)sentence,(char*)++p);
    }
#else
    unsigned char       *symbol_p;
    unsigned char       *malloc_p;
    int                         sentence_len;
    if (sentence == NULL ||*sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // 削除対象キャラクターが最後に出てくる所を探す。
    symbol_p = (unsigned char*)strrchr((char*)sentence, cut_char);
    if (symbol_p == NULL){
        // 発見できなかった場合、なにもしない。
        return;
    }
    symbol_p++;
    // テンポラリエリアmalloc.
    malloc_p = (unsigned char*)mymalloc(sentence_len + 10);
    if (malloc_p == NULL)
    return;
    // 削除対象キャラクターの後ろから最後までの文字列をコピー
    strncpy((char*)malloc_p, (char*)symbol_p, sentence_len + 10);
    // sentence書き換え
    strncpy((char*)sentence, (char*)malloc_p, sentence_len);
    myfree(malloc_p);
    return;
#endif
}
//************************************************************************
// sentence文字列の、cut_charが最後に出てきた所から後ろをCUT
// もし、cut_charがsentence文字列に入っていなかった場合、文字列全部削除。
//************************************************************************
void 	cut_after_last_character(unsigned char *sentence, unsigned char cut_char)
{
    unsigned char       *symbol_p;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    // 削除対象キャラクターが最後に出てくる所を探す。
    symbol_p = (unsigned char*)strrchr((char*)sentence, cut_char);
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
void    cat_before_n_length(unsigned char *sentence,  unsigned int n)
{
#if 1
    unsigned int len;
    if (sentence == NULL || *sentence == 0){
        return;
    }
    len = strlen(sentence);
    // sentence が、nよりも同じか短いならばreturn
    if (len <= n ){
        return;
    }
    strncpy( (char*)sentence, (char*)(sentence+len-n),n+1);
#else
    unsigned char       *malloc_p;
    unsigned char       *work_p;
    unsigned int        sentence_len;
    if (sentence == NULL || *sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // sentence が、nよりも同じか短いならばreturn
    if ( sentence_len <= n )
    return;
    // テンポラリエリアmalloc.
    malloc_p = (unsigned char*)mymalloc(sentence_len + 10);
    if (malloc_p == NULL)
    return;
    work_p = sentence;
    work_p += sentence_len;
    work_p -= n;
    strncpy((char*)malloc_p, (char*)work_p, sentence_len + 10);
    strncpy( (char*)sentence, (char*)malloc_p, sentence_len);
    myfree(malloc_p);
    return;
#endif
}
//******************************************************************
// sentenceの、後ろ n byteを削除
//  全長がn byteに満たなかったら、文字列全部削除
//******************************************************************
void    cat_after_n_length(unsigned char *sentence,  unsigned int n)
{
    unsigned char       *work_p;
    unsigned int        sentence_len;
    if (sentence == NULL || *sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // sentence が、nよりも同じか短いならば、全部削除してreturn;
    if ( sentence_len <= n ){
        strncpy((char*)sentence, "", sentence_len);
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
void    cut_character(unsigned char *sentence, unsigned char cut_char)
{
#if 1
    unsigned char       *work1;
    unsigned char       *work2;
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
    unsigned char       *symbol_p;
    unsigned char       *malloc_p;
    unsigned char       *work_p;
    int                         sentence_len;
    if (sentence == NULL || *sentence == 0)
    return;
    sentence_len = strlen(sentence);
    // テンポラリエリアmalloc.
    malloc_p = (unsigned char*)mymalloc(sentence_len + 10);
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
    strncpy((char*)sentence, (char*)malloc_p, sentence_len);
    myfree(malloc_p);
    return;
#endif
}
//******************************************************************
// sentence文字列の、頭にcut_charがいたら、抜く。
//******************************************************************
void    cut_first_character(unsigned char *sentence, unsigned char cut_char)
{
#if 1
    unsigned char* p = sentence;
    if (sentence == NULL || *sentence == 0 ){
        return;
    }
    // 削除対象キャラクターがあるかぎり進める。
    while ((*p == cut_char) && *p){
        p++;
    }
    // sentence書き換え
    if( p != sentence ){
        strcpy((char*)sentence, (char*)p);
    }
    return;
#else
    unsigned char       *malloc_p;
    unsigned char       *work_p;
    int                         sentence_len;
    if (sentence == NULL || *sentence == 0 )
    return;
    sentence_len = strlen(sentence);
    // テンポラリエリアmalloc.
    //FILE* wws;
    //wws = fopen("c:\\temp.log","w");fprintf( wws, "IN\n" );fflush( wws );fclose( wws );
    malloc_p = (unsigned char*)mymalloc(1024);//sentence_len + 10);
    //wws = fopen("c:\\temp.log","w");fprintf( wws, "OUT\n" );fflush( wws );fclose( wws );
    if (malloc_p == NULL)
    return;
    strncpy((char*)malloc_p, (char*)sentence, sentence_len + 10);
    work_p = malloc_p;
    // 削除対象キャラクターがあるかぎり進める。
    while ((*work_p == cut_char) && (*work_p != '\0')){
        work_p++;
    }
    // sentence書き換え
    strncpy((char*)sentence, (char*)work_p, sentence_len);
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
void duplex_character_to_unique(unsigned char *sentence, unsigned char unique_char)
{
#if 1
    unsigned char       *p1;
    unsigned char       *p2;
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
    unsigned char       *source_p, *work_p;
    unsigned char       *work_malloc_p;
    unsigned char       unique_char_count = 0;
    int                         org_sentence_len;
    if (sentence == NULL || *sentence == 0)
    return;
    // オリジナル文字列長を保存。
    org_sentence_len = strlen(sentence);
    // ワークバッファ確保。
    work_malloc_p = (unsigned char*)mymalloc( org_sentence_len+10 );
    if ( work_malloc_p == NULL )
    return;
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
    strncpy((char*)sentence, (char*)work_malloc_p, org_sentence_len );
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
int sentence_split(unsigned char *sentence, unsigned char cut_char, unsigned char *split1, unsigned char *split2)
{
#if 1
    unsigned char *p = sentence;
    unsigned char *pos;
    // エラーチェック。
    if (sentence == NULL|| *sentence == 0 ||
    split1 == NULL ||
    split2 == NULL ){
        return 1;       // 引数にNULLまじり。
    }
    pos  = (unsigned char*)strchr( (char*)sentence, cut_char );
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
    unsigned char       *p;
    unsigned char       *malloc_p;
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
    malloc_p = (unsigned char*)mymalloc(sentence_len + 10);
    if (malloc_p == NULL){
        // malloc 失敗。エラー。
        return 1;
    }
    // sentence文字列をワークにコピー。
    strncpy((char*)malloc_p, (char*)sentence, sentence_len + 10);
    // sentence 内に、cut_char が有るかチェック。無ければエラー。
    p = (unsigned char*)strchr((char*)malloc_p, cut_char);
    if (p == NULL){
        myfree(malloc_p);
        return 1;       // 分割文字発見できず。
    }
    // cut_charより、後ろをカット。
    *p = '\0';
    // 前半部分をコピー。
    strncpy((char*)split1, (char*)malloc_p, sentence_len);
    // 後半部分をコピー。
    p++;
    strncpy((char*)split2, (char*)p, sentence_len);
    myfree(malloc_p);
    return 0; // 正常終了。
#endif
}
char* ExtractFileExtension( unsigned char* filename )
{
    static unsigned char buf[4];
    filename_to_extension(filename,buf,sizeof(buf));
    return (char*)buf;
}
//******************************************************************
// filenameから、拡張子を取り出す('.'も消す）
// '.'が存在しなかった場合、拡張子が長すぎた場合は、""が入る。
//******************************************************************
void filename_to_extension(unsigned char *filename, unsigned char *extension_buf, unsigned int extension_buf_size)
{
    unsigned char       *p;
    // 拡張子の存在チェック。
    p = (unsigned char*)strrchr((char*)filename, '.' );
    if (( p == NULL ) || ( strlen(p) > extension_buf_size )){
        strncpy((char*)extension_buf, "", extension_buf_size );
        return;
    }
    // 拡張子を切り出し。
    p++;
    strncpy( (char*)extension_buf, (char*)p, extension_buf_size );
    return;
}
// **************************************************************************
// text_buf から、CR/LF か'\0'が現れるまでを切り出して、line_bufにcopy。
// (CR/LFはcopyされない)
// 次の行の頭のポインタをreturn。
// Errorか'\0'が現れたらNULLが戻る。
// **************************************************************************
unsigned char *buffer_distill_line(unsigned char *text_buf_p, unsigned char *line_buf_p, unsigned int line_buf_size )
{
    unsigned char       *p;
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
    strncpy((char*)line_buf_p, (char*)text_buf_p, counter);
    if ( *p == '\0' ){
        return NULL;            // バッファの最後
    }else{
        return p;               // バッファの途中
    }
}
unsigned char* myuri_encode(unsigned char* src)
{
    static unsigned char dst[1024];
    uri_encode(dst,sizeof(dst),src,sizeof(src));
    return dst;
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
int uri_encode(unsigned char *dst,  unsigned int dst_len, const unsigned char *src, unsigned int src_len)
{
    // 2004/10/01 Update start
    /*
    unsigned int idx_src;
    unsigned int idx_dst;
    int cnt;
    // 引数チェック
    if((dst == NULL) || (dst_len < 1) || (src == NULL) || (src_len < 1)){
        return 0;
    }
    cnt = 0;
    for (idx_src = idx_dst = 0 ; (idx_src < src_len) && (idx_dst < dst_len) && (src[idx_src] != '\0'); idx_src++){
        if (src[idx_src] == '.' || src[idx_src] == '_' || src[idx_src] == '-'){
            dst[idx_dst] = src[idx_src];
            idx_dst += 1;
        }else if ( src[idx_src] == ' ' ){
            dst[idx_dst] = '+';
            idx_dst += 1;
        }else if (      ( src[idx_src]  <= 0x7F ) &&
        ( src[idx_src]  != '='  ) &&
        ( src[idx_src]  != '&'  ) &&
        ( src[idx_src]  != '%'  ) &&
        ( src[idx_src]  != '#'  ) &&
        ( src[idx_src]  != '+'  ) )
        {
            dst[idx_dst] = src[idx_src];
            idx_dst += 1;
        }else{
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
    */
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
        idx_dst += sprintf((char*)&dst[idx_dst],"%%%2X",(unsigned char)(src[idx_src]));
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
int uri_decode(unsigned char *dst, unsigned int dst_len, const unsigned char *src, unsigned int src_len)
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
//ptr_stop = (char*)src;
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
        dst[idx_dst] = (unsigned char)code;
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
void make_datetime_string(unsigned char *sentence)
{
time_t                              now;
struct tm                   *tm_p;
// 現在時刻Get.
time(&now);
tm_p = localtime(&now);
sprintf((char*)sentence,    "%04d/%02d/%02d %02d:%02d:%02d",
tm_p->tm_year+1900  ,       // 年
tm_p->tm_mon+1      ,               // 月
tm_p->tm_mday       ,               // 日
tm_p->tm_hour       ,               // 時刻
tm_p->tm_min        ,               // 分
tm_p->tm_sec                );      // 秒
return;
}
/********************************************************************************/
// time_t から、"YYYY/MM/DD HH:MM" 形式の文字列を生成する。
/********************************************************************************/
void conv_time_to_string(unsigned char *sentence, time_t conv_time)
{
struct tm                   *tm_p;
if (conv_time == 0) {
    sprintf((char*)sentence, "--/--/-- --:--");
    return ;
}
tm_p = localtime(&conv_time);
sprintf((char*)sentence,    "%04d/%02d/%02d %02d:%02d",
tm_p->tm_year+1900  ,       // 年
tm_p->tm_mon+1      ,               // 月
tm_p->tm_mday       ,               // 日
tm_p->tm_hour       ,               // 時刻
tm_p->tm_min                );      // 分
return;
}
/********************************************************************************/
// time_t から、"YYYY/MM/DD" 形式の文字列を生成する。
/********************************************************************************/
void conv_time_to_date_string(unsigned char *sentence, time_t conv_time)
{
struct tm                   *tm_p;
if (conv_time == 0) {
    sprintf((char*)sentence, "--/--/--");
    return ;
}
tm_p = localtime(&conv_time);
sprintf((char*)sentence,    "%04d/%02d/%02d",
tm_p->tm_year+1900  ,       // 年
tm_p->tm_mon+1      ,               // 月
tm_p->tm_mday               );      // 日
return;
}
/********************************************************************************/
// time_t から、"HH:MM" 形式の文字列を生成する。
/********************************************************************************/
void conv_time_to_time_string(unsigned char *sentence, time_t conv_time)
{
struct tm                   *tm_p;
if (conv_time == 0) {
    sprintf((char*)sentence, "--:--");
    return ;
}
tm_p = localtime(&conv_time);
sprintf((char*)sentence,    "%02d:%02d",
tm_p->tm_hour       ,               // 時刻
tm_p->tm_min                );      // 分
return;
}
/********************************************************************************/
// 100000000 → "100.00 MB" への変換を行う。
//
//      K,M,G に対応。
/********************************************************************************/
void conv_num_to_unit_string(unsigned char *sentence, u_int64_t file_size)
{
u_int64_t   real_size;
u_int64_t   little_size;
if ( file_size < 1024 ){
    sprintf((char*)sentence, "%lu B", file_size );
}else if ( file_size < (1024 * 1024) ){
    real_size       = file_size / 1024;
    little_size = (file_size * 100 / 1024) % 100;
    sprintf((char*)sentence, "%lu.%02lu KB", real_size, little_size);
}else if ( file_size < (1024 * 1024 * 1024) ){
    real_size       = file_size / ( 1024*1024 );
    little_size = (file_size * 100 / (1024*1024)) % 100;
    sprintf((char*)sentence, "%lu.%02lu MB", real_size, little_size);
}else{
    real_size       = file_size / ( 1024*1024*1024 );
    little_size = (file_size * 100 / (1024*1024*1024)) % 100;
    sprintf((char*)sentence, "%lu.%02lu GB", real_size, little_size);
}
return;
}
//*******************************************************************
// デバッグ出力初期化(ファイル名セット)関数
// この関数を最初に呼ぶまでは、デバッグログは一切出力されない。
//*******************************************************************
void debug_log_initialize(const unsigned char *set_debug_log_filename)
{
    // 引数チェック
    if (set_debug_log_filename == NULL)
    return;
    if ( strlen(set_debug_log_filename) == 0 )
    return;
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
    FILE	*fp;
    unsigned char	buf[1024*5+1];
    unsigned char	work_buf[1024*4+1];
    unsigned char	date_and_time[32];
    unsigned char	replace_date_and_time[48];
    va_list 	arg;
    int		len;
    // =========================================
    // デバッグログ 初期化フラグをチェック
    // =========================================
    if (debug_log_initialize_flag != 0 )
    return;
    // =========================================
    // Debug出力文字列生成。
    // 行頭に、date_and_time を挿入しておく
    // =========================================
    memset(buf, '\0', sizeof(buf));
    memset(work_buf, '\0', sizeof(work_buf));
    // 引数で与えられた文字列を展開。
    va_start(arg, fmt);
    vsnprintf((char*)work_buf, sizeof(work_buf), fmt, arg);
    va_end(arg);
    // work_bufの一番最後に'\n'がついていたら削除。
    len = strlen(work_buf);
    if (work_buf[len-1] == '\n')
    {
        work_buf[len-1] = '\0';
    }
    // 挿入用文字列生成( "\ndate_and_time" になる)
    make_datetime_string(date_and_time);
    snprintf(replace_date_and_time, sizeof(replace_date_and_time), "\n%s[%d] ", date_and_time, getpid() );
    // 出力文字列生成開始。
    snprintf(buf, sizeof(buf), "%s[%d] %s", date_and_time, getpid(), work_buf);
    replace_character(buf, (unsigned char*)"\n", replace_date_and_time);	// \nの前にdate_and_timeを挿入
    // 一番最後に'\n'をつける。
    strncat(buf, "\n", sizeof(buf)-strlen(buf) );
    // =====================
    // ログファイル出力
    // =====================
    // ファイルオープン（追記モード)
    fp = fopen((char*)debug_log_filename, "a");
    if ( fp == NULL )
    return;
    // 出力
    fwrite(buf, 1, strlen(buf), fp );	// メッセージ実体を出力
    fflush(fp);
    // ファイルクローズ
    fclose( fp );
    return;
}
// **************************************************************************
// 拡張子変更処理。追加用。
//	extension_convert_listに従い、org → rename への変換を行う。
//
// 例) "hogehoge.m2p" → "hogehoge.m2p.mpg"
// **************************************************************************
void extension_add_rename(unsigned char *rename_filename_p, size_t rename_filename_size)
{
    int i;
    unsigned char	ext[FILENAME_MAX];
    if ( rename_filename_p == NULL )
    return;
    filename_to_extension(rename_filename_p, ext, sizeof(ext));
    if (strlen(ext) <= 0) return;
    for ( i=0; extension_convert_list[i].org_extension != NULL; i++ )
    {
        debug_log_output("org='%s', rename='%s'"
        , extension_convert_list[i].org_extension
        , extension_convert_list[i].rename_extension);
        // 拡張子一致？
        if ( strcasecmp(ext, extension_convert_list[i].org_extension) == 0 )
        {
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
//	extension_convert_listに従い、rename → org への変換を行う。
//
// 例) "hogehoge.m2p.mpg" → "hogehoge.m2p"
// **************************************************************************
void extension_del_rename(unsigned char *rename_filename_p, size_t rename_filename_size)
{
    int i;
    unsigned char	renamed_ext[FILENAME_MAX];
    unsigned char	ext[FILENAME_MAX];
    if ( rename_filename_p == NULL )
    return;
    for ( i=0; extension_convert_list[i].org_extension != NULL; i++ )
    {
        debug_log_output("org='%s', rename='%s'"
        , extension_convert_list[i].org_extension
        , extension_convert_list[i].rename_extension);
        snprintf(renamed_ext, sizeof(renamed_ext), ".%s.%s"
        , extension_convert_list[i].org_extension
        , extension_convert_list[i].rename_extension);
        // 比較する拡張子と同じ長さにそろえる。
        strncpy(ext, rename_filename_p, sizeof(ext));
        cat_before_n_length(ext, strlen(renamed_ext));
        // 拡張子一致？
        if ( strcasecmp(ext, renamed_ext) == 0 )
        {
            debug_log_output(" HIT!!!" );
            // 拡張子を「削除」
            cat_after_n_length(rename_filename_p
            , strlen(extension_convert_list[i].rename_extension) + 1);
            debug_log_output("rename_filename_p='%s'", rename_filename_p);
            break;
        }
    }
    return;
}
// **************************************************************************
// * EUC文字列中の半角文字を全角にする。
// **************************************************************************
void han2euczen(unsigned char *src, unsigned char *dist, int dist_size)
{
    unsigned char	*dist_p, *src_p;
    dist_p 	= dist;
    src_p	= src;
    while ( *src_p != '\0')
    {
        switch( (int)*src_p )
        {
        case ' ':	strncpy(dist_p, "  ", dist_size);	dist_p+=2;	break;
        case '!':	strncpy(dist_p, "！", dist_size);	dist_p+=2;	break;
        case '"':	strncpy(dist_p, "”", dist_size);	dist_p+=2;	break;
        case '#':	strncpy(dist_p, "＃", dist_size);	dist_p+=2;	break;
        case '$':	strncpy(dist_p, "＄", dist_size);	dist_p+=2;	break;
        case '%':	strncpy(dist_p, "％", dist_size);	dist_p+=2;	break;
        case '&':	strncpy(dist_p, "＆", dist_size);	dist_p+=2;	break;
        case '\'':	strncpy(dist_p, "’", dist_size);	dist_p+=2;	break;
        case '(':	strncpy(dist_p, "（", dist_size);	dist_p+=2;	break;
        case ')':	strncpy(dist_p, "）", dist_size);	dist_p+=2;	break;
        case '*':	strncpy(dist_p, "＊", dist_size);	dist_p+=2;	break;
        case '+':	strncpy(dist_p, "＋", dist_size);	dist_p+=2;	break;
        case ',':	strncpy(dist_p, "，", dist_size);	dist_p+=2;	break;
        case '-':	strncpy(dist_p, "ー", dist_size);	dist_p+=2;	break;
        case '.':	strncpy(dist_p, "．", dist_size);	dist_p+=2;	break;
        case '/':	strncpy(dist_p, "／", dist_size);	dist_p+=2;	break;
        case '0':	strncpy(dist_p, "０", dist_size);	dist_p+=2;	break;
        case '1':	strncpy(dist_p, "１", dist_size);	dist_p+=2;	break;
        case '2':	strncpy(dist_p, "２", dist_size);	dist_p+=2;	break;
        case '3':	strncpy(dist_p, "３", dist_size);	dist_p+=2;	break;
        case '4':	strncpy(dist_p, "４", dist_size);	dist_p+=2;	break;
        case '5':	strncpy(dist_p, "５", dist_size);	dist_p+=2;	break;
        case '6':	strncpy(dist_p, "６", dist_size);	dist_p+=2;	break;
        case '7':	strncpy(dist_p, "７", dist_size);	dist_p+=2;	break;
        case '8':	strncpy(dist_p, "８", dist_size);	dist_p+=2;	break;
        case '9':	strncpy(dist_p, "９", dist_size);	dist_p+=2;	break;
        case ':':	strncpy(dist_p, "：", dist_size);	dist_p+=2;	break;
        case '<':	strncpy(dist_p, "＜", dist_size);	dist_p+=2;	break;
        case '=':	strncpy(dist_p, "＝", dist_size);	dist_p+=2;	break;
        case '>':	strncpy(dist_p, "＞", dist_size);	dist_p+=2;	break;
        case '?':	strncpy(dist_p, "？", dist_size);	dist_p+=2;	break;
        case '@':	strncpy(dist_p, "＠", dist_size);	dist_p+=2;	break;
        case 'A':	strncpy(dist_p, "Ａ", dist_size);	dist_p+=2;	break;
        case 'B':	strncpy(dist_p, "Ｂ", dist_size);	dist_p+=2;	break;
        case 'C':	strncpy(dist_p, "Ｃ", dist_size);	dist_p+=2;	break;
        case 'D':	strncpy(dist_p, "Ｄ", dist_size);	dist_p+=2;	break;
        case 'E':	strncpy(dist_p, "Ｅ", dist_size);	dist_p+=2;	break;
        case 'F':	strncpy(dist_p, "Ｆ", dist_size);	dist_p+=2;	break;
        case 'G':	strncpy(dist_p, "Ｇ", dist_size);	dist_p+=2;	break;
        case 'H':	strncpy(dist_p, "Ｈ", dist_size);	dist_p+=2;	break;
        case 'I':	strncpy(dist_p, "Ｉ", dist_size);	dist_p+=2;	break;
        case 'J':	strncpy(dist_p, "Ｊ", dist_size);	dist_p+=2;	break;
        case 'K':	strncpy(dist_p, "Ｋ", dist_size);	dist_p+=2;	break;
        case 'L':	strncpy(dist_p, "Ｌ", dist_size);	dist_p+=2;	break;
        case 'M':	strncpy(dist_p, "Ｍ", dist_size);	dist_p+=2;	break;
        case 'N':	strncpy(dist_p, "Ｎ", dist_size);	dist_p+=2;	break;
        case 'O':	strncpy(dist_p, "Ｏ", dist_size);	dist_p+=2;	break;
        case 'P':	strncpy(dist_p, "Ｐ", dist_size);	dist_p+=2;	break;
        case 'Q':	strncpy(dist_p, "Ｑ", dist_size);	dist_p+=2;	break;
        case 'R':	strncpy(dist_p, "Ｒ", dist_size);	dist_p+=2;	break;
        case 'S':	strncpy(dist_p, "Ｓ", dist_size);	dist_p+=2;	break;
        case 'T':	strncpy(dist_p, "Ｔ", dist_size);	dist_p+=2;	break;
        case 'U':	strncpy(dist_p, "Ｕ", dist_size);	dist_p+=2;	break;
        case 'V':	strncpy(dist_p, "Ｖ", dist_size);	dist_p+=2;	break;
        case 'W':	strncpy(dist_p, "Ｗ", dist_size);	dist_p+=2;	break;
        case 'X':	strncpy(dist_p, "Ｘ", dist_size);	dist_p+=2;	break;
        case 'Y':	strncpy(dist_p, "Ｙ", dist_size);	dist_p+=2;	break;
        case 'Z':	strncpy(dist_p, "Ｚ", dist_size);	dist_p+=2;	break;
        case '[':	strncpy(dist_p, "［", dist_size);	dist_p+=2;	break;
        case '\\':	strncpy(dist_p, "￥", dist_size);	dist_p+=2;	break;
        case ']':	strncpy(dist_p, "］", dist_size);	dist_p+=2;	break;
        case '^':	strncpy(dist_p, "＾", dist_size);	dist_p+=2;	break;
        case '_':	strncpy(dist_p, "  ", dist_size);	dist_p+=2;	break;
        case '`':	strncpy(dist_p, "‘", dist_size);	dist_p+=2;	break;
        case 'a':	strncpy(dist_p, "ａ", dist_size);	dist_p+=2;	break;
        case 'b':	strncpy(dist_p, "ｂ", dist_size);	dist_p+=2;	break;
        case 'c':	strncpy(dist_p, "ｃ", dist_size);	dist_p+=2;	break;
        case 'd':	strncpy(dist_p, "ｄ", dist_size);	dist_p+=2;	break;
        case 'e':	strncpy(dist_p, "ｅ", dist_size);	dist_p+=2;	break;
        case 'f':	strncpy(dist_p, "ｆ", dist_size);	dist_p+=2;	break;
        case 'g':	strncpy(dist_p, "ｇ", dist_size);	dist_p+=2;	break;
        case 'h':	strncpy(dist_p, "ｈ", dist_size);	dist_p+=2;	break;
        case 'i':	strncpy(dist_p, "ｉ", dist_size);	dist_p+=2;	break;
        case 'j':	strncpy(dist_p, "ｊ", dist_size);	dist_p+=2;	break;
        case 'k':	strncpy(dist_p, "ｋ", dist_size);	dist_p+=2;	break;
        case 'l':	strncpy(dist_p, "ｌ", dist_size);	dist_p+=2;	break;
        case 'm':	strncpy(dist_p, "ｍ", dist_size);	dist_p+=2;	break;
        case 'n':	strncpy(dist_p, "ｎ", dist_size);	dist_p+=2;	break;
        case 'o':	strncpy(dist_p, "ｏ", dist_size);	dist_p+=2;	break;
        case 'p':	strncpy(dist_p, "ｐ", dist_size);	dist_p+=2;	break;
        case 'q':	strncpy(dist_p, "ｑ", dist_size);	dist_p+=2;	break;
        case 'r':	strncpy(dist_p, "ｒ", dist_size);	dist_p+=2;	break;
        case 's':	strncpy(dist_p, "ｓ", dist_size);	dist_p+=2;	break;
        case 't':	strncpy(dist_p, "ｔ", dist_size);	dist_p+=2;	break;
        case 'u':	strncpy(dist_p, "ｕ", dist_size);	dist_p+=2;	break;
        case 'v':	strncpy(dist_p, "ｖ", dist_size);	dist_p+=2;	break;
        case 'w':	strncpy(dist_p, "ｗ", dist_size);	dist_p+=2;	break;
        case 'x':	strncpy(dist_p, "ｘ", dist_size);	dist_p+=2;	break;
        case 'y':	strncpy(dist_p, "ｙ", dist_size);	dist_p+=2;	break;
        case 'z':	strncpy(dist_p, "ｚ", dist_size);	dist_p+=2;	break;
            case '{':	strncpy(dist_p, "｛", dist_size);	dist_p+=2;	break;
            case '|':	strncpy(dist_p, "｜", dist_size);	dist_p+=2;	break;
        case '}':	strncpy(dist_p, "｝", dist_size);	dist_p+=2;	break;
        default:
            *dist_p = *src_p;
            dist_p++;
            *dist_p = '\0';
            break;
        }
        src_p++;
    }
    return;
}
//******************************************************************
// EUC文字列を、n byteに削除。文字列境界を見て良きに計らう。
// もし menu_font_metric が指定されており menu_font_metric_string が
// 指定されていれば、その値に従ってちょんぎる。
//******************************************************************
void euc_string_cut_n_length(unsigned char *euc_sentence,  unsigned int n)
{
    int	euc_flag = 0;
    unsigned char	*p, *prev_p;
    int total_len = 0;
    int size = 0;
    total_len = global_param.menu_font_metric * n;
    prev_p = p = euc_sentence;
    while (*p && total_len > 0) {
        if (*p >= sizeof(global_param.menu_font_metric_string)) {
            size = global_param.menu_font_metric;
        } else {
            char ch;
            ch = global_param.menu_font_metric_string[*p];
            if (ch < 'a' || ch > 'z') {
                size = global_param.menu_font_metric;
            } else {
                size = ch - 'a';
                if (size <= 0) {
                    //debug_log_output("damn. old size: %d", size);
                    size = global_param.menu_font_metric;
                }
            }
        }
        if (total_len < size) {
            if (euc_flag == 1) {
                *prev_p = '\0';
            } else {
                *p = '\0';
            }
            return ;
        }
        if (euc_flag == 0 && (( *p >= 0xA1 ) || ( *p == 0x8E ))) {
            // EUC-Code or 半角カタカナの上位
            euc_flag = 1;
        } else {
            euc_flag = 0;
        }
        total_len -= size;
        prev_p = p++;
    }
    debug_log_output("debug: rest %d", total_len);
    *p = '\0';
    return;
}
// **************************************************************************
// * PNGフォーマットファイルから、画像サイズを得る。
// **************************************************************************
void png_size(unsigned char *png_filename, unsigned int *x, unsigned int *y)
{
    int		fd;
    unsigned char	buf[255];
    ssize_t	read_len;
    *x = 0;
    *y = 0;
    fd = open(png_filename, O_RDONLY);
    if ( fd < 0 )
    {
        return;
    }
    // ヘッダ+サイズ(0x18byte)  読む
    memset(buf, 0, sizeof(buf));
    read_len = read(fd, buf, 0x18);
    if ( read_len == 0x18)
    {
        *x = 	(buf[0x10] << 24)	+
        (buf[0x11] << 16)	+
        (buf[0x12] << 8 )	+
        (buf[0x13]);
        *y = 	(buf[0x14] << 24) 	+
        (buf[0x15] << 16)	+
        (buf[0x16] << 8 )	+
        (buf[0x17]);
    }
    close( fd );
    return;
}
// **************************************************************************
// * GIFフォーマットファイルから、画像サイズを得る。
// **************************************************************************
void gif_size(unsigned char *gif_filename, unsigned int *x, unsigned int *y)
{
    int		fd;
    unsigned char	buf[255];
    ssize_t	read_len;
    *x = 0;
    *y = 0;
    fd = open(gif_filename, O_RDONLY);
    if ( fd < 0 )
    {
        return;
    }
    // ヘッダ+サイズ(10byte)  読む
    memset(buf, 0, sizeof(buf));
    read_len = read(fd, buf, 10 );
    if ( read_len == 10)
    {
        *x = buf[6] + (buf[7] << 8);
        *y = buf[8] + (buf[9] << 8);
    }
    close( fd );
    return;
}
// **************************************************************************
// * JPEGフォーマットファイルから、画像サイズを得る。
// **************************************************************************
void  jpeg_size(unsigned char *jpeg_filename, unsigned int *x, unsigned int *y)
{
    int		fd;
    unsigned char	buf[255];
    ssize_t		read_len;
    off_t		length;
    *x = 0;
    *y = 0;
    //debug_log_output("jpeg_size: '%s'.", jpeg_filename);
    fd = open(jpeg_filename,  O_RDONLY);
    if ( fd < 0 )
    {
        return;
    }
    while ( 1 )
    {
        // マーカ(2byte)  読む
        read_len = read(fd, buf, 2);
        if ( read_len != 2)
        {
            //debug_log_output("fraed() EOF.\n");
            break;
        }
        // Start of Image.
        if (( buf[0] == 0xFF ) && (buf[1] == 0xD8))
        {
            continue;
        }
        // Start of Frame 検知
        if (( buf[0] == 0xFF ) && ( buf[1] >= 0xC0 ) && ( buf[1] <= 0xC3 )) // SOF 検知
        {
            //debug_log_output("SOF0 Detect.");
            // sof データ読み込み
            memset(buf, 0, sizeof(buf));
            read_len = read(fd, buf, 0x11);
            if ( read_len != 0x11 )
            {
                debug_log_output("fraed() error.\n");
                break;
            }
            *y = (buf[3] << 8) + buf[4];
            *x = (buf[5] << 8) + buf[6];
            break;
        }
        // SOS検知
        if (( buf[0] == 0xFF ) && (buf[1] == 0xDA)) // SOS 検知
        {
            //debug_log_output("Start Of Scan.\n");
            // 0xFFD9 探す。
            while ( 1 )
            {
                // 1byte 読む
                read_len = read(fd, buf, 1);
                if ( read_len != 1 )
                {
                    //debug_log_output("fraed() error.\n");
                    break;
                }
                // 0xFFだったら、もう1byte読む
                if ( buf[0] == 0xFF )
                {
                    buf[0] = 0;
                    read(fd, buf, 1);
                    // 0xD9だったら 終了
                    if ( buf[0] == 0xD9 )
                    {
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
// Samba の HEX/CAP形式の文字列を、バイナリに戻す。
// **************************************************************************
void decode_samba_hex_and_cap_coding( unsigned char *sentence )
{
    int		src_len;
    int		malloc_size;
    unsigned char	*malloc_p;
    unsigned char	*src_p, *dist_p;
    char			work[3];
    long int		hex_code;
    int	src_idx, dist_idx;
    src_len = strlen(sentence);
    malloc_size = src_len + 32;
    malloc_p = (unsigned char*)malloc( malloc_size );
    if ( malloc_p == NULL )
    {
        debug_log_output("decode_cap: malloc_error.");
        return;
    }
    memset( malloc_p, '\0', malloc_size ) ;
    src_idx = 0;
    dist_idx = 0;
    src_p = sentence;
    dist_p = malloc_p;
    while ( src_p[src_idx] != '\0' )
    {
        if (src_p[src_idx] == ':') // ':'発見
        {
            if (src_idx+2 < src_len) // あと２文字分ある？
            {
                // ':' の後ろ２文字の文字チェック
                if (	(((src_p[src_idx+1] >= '0') && (src_p[src_idx+1] <= '9')) ||
                ((src_p[src_idx+1] >= 'A') && (src_p[src_idx+1] <= 'F')) ||
                ((src_p[src_idx+1] >= 'a') && (src_p[src_idx+1] <= 'f')) 	)	&&
                (((src_p[src_idx+2] >= '0') && (src_p[src_idx+2] <= '9')) ||
                ((src_p[src_idx+2] >= 'A') && (src_p[src_idx+2] <= 'F')) ||
                ((src_p[src_idx+2] >= 'a') && (src_p[src_idx+2] <= 'f')) 	)			)
                {
                    // ':'の後ろ2文字をworkにコピー
                    work[0] = src_p[src_idx+1];
                    work[1] = src_p[src_idx+2];
                    work[2] = '\0';
                    // 変換実行
                    hex_code = strtol(work, NULL, 16);
                    dist_p[dist_idx] = (unsigned char)(hex_code & 0xFF);
                    src_idx += 3;
                    dist_idx++;
                    dist_p[dist_idx] = '\0';
                    continue;
                }
            }
        }
        // 変換対象でなければ、１文字コピー
        dist_p[dist_idx] = src_p[src_idx];
        dist_idx++;
        src_idx++;
        dist_p[dist_idx] = '\0';
        continue;
    }
    debug_log_output("decode_cap: result='%s'", malloc_p);
    strncpy(sentence, malloc_p, src_len);
    free ( malloc_p );
    return;
}
// **************************************************************************
// * SJIS文字列の中から、codeをを含む文字(2byte文字)を、固定文字列で置換する。
// **************************************************************************
void sjis_code_thrust_replace(unsigned char *sentence, const unsigned char code)
{
    int i;
    unsigned char rep_code[2];
    // 置換する文字列。(SJISで'＊'）
    rep_code[0] = 0x81;
    rep_code[1] = 0x96;
    // 文字列長が、1byte以下なら、処理必要なし。
    if ( strlen(sentence) <= 1 )
    {
        return;
    }
    // 置換対象文字 捜索
    for ( i=1; sentence[i] != '\0' ;i++ )
    {
        // code にヒット？
        if ( sentence[i] == code )
        {
            // 1byte前が、SJIS 1byte目の範囲？(0x81～0x9F、0xE0～～0xFC)
            if ((( sentence[i-1] >= 0x81 ) && (sentence[i-1] <= 0x9F)) ||
            (( sentence[i-1] >= 0xE0 ) && (sentence[i-1] <= 0xFC))		)
            {
                debug_log_output("SJIS Replase HIT!!!!");
                // 置換実行
                sentence[i-1] = rep_code[0];
                sentence[i  ] = rep_code[1];
            }
        }
    }
    return;
}
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
unsigned char *path_sanitize(unsigned char *orig_dir, size_t dir_size)
{
    char *p;
    unsigned char *q;
    unsigned char *dir;
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
        q = (unsigned char*)strchr(dir, '/');
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
    return orig_dir;
}
//**************************************************************************
// ファイル存在を調べる
//**************************************************************************
bool FileExists(unsigned char* str)
{
    bool flag=false;
#ifdef linux
    struct stat send_filestat;
    int  result = stat((char*)str, &send_filestat);
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
//**************************************************************************
// ファイルサイズを調べる
//**************************************************************************
off_t FileSizeByName(unsigned char* str)
{
    off_t pos;
    int handle;
    handle = open(str,O_BINARY);
    pos = lseek( handle,0,SEEK_END);
    close( handle );
    return pos;
}
//**************************************************************************
// スリープ
//**************************************************************************
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
// ソケットを作成し、相手に接続するラッパ. 失敗 = -1
//---------------------------------------------------------------------------
SOCKET sock_connect(char *host, int port)
{
    SOCKET sock;
struct sockaddr_in sockadd={0};     //ＳＯＣＫＥＴ構造体
    struct hostent *hent;
#ifndef linux
    //呪文
    WORD version = MAKEWORD(2, 0);      //
    WSADATA wsa;                        //
    WSAStartup(version, &wsa);          //
#endif
    debug_log_output("sock_connect: %s:%d", host, port);
    //ＳＯＣＫＥＴ作成
    if (SERROR(sock = socket(PF_INET, SOCK_STREAM, 0))){
        debug_log_output("sock_connect_error:");
#ifndef linux
        WSACleanup();
#endif
        return INVALID_SOCKET;
    }
    debug_log_output("sock: %d", sock);
    if (NULL == (hent = gethostbyname(host))) {
        sock_close( sock );
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
        sock_close( sock );
        return INVALID_SOCKET;
    }
    debug_log_output("Sock Connected\n");
    set_blocking_mode(sock, 0);    /* blocking */
    return sock;
}
//---------------------------------------------------------------------------
//ソケットクローズ
//後始末付き
//---------------------------------------------------------------------------
void sock_close(SOCKET sock)
{
    close(sock);
#ifndef linux
    WSACleanup();
#endif
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
#ifdef linux
void set_blocking_mode(int fd, int flag)
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
