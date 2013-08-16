// =======================================================================
//code=UTF8      tab=4
//
// wizd:        MediaWiz Server daemon.
//
//              wizd_send_file.c
//              $Revision: 1.8 $
//              $Date: 2004/05/10 23:33:39 $
//
//      すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
int loop_flag = 1;
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef linux
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <windows.h>
#include <winsock.h>
#include <io.h>
#include <process.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
//!!#include <pthread.h>
#include <signal.h>
#include "wizd.h"
#include "wizd_tools.h"
// 2004/07/12 Add
// thread 追加のため
#define BUFFER_COUNT ((global_param.buffer_size == 0 ? 1024 : global_param.buffer_size))
#define BUFFER_ATOM (10240)
#define BUFFER_ATOM2 (2048)
#ifndef linux
#define O_NONBLOCK 1
/*  fcntl のオプション */
#define O_NDELAY    0x04        /* non-blocking */
#define FNDELAY     O_NDELAY    /* synonym */
#define F_GETFL     3           /* get flags */
#define F_SETFL     4           /* set flags */
#endif
// 2004/08/12 Add start
#define GetCurrentProcessId()        getpid()
#define BUFSIZE (5000)
// 2004/08/12 Add end
// extern int errno;
// 2004/08/13 Update start
static int http_file_send(int accept_socket,
unsigned char *filename,
off_t content_length,
off_t range_start_pos );
// 2004/08/13 Update end
static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p);
void * read_buffer(void *arg);
typedef struct {
    int *flag_abort;    //中止
    int *flag_exhost;   //
    int *in_fd;         //
    int *tw_fd;
    int *flag_finish;   //終了
    off_t *readLength;    //読み取り長さ
    int *finish_demand;
    char** buffer;
    char* server;
    JOINT_FILE_INFO_T *joint_file_info_p;
    off_t* range_start_pos;
    off_t* range_end_pos;
    off_t* content_length;
} TRANS;
static char* sswork=0;
static char  buffer_name[256]={0};
static int   end_pos;
int CacheStart(TRANS* trans,pthread_t& handle);
//キャッシュ開始
#ifdef linux
pthread_t CacheStart2(TRANS* trans,int &tw_fd, int &tr_fd,char* buff,char* filename);
#else
HANDLE CacheStart2(TRANS* trans,int &tw_fd, int &tr_fd,char* buff,char* filename);
#endif
char* CacheGet(int myEndPos);
// **************************************************************************
// ファイル実体の返信。
// ヘッダ生成＆送信準備
// **************************************************************************
int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    int send_header_data_len;
    int result_len;
    char        send_http_header_buf[2048];
    char        work_buf[1024];
    off_t       content_length;
    //struct    stat    file_stat;
    //int                               result;
    //WIN32_FIND_DATA ffd;
    //DWORD dwAttr;
    // ---------------
    // 作業用変数初期化
    // ---------------
    memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));
    // -------------------------------
    // ファイルサイズチェック
    // -------------------------------
    if ( http_recv_info_p->range_end_pos > 0 )  {// end位置指定有り。
        content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
    }else{ // end位置指定無し。
        if ( ! FileExists( http_recv_info_p->send_filename ) ){             // パスが示すファイルが存在しない
            return ( -1 );
        }
        content_length = FileSizeByName(http_recv_info_p->send_filename) - http_recv_info_p->range_start_pos;// ファイルサイズチェック。
    }
    // --------------
    // OK ヘッダ生成
    // --------------
    sprintf( work_buf,HTTP_SERVER_NAME, SERVER_NAME);
    sprintf( send_http_header_buf, "%s%s%sAccept-Ranges: bytes\r\n;",
    HTTP_OK,
    HTTP_CONNECTION,
    work_buf);
    
    if (content_length) {
        sprintf( work_buf, HTTP_CONTENT_LENGTH, content_length);
        sprintf( send_http_header_buf+strlen(send_http_header_buf), "%s" ,  work_buf);
    }
    sprintf( work_buf,HTTP_CONTENT_TYPE, http_recv_info_p->mime_type);
    sprintf( send_http_header_buf+strlen(send_http_header_buf), "%s"HTTP_END ,  work_buf);
    send_header_data_len = strlen(send_http_header_buf);
    // --------------
    // ヘッダ返信
    // --------------
    result_len = send(accept_socket, send_http_header_buf, send_header_data_len, 0);
    // --------------
    // 実体返信
    // --------------
    http_file_send(accept_socket,
    http_recv_info_p->send_filename,
    content_length,
    http_recv_info_p->range_start_pos);
    return 0;
}
// **************************************************************************
// ファイルの実体の送信実行部
// **************************************************************************
static int http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
    int         fd;
    off_t       seek_ret;
    // ---------------------
    // ファイルオープン
    // ---------------------
    fd = open(filename, O_BINARY);
    if ( fd < 0 ){
        return ( -1 );
    }
    //  printf("\nifile name=%s", filename);
    //fflush(stdout);
    // ------------------------------------------
    // range_start_posへファイルシーク
    // ------------------------------------------
    if( range_start_pos ){
        seek_ret = lseek(fd, range_start_pos, SEEK_SET);
        if ( seek_ret < 0 )     {// lseek エラーチェック
            close(fd);
            return ( -1 );
        }
    }
    //実態を転送する
    copy_descriptors(fd, accept_socket, content_length, (JOINT_FILE_INFO_T*)-1, (char*)filename,range_start_pos);//NULL);
    close(fd);  // File Close.
    // 正常終了
    return 0;
}
/*
* バッファリングしながら in_fd から out_fd へ データを転送
* 書き込み側を NONBLOCK モードに設定することによって
* write(send?) 中にブロックするのを防ぐ。
* こうすることで、読み込みができる場合にはさらにキャッシュにデータを
* ためることができる。
*
* 今のところ上のファイル実体送信と、プロクシから呼び出している.
* content_length をあまり重要視していない..
*
* vobの連続再生に対応のため、複数ファイルの連続転送に対応
*
*/
int copy_descriptors(int in_fd,
int out_fd,
off_t content_length,
JOINT_FILE_INFO_T *joint_file_info_p,
char* infilename,
off_t range_start_pos)
{
    //int i;
    unsigned char buffp1[BUFFER_ATOM];
    //char filename[256];
    TRANS trans;
    int tw_fd = 0;                      //一時書き込み用ファイルディスクリプタ
    int tr_fd;                  //一時読み込み用ファイルディスクリプタ
    int len;
    int buf_len = 0;
    int buf_ptr = 0;
    int flag_finish = 0;
    int flag_exhost = 0;
    off_t readLength = 0;
    off_t writeLength = 0;
    int transmode = 0;
    int flag_abort = 0;
    // 2004/07/12 Add start
    /* スレッド用パラメータ */
    //DWORD id;
    int rbgn_time = -1;
    int wbgn_time = -1;
    trans.flag_abort  = &flag_abort;
    trans.flag_finish = &flag_finish;
    trans.flag_exhost = &flag_exhost;
    trans.readLength = &readLength;
    trans.joint_file_info_p = joint_file_info_p;
    trans.in_fd = &in_fd;
    trans.tw_fd = &tw_fd;
    trans.range_start_pos = &range_start_pos;
    //strcpy( buff, current_dir.c_str() );
    //2004.10.03 カレントディレクトリをとってはいけない
    //sprintf( filename, "%s%s_wizd_temp%d.dat" , buff , DELIMITER, getpid() );
    // 2004/08/12 Add end
#ifdef linux
    //シグナルハンドラ
    if( signal(SIGPIPE,SIG_IGN) == SIG_ERR ){
        debug_log_output("signal");
        exit(1);
    }
    // ブロックモードの設定
    set_blocking_mode(in_fd, 0);    /* blocking */
    set_blocking_mode(out_fd, 0);   /* blocking */
    debug_log_output("set blocking mode");
#else
    debug_log_output("set non-blocking mode");
#endif
    //ファイル転送のとき
    if( joint_file_info_p == (JOINT_FILE_INFO_T*)-1 ){
        readLength = content_length;
        flag_finish = 1;
        tr_fd = in_fd;
        transmode = 2;
        if(  readLength == 0 ){
            close( tw_fd );
            close( tr_fd );
            return -1;
        }
        //小さいデータ転送のとき
    }else{
        // if( content_length < BUFFER_ATOM ){
            readLength = content_length;
            flag_finish = 1;
            tr_fd = in_fd;
            transmode = 1;
        }
        // ================
        // 実体転送開始
        // ================
        while ( ! flag_abort ){
            if (readLength > 0 ){
                for(;;){
                    //printf("\nsend: len=%6d, readL=%d, writeL = %d", buf_len,readLength,writeLength );
                    //前回読み込んだバッファを吐き出す
                    if( buf_len > 0 ){
                        //書き込む
                        len = send( out_fd , (buffp1 + buf_ptr) , buf_len , 0 );
                        // 2004/11/08 Update end
                        if( len == 0 ){
                            //指定時刻書き込めなかったら落ちる
                            if( time( NULL ) > rbgn_time ){
                                break;
                            }
                            // 2004/08/23 Add end
                            continue;
                        }
                        //SIGPIPEがきたら終わる
                        if( SERROR( len ) ){
                            flag_abort = 1;
                            break;
                        }
                        buf_len -= len;
                        buf_ptr += len;
                        writeLength += len;
                        // 2004/08/23 Add start
                        //開始時刻を３０秒後に設定
                        rbgn_time = time( NULL ) + NO_RESPONSE_TIMEOUT;
                        // 2004/08/23 Add end
                        //    printf( "\nBR:len=%d ptr=%d  read=%d write=%d", buf_len, buf_ptr, readLength, writeLength );
                        //バッファを読み込む
                    }else if (readLength > writeLength ){
                        buf_ptr = 0;
                        buf_len = 0;
                        len = readLength-writeLength;
                        if( len > BUFFER_ATOM ){
                            len = BUFFER_ATOM;
                        }
                        //2004/07/08 Update start
                        len = read( tr_fd , buffp1 , len);
                        //読めない。５秒かけなかったら終わり。
                        if( len <= 0 ){
                            // printf("\nread erro:len=%d readLen=%d writeLen=%d", len,readLength,writeLength);
                            if( time( NULL ) > wbgn_time ){
                                flag_abort = true;
                            }
                            break;
                        }
                        wbgn_time = time( NULL ) + 5000;
                        buf_len = len;
                        //    len = write(out_fd, buffp1,buf_len);
                        // 2004/11/05 Update start
                        //                    len = send( out_fd , buffp1 , buf_len , 0 );
                        len = send( out_fd , buffp1 , buf_len , 0 );
                        // 2004/11/05 Update end
                        //printf( "\nBL:len=%d ptr=%d  read=%d write=%d", buf_len, buf_ptr, readLength, writeLength );
                        if( len == 0 ){
                            //                                      printf("\nwrite error with write2");
                            continue;
                        }
                        //SIGPIPEがきたら終わる
                        if( SERROR( len ) ){
                            flag_abort = 1;
                            break;
                        }
                        buf_ptr = len;
                        buf_len -= len;
                        writeLength += len;
                        //終わり
                    }else{
                        if( flag_finish == 1 ){
                            flag_abort = 1;
                        }
                        break;
                    }
                }
                //スレッドが終わっても読み込みサイズが０なら終わり
                //または最初からサイズが０固定なら
            }else if ( transmode == 2 || flag_exhost ){
                break;
                //来る可能性があるなら寝るか
            }else{
                Sleep(50);
            }
        }
        //スレッドは作成されているのでjoinする
        switch( transmode ){
        case 0:
        case 1:
            close( tw_fd );
            close( tr_fd );
            break;
        case 2:
#ifdef linux
            break;
#else
            WaitForSingleObject( handle , INFINITE );
            CloseHandle( handle );
#endif
            break;
        default:
            break;
        }
        // 正常終了
        return 0;
    }
    /////////////////////////////////////////////////////////////////////////////////////////
    void * read_descriptors(void *arg)
    {
        int rbgn_time = time(NULL)+NO_RESPONSE_TIMEOUT;
        int ptr;
        int len;
        unsigned char buffp2[BUFFER_ATOM2];
        TRANS* trans = (TRANS*)arg;
        //出力ファイルの設定
        // ================
        // 実体転送開始
        // ================
        while ( (! *trans->flag_abort) && loop_flag  ){
            len = recv( *trans->in_fd , buffp2 , BUFFER_ATOM2 , 0 );
            if (len == 0) {
                if (*trans->readLength >= *trans->content_length ){
                    //読み終わった。
                    if (next_file(trans->in_fd, trans->joint_file_info_p)){
                        *trans->flag_finish = 1;
                        break;
                    }
                    continue;
                    //指定時刻書き込めなかったら落ちる
                }else if( time( NULL ) > rbgn_time ){
                    *trans->flag_finish = 1;
                    //printf("break by time\n");
                    break;
                }
                // 2004/08/23 Add end
            } else if (len > 0) {
                ptr = write(*trans->tw_fd, buffp2,len);
                *trans->readLength += ptr;
                //開始時刻を３０秒後に設定
                rbgn_time = time(NULL)+NO_RESPONSE_TIMEOUT;
            }else{
                break;
            }
        }
        // スレッド終了
        *trans->flag_exhost = 1;
#ifndef linux
        ExitThread(TRUE);
#endif
        return NULL;
    }
    /////////////////////////////////////////////////////////////////////////////////////////
    void * read_buffer(void *arg)
    {
        char        HOST_NAME[256];
        char        temp[1024];
        char        **tmp;                          //バッファ
        char        *buf;                           //バッファ
        int         recv_len;                       //読み取り長さ
        SOCKET      server_socket;                  //サーバーソケット
        int         len=0;
        int         content_length;
        char        *pp;
        int         work1;
        TRANS* trans = (TRANS*)arg;
        //出力ファイルの設定
        // ================
        // 実体転送開始
        // ================
        tmp = trans->buffer;
        //準備
        strcpy( HOST_NAME, WIZDROOT2);
        //Servert Socket
        server_socket = sock_connect(HOST_NAME, 80);//socket( PF_INET , SOCK_STREAM , 0 );
        if ( ! SERROR(server_socket) ){
            //
            sprintf(temp ,
            "GET %s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\nConnection: close\r\n\r\n" ,
            uri_encode(trans->server) ,
            USERAGENT ,
            HOST_NAME );
            //"Range: bytes=%u-\r\n"
            // 0 );
            //サーバに繋がった
            if( ! SERROR( send( server_socket, temp, strlen( temp ) , 0) ) ){
                recv_len = recv(server_socket, temp, sizeof(temp)-1, 0);
                //ない場合…エラー
                if( recv_len > 0 ){
                    temp[recv_len] = 0;
                    //見つからない
                    work1 = atoi(strchr(temp,' ')+1);
                    if( work1 < 200 || 300 <= work1 ){
                        sock_close(server_socket);
                        buffer_name[0] = 0;
                        return NULL;
                    }
                    pp = strstr(temp,"Content-Length:" );
                    if( pp == NULL ){
                        sock_close(server_socket);
                        buffer_name[0] = 0;
                        return NULL;
                    }
                    pp += 16;
                    content_length = atoi(pp);
                    *tmp = (char*)malloc(content_length );
                    buf = *tmp;
                    sswork = buf;
                    //\r\n\r\nを探す
                    //buf[recv_len] = 0;
                    //if(
                    pp = strstr(temp,"\r\n\r\n" );
                    recv_len -= (pp+4-temp);
                    memcpy( buf, pp+4,recv_len );
                    buf += recv_len;
                    len += recv_len;
                    while( loop_flag  ){
                        recv_len = recv(server_socket, buf, 1024, 0);
                        if ( recv_len <= 0 ){
                            break;
                        }
                        buf += recv_len;
                        len += recv_len;
                        *trans->readLength = len;
                    }
                }else{
                    //error
                }
            }
            sock_close(server_socket);
        }
        // スレッド終了
        *trans->readLength = len;
        *trans->flag_exhost = 1;
#ifndef linux
        ExitThread(TRUE);
#endif
        return NULL;
    }
    /////////////////////////////////////////////////////////////////////////////////////////
    static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p)
    {
        if (in_fd_p && joint_file_info_p){
            // 読み終わったファイルをCLOSE()
            close(*in_fd_p);
            // 次のファイルがあるか?
            joint_file_info_p->current_file_num++;
            if ( joint_file_info_p->current_file_num >= joint_file_info_p->file_num ){
                return 1;           // これで終了
            }
            // 次のファイルをOPEN()
            *in_fd_p = open(joint_file_info_p->file[joint_file_info_p->current_file_num].name, O_BINARY);
            if ( *in_fd_p < 0 ){
                return ( -1 );
            }
            // ブロックモードの設定
#ifdef linux
            set_blocking_mode(*in_fd_p, 0); /* blocking */
#endif
            return 0;               // 次のファイルの準備完了
        } else {
            // パラメータがNULLの場合には1ファイルのみの処理とする
            return 1;               // これで終了
        }
    }
    int copy_descriptors2(int in_fd,
    int out_fd,
    off_t content_length,
    JOINT_FILE_INFO_T *joint_file_info_p,
    char* infilename,
    off_t range_start_pos)
    {
        //int i;
        unsigned char buffp1[BUFFER_ATOM];
        char filename[256];
        int tw_fd = 0;                      //一時書き込み用ファイルディスクリプタ
        int tr_fd;                  //一時読み込み用ファイルディスクリプタ
        int len;
        //int count=0;
        int buf_len = 0;
        int buf_ptr = 0;
        int flag_finish = 0;
        int flag_exhost = 0;
        off_t readLength = 0;
        int writeLength = 0;
        int transmode = 0;
        TRANS trans;
    char buff[1024]={0};
        int flag_abort = 0;
        // 2004/07/12 Add start
        /* スレッド用パラメータ */
#ifdef linux
        pthread_t  handle=0;
#else
#endif
        //    DWORD id;
        int rbgn_time = -1;
        int wbgn_time = -1;
        int thread_flag = 0;
        //見ている
        trans.flag_abort  = &flag_abort;
        trans.flag_finish = &flag_finish;
        trans.flag_exhost = &flag_exhost;
        trans.readLength = &readLength;
        trans.content_length = &content_length;
        trans.joint_file_info_p = joint_file_info_p;
        trans.in_fd = &in_fd;
        trans.tw_fd = &tw_fd;
        trans.server= infilename;
        trans.range_start_pos = &range_start_pos;
        //memset( buff, 0, sizeof( buff ) );
        //::GetCurrentDirectory( sizeof( buff ), buff );
        strcpy( buff, "/opt/sybhttpd/localhost.drives/HARD_DISK/wizdlive/work" );
        //2004.10.03 カレントディレクトリをとってはいけない
        sprintf( filename, "%s%s_wizd_temp%d.dat" , buff , DELIMITER, getpid() );
#ifdef linux
        //シグナルハンドラ
        if( signal(SIGPIPE,SIG_IGN) == SIG_ERR ){
            exit(1);
        }
        // ブロックモードの設定
        set_blocking_mode(in_fd, 0);    /* blocking */
        set_blocking_mode(out_fd, 0);   /* blocking */
#else
#endif
        //ファイル転送のとき
        if( (int)joint_file_info_p == -1 ){
            //              printf("\nfile");
            readLength = content_length;
            flag_finish = 1;
            tr_fd = in_fd;
            transmode = 2;
            //小さいデータ転送のとき
        }else if( content_length < BUFFER_ATOM*2 ||
        strcmp(ExtractFileExtension((unsigned char*)trans.server),"jpg") == 0 ||
        strcmp(ExtractFileExtension((unsigned char*)trans.server),"png") == 0){
            readLength = content_length;
            flag_finish = 1;
            tr_fd = in_fd;
            transmode = 1;
        }else{
            thread_flag = 1;
            handle = CacheStart2(&trans,tw_fd,tr_fd,buff,filename);
        }
        // ================
        // 実体転送開始
        // ================
        //終了フラグが立つまでループ
        while ( (! flag_abort) && loop_flag){
            if (readLength > 0 ){
                wbgn_time = 0;
                for(;;){
                    //前回読み込んだバッファを吐き出す
                    if( buf_len > 0 ){
                        len = send( out_fd , (buffp1 + buf_ptr) , buf_len , 0 );
                        if( len == 0 ){
                            //Sleep(100);
                            //指定時刻書き込めなかったら落ちる
                            if( time( NULL ) > rbgn_time ){
                                flag_abort = 1;
                                break;
                            }
                            // 2004/08/23 Add end
                            continue;
                        }
                        //SIGPIPEがきたら終わる
                        if( SERROR( len ) ){
                            flag_abort = 1;
                            break;
                        }else{
                            buf_len -= len;
                            buf_ptr += len;
                            writeLength += len;
                            //開始時刻を３０秒後に設定
                            wbgn_time = 0;
                            rbgn_time = time( NULL ) + NO_RESPONSE_TIMEOUT;
                        }
                        //バッファを読み込む
                    }else if (readLength > writeLength ){
                        buf_ptr = 0;
                        buf_len = 0;
                        len = readLength-writeLength;
                        if( len > BUFFER_ATOM ){
                            len = BUFFER_ATOM;
                        }
                        // キャッシュか否かでデータの受け取りが違う
                        if( thread_flag == 1 ){
                            len = read( tr_fd , buffp1 , len);
                        }else{
                            len = recv( tr_fd , buffp1 , len , 0 );
                        }
                        //読めなかったら終わるか？
                        //ここは一応なさそうだが、あったら必ずエラー
                        if( len == 0 || SERROR( len )){
                            flag_abort = 1;
                            break;
                        }
                        buf_len = len;
                        //終わり 読み込むものがなくて、バッファもない。
                    }else{
                        if( flag_finish == 1 ){
                            flag_abort = 1;
                        }
                        Sleep(5);
                        break;
                    }
                }
                //サイズ０で読み込みが終わってたら終わり
                //ソケット直結か、読み込み終わりの場合
            }else if ( transmode == 2 || flag_exhost ){
                break;
                //寝るか
            }else{
                Sleep(5);
                wbgn_time += 5;
                //タイムアウト５秒
                if( wbgn_time >= 5000 ){
                    break;
                }
            }
        }
        //スレッドは作成されているのでjoinする
        //transmode=0 ：ファイルバッファリング
        //transmode=1 ：そのまま読み書き
        //transmode=2 ：直結(クローズは接続先でしてもらう）
        switch( transmode ){
        case 0:
            // 2004/08/24 Add start
#ifdef linux
            pthread_join (handle, NULL);
#else
            WaitForSingleObject( handle , INFINITE );
            CloseHandle( handle );
#endif
            close( tw_fd );
            close( tr_fd );
            // 2004/08/24 Add end
            //スレッド終了対策
            //printf( "unlink %s\n", filename );
            unlink( filename );
            break;
        case 1:
            //close( tw_fd );
            //close( tr_fd );
            break;
        default:
            break;
        }
        //printf("finish\n");
        // 正常終了
        return 0;
    }
    //キャッシュ開始
    int CacheStart(TRANS* trans,pthread_t& handle)
    {
        //HANDLE handle;
        DWORD id;
        //バッファにあった
        if( strcmp(buffer_name,trans->server) == 0 ){
            return 0;
        }
        free(sswork);
        sswork = 0;
        //名前登録
        strcpy(buffer_name,trans->server);
        //
        end_pos=0;
#ifdef linux
        id = pthread_create(&handle,
        NULL,//pthread_attr_default,
        read_buffer,
        (void*)trans);
#else
        CreateThread( 0 , 0 , (LPTHREAD_START_ROUTINE)read_buffer , (void*)trans , 0 , &id );
#endif
        return 1;
    }
    //キャッシュ開始
