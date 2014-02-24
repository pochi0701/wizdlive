// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_listen.c
//											$Revision: 1.6 $
//											$Date: 2004/03/06 13:52:56 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>  
#include <error.h>
#include <cerrno>
#ifdef use_thread
#include <pthread.h>
#else
int                 epfd;                           // EPOLL FileDescriptor
#endif
#include "wizd.h"
int volatile child_count = 0;
#define MAX_CHILD_COUNT (global_param.max_child_count)
typedef struct {
    SOCKET                  accept_socket;      // SOCKET
    struct  sockaddr_in     caddr;
} ACCESS_INFO;
void* server_listen_main(void* arg);
/////////////////////////////////////////////////////////////////////////
// HTTPサーバ 待ち受け動作部
/////////////////////////////////////////////////////////////////////////
void	server_listen()
{
    int        		ret;
    int			listen_socket;			// 待ち受けSocket
    int			accept_socket;			// 接続Socket
    struct sockaddr_in  saddr;				// サーバソケットアドレス構造体
    struct sockaddr_in  caddr;				// クライアントソケットアドレス構造体
    socklen_t   	caddr_len = sizeof(caddr);	// クライアントソケットアドレス構造体のサイズ
    int			sock_opt_val;
#ifndef use_thread
    struct epoll_event  events[MAX_EVENTS];             // EPOLL EVENTS
    int                 nfds;         			// EPOLL用 
    int                 n;
#endif
    //struct timeval t_val;         			// 待ちタイマー値
    ACCESS_INFO*     ac_in;
    // =============================
    // listenソケット生成
    // =============================
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if ( SERROR( listen_socket) ){ // ソケット生成失敗チェック
        debug_log_output("socket() error.");
        perror("socket");
        return;
    }
    // ===============================
    // SO_REUSEADDRをソケットにセット
    // ===============================
    sock_opt_val = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val, sizeof(sock_opt_val));
    // ===========================================
    // ソケットアドレス構造体に値をセット
    // ===========================================
    memset( (char *)&saddr, 0, sizeof(saddr) );
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(global_param.server_port);
    // =============================
    // bind 実行
    // =============================
    ret = bind(listen_socket, (struct sockaddr *)&saddr, sizeof(saddr));
    if ( ret < 0 ){ // bind 失敗チェック
        debug_log_output("bind() error. ret=%d\n", ret);
        perror("bind");
        return;
    }
    // =============================
    // listen実行
    // =============================
    ret = listen(listen_socket, LISTEN_BACKLOG);
    if ( ret < 0 ){  // listen失敗チェック
        debug_log_output("listen() error. ret=%d\n", ret);
        perror("listen");
        return;
    }
#ifdef use_thread
    // =====================
    // BLOCKING MODE設定
    // =====================
    debug_log_output("THREAD MODE START");
    set_nonblocking_mode(listen_socket,0);//BLOCKING_MODE
    // =====================
    // メインループ
    // =====================
    pthread_t thread;
    while( 1 )
    {
        accept_socket = accept(listen_socket, (struct sockaddr *)&caddr, &caddr_len);
        if ( accept_socket < 0 ) // accept失敗チェック
        { 
            debug_log_output("accept() error. ret=%d\n", accept_socket);
            continue;           // 最初に戻る。
        }
        //set_nonblocking_mode(accept_socket,0);//BLOCKING_MODE
        ac_in = new ACCESS_INFO;
        ac_in->accept_socket = accept_socket;
        ac_in->caddr = caddr;
        debug_log_output( "thread created from %s sock %d port %d",inet_ntoa(caddr.sin_addr),accept_socket,ntohs(caddr.sin_port));
        if( pthread_create(&thread, NULL , server_listen_main , ac_in) < 0 ){
            debug_log_output( "thread create failed");
        }
        //部分的に待つ
        pthread_join(thread,NULL);
    }
#else
    // =====================
    // NON BLOCKING MODE設定
    // =====================
    set_nonblocking_mode(listen_socket,1);//NON_BLOCKING_MODE
    // =====================
    // メインループ
    // =====================
    if((epfd = epoll_create(MAX_EVENTS)) < 0) {
        debug_log_output("epoll_create()\\n");
        exit(1);
    }
    memset(&events, 0, sizeof(events));
    //queue初期化
    queue_init();
    //レベルトリガー
    add_epoll(listen_socket, EPOLLIN );
    while( 1 )
    {
        // ====================
        // Accept待ち.
        // ====================
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            continue;
            exit(EXIT_FAILURE);
        }
        //可能なファイルディスクリプタ一覧
        int ff=0;
        for (n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_socket) {
                accept_socket = accept(listen_socket, (struct sockaddr *)&caddr, &caddr_len);
                if ( accept_socket < 0 ) // accept失敗チェック
                {
                    debug_log_output("accept() error. ret=%d\n", accept_socket);
                    continue;           // 最初に戻る。
                }
                set_nonblocking_mode(accept_socket,1);//NON_BLOCKING_MODE
                //xxエッジトリガー->レベルトリガ
                add_epoll(accept_socket, EPOLLIN );
            } else {
                if( queue_check(events[n].data.fd) == 1 ){
                     ff=1;
                     //queue_do_copy();
                }else{
                    // 2004/08/27 Add end
                    debug_log_output("do server process %d", events[n].data.fd );
                    del_epoll(events[n].data.fd); 
                    ac_in = new ACCESS_INFO;
                    ac_in->accept_socket = (unsigned int)events[n].data.fd;
                    ac_in->caddr = caddr;
                    server_listen_main(ac_in);
                }
            }
        }
        if( ff ){
        queue_do_copy();
        }
    }