#ifdef linux
    pthread_t CacheStart2(TRANS* trans,int &tw_fd, int &tr_fd,char* buff,char* filename)
#else
    HANDLE CacheStart2(TRANS* trans,int &tw_fd, int &tr_fd,char* buff,char* filename)
#endif
    {
        static unsigned int pid=0;
        pid++;//=getpid();
        unsigned long id;
        char work[256];
        char* ptr1;
        char* ptr2=NULL;
        //static int tw_fd_old;
#ifdef linux
        pthread_t handle;
#else
        HANDLE handle;
#endif
        //2004.10.03 カレントディレクトリをとってはいけない
        //スレッドＩＤを設定
        ptr1 = trans->server;
        while( *ptr1 ){
            if( *ptr1 == '/' ){
                ptr2 = ptr1+1;
            }
            ptr1++;
        }
        strcpy( work, ptr2 );
        //
        sprintf( filename, "/opt/sybhttpd/localhost.drives/HARD_DISK/work/wizd_temp_%d.dat", pid);//ptr2 );
        //スレッドの作成
#ifdef linux
        tw_fd = open((char*)filename,O_WRONLY | O_CREAT | O_BINARY, 0777);
        tr_fd = open(filename,O_BINARY );
        strcpy( buffer_name, filename );
        id = pthread_create(&handle,
        NULL,//pthread_attr_default,
        read_descriptors,
        (void*)trans);
#else
        tw_fd = open(filename,O_WRONLY | O_CREAT | O_BINARY, 0777);
        tr_fd = open(filename,O_BINARY );
        strcpy( buffer_name, filename );
        handle = CreateThread( 0 , 0 , (LPTHREAD_START_ROUTINE)read_descriptors , (void*)&trans , 0 , &id );
#endif
        return handle;
    }