#endif
    close( listen_socket );
    return;
}
#ifndef use_thread
/////////////////////////////////////////////////////////////////////////
// トリガー追加
/////////////////////////////////////////////////////////////////////////
int add_epoll( int socket, int rwtrigger )
{
    struct epoll_event  ev;                             // EPOLL EVENTS
    memset(&ev, 0, sizeof(ev));
    ev.events  = rwtrigger ;
    ev.data.fd = socket;
    /* ソケットをepollに追加 */
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, socket, &ev) < 0) {
        debug_log_output("epoll_ctl(EPOLL_CTL_ADD) error=%s\n", strerror(errno));
        exit(1);
    }else{
        debug_log_output("add_epoll %d", socket );
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////
//トリガー削除
/////////////////////////////////////////////////////////////////////////
int del_epoll( int socket )
{
    struct epoll_event  ev;                             // EPOLL EVENTS
    memset(&ev, 0, sizeof(ev));
    /* ソケットをepollに追加 */
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, socket, &ev) < 0) {
        debug_log_output("epoll_ctl(EPOLL_CTL_DEL) socket=%d error=%s\n", socket, strerror(errno));
        exit(1);
    }else{
        debug_log_output("dell_epoll %d", socket );
    }
    return 0;
}
#endif
/////////////////////////////////////////////////////////////////////////
// 複数アクセス対応
/////////////////////////////////////////////////////////////////////////
void* server_listen_main(void* arg)
{
    ACCESS_INFO* ac_in = (ACCESS_INFO*)arg;
    SOCKET accept_socket = (unsigned int)ac_in->accept_socket;
#ifdef use_thread
    pthread_detach(pthread_self());
#endif
    int                 access_check_ok;
    int                 i;
    unsigned char       client_addr_str[32];
    unsigned char       client_address[4];
    unsigned char       masked_client_address[4];
    unsigned char       work1[32];
    unsigned char       work2[32];
    struct sockaddr_in    caddr = ac_in->caddr;         // クライアントソケットアドレス構造体
    debug_log_output("\n\n=============================================================\n");
    debug_log_output("Socket Accept!!(accept_socket=%d)\n", accept_socket);
    child_count ++;
    rand();
    // caddr 情報表示
    debug_log_output("client addr = %s\n", inet_ntoa(caddr.sin_addr) );
    debug_log_output("client port = %d\n", ntohs(caddr.sin_port) );
    // ==============================
    // アクセスチェック
    // ==============================
    access_check_ok = FALSE;
    // クライアントアドレス
    strncpy((char*)client_addr_str, inet_ntoa(caddr.sin_addr), sizeof(client_addr_str));
    // -------------------------------------------------------------------------
    // Access Allowチェック
    //  リストが空か、クライアントアドレスが、リストに一致したらＯＫとする。
    // -------------------------------------------------------------------------
    if ( access_allow_list[0].flag == FALSE ){ // アクセスリストが空。
        // チェックＯＫとする。
        debug_log_output("No Access Allow List. No Check.\n");
        access_check_ok = TRUE;
    }else{
        debug_log_output("Access Check.\n");
        // クライアントアドレス
        strncpy(client_addr_str, inet_ntoa(caddr.sin_addr), sizeof(client_addr_str));
        // client_addr_strをchar[4]に変換
        strncat(client_addr_str, ".", sizeof(client_addr_str) - strlen(client_addr_str));
        for (i=0; i<4; i++ ){
            sentence_split(client_addr_str, '.', work1, work2);
            client_address[i] = (unsigned char)atoi(work1);
            strncpy(client_addr_str, work2, sizeof(client_addr_str));
        }
        // リストの存在する数だけループ
        for ( i=0; i<ACCESS_ALLOW_LIST_MAX; i++ ){
            if ( access_allow_list[i].flag == FALSE ) // リスト終了
            break;
            // masked_client_address 生成
            masked_client_address[0] = client_address[0] & access_allow_list[i].netmask[0];
            masked_client_address[1] = client_address[1] & access_allow_list[i].netmask[1];
            masked_client_address[2] = client_address[2] & access_allow_list[i].netmask[2];
            masked_client_address[3] = client_address[3] & access_allow_list[i].netmask[3];
            // 比較実行
            if ( 	(masked_client_address[0] == access_allow_list[i].address[0]) &&
            (masked_client_address[1] == access_allow_list[i].address[1]) &&
            (masked_client_address[2] == access_allow_list[i].address[2]) &&
            (masked_client_address[3] == access_allow_list[i].address[3]) 	)
            {
                debug_log_output("[%d.%d.%d.%d] == [%d.%d.%d.%d] accord!!",
                masked_client_address[0], masked_client_address[1], masked_client_address[2], masked_client_address[3],
                access_allow_list[i].address[0],access_allow_list[i].address[1],access_allow_list[i].address[2], access_allow_list[i].address[3] );
                access_check_ok = TRUE;
                break;
            }else{
                debug_log_output("[%d.%d.%d.%d] == [%d.%d.%d.%d] discord!!",
                masked_client_address[0], masked_client_address[1], masked_client_address[2], masked_client_address[3],
                access_allow_list[i].address[0],access_allow_list[i].address[1],access_allow_list[i].address[2], access_allow_list[i].address[3] );
            }
        }
    }
    delete ac_in;
    if ( access_check_ok == FALSE ){
        debug_log_output("Access Denied.\n");
        close(accept_socket);     // Socketクローズ
    }else{
        // HTTP鯖として、仕事実行
        server_http_process(accept_socket);
        //close(accept_socket);
        debug_log_output("HTTP process end.\n");
        debug_log_output("=============================================================\n");
    }
    return NULL;
}
