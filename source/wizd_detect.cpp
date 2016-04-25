// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_detect.c
//											$Revision: 1.4 $
//											$Date: 2004/06/27 06:50:00 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <signal.h>
#ifdef linux
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#include <io.h>
#endif
#include "wizd.h"
#include "wizd_tools.h"
#define  SSDP_IP_ADDRESS  "239.255.255.250"
#define  SSDP_PORT   (1900)
#define  SSDP_CHECK_KEY  "Server"
#define  SSDP_CHECK_VALUE "Syabas myiBox"
SOCKET          ssdp_socket;    // SSDP 待ち受けSocket
// =============================================================
// MediaWiz検出部
// =============================================================
void*   server_detect(void* ptr)
{
    IGNORE_PARAMETER(ptr);
    int    ret;
    struct      sockaddr_in             ssdp_server_addr;               // サーバ側ソケットアドレス構造体
    struct      sockaddr_in             ssdp_client_addr;               // 受信ソケットアドレス構造体
    struct      ip_mreq                 ssdp_mreq;                              // マルチキャスト設定
    int         ssdp_client_addr_len = sizeof(ssdp_client_addr);
    int         sock_opt_val;
    int         recv_len;
    int                                         regist_socket;
    struct      sockaddr_in             connect_addr, self_addr;
    int         self_addr_len;
    int         flag_media_wiz;
    char        recv_buf[1024*2];
    char        send_buf[1024*2];
    char        work_buf[1024];
    char        line_buf[1024];
    char        ssdp_check_key_buf[1024];
    char        *work_p;
    char        self_ip_address[32];
    // 2004/10/19 Add start
    // ネットワーク2枚ざしの時のIP変化に対応
    int count;
    #ifdef linux
    struct hostent *lpHost=NULL;
    struct in_addr inaddr;
    #else
    HOSTENT *lpHost;
    IN_ADDR inaddr;
    #endif
    char  server_ip[32];
    int     timeout = 600;// 0.6 seconds
    //2004.10.31 初期値は未定義ということで-1 R.K
    int     host_idx = -1;
    int     detect_flag = 0;
    // 2004/10/19 Add end
    // 2004/10/19 Add start
    // ネットワーク2枚ざしの時のIP変化に対応
    while( loop_flag ){
        lpHost = gethostbyname( global_param.server_name );
        for( count = 0 ; lpHost && lpHost->h_addr_list[count] != NULL ; count++ ){
            //終了の場合抜ける
            if( ! loop_flag ){
                break;
            }
            // 1度でもdetect成功していて、index値が違う場合はSSDPしない
            if( ( detect_flag == 1 )&&( count != host_idx ) ){
                continue;
            }
            // 2004/10/22 Add end
            // =============================
            // ソケット生成
            // =============================
            ssdp_socket = socket(AF_INET, SOCK_DGRAM, 0 );
            //WINDOWSではINVALID_SOCKET=NOT(0)をかえすらしい
            #ifdef linux
            if ( SERROR(ssdp_socket) ){ // ソケット生成失敗チェック
                debug_log_output("AutoDetect: socket() error.");
                exit( 1 );
            }
            #else
            if ( SERROR( ssdp_socket ) ){ // ソケット生成失敗チェック
                // スレッド終了
                ExitThread(TRUE);
                return NULL;
                // 2004/08/19 Update end
            }
            #endif
            // 1度もdetect成功してない場合は、受信のタイムアウト時間を設定
            if( detect_flag == 0 ){
                setsockopt( ssdp_socket , SOL_SOCKET , SO_RCVTIMEO , (char*)&timeout , sizeof(timeout) );
            }
            // bind の前に setsockopt しないとbind が-1 を返して、駄目でした。
            // REUSEADDR 設定
            sock_opt_val = 1;
            ret = setsockopt(ssdp_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val, sizeof(sock_opt_val));
            if( SERROR(ret) ){
                sClose( ssdp_socket );
                ExitThread(TRUE);
                return NULL;
            }    
            // ===========================================
            // ソケットアドレス構造体に値をセット
            // ===========================================
            memset( (char *)&ssdp_server_addr, 0, sizeof(ssdp_server_addr) );
            ssdp_server_addr.sin_family = AF_INET;
            ssdp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            ssdp_server_addr.sin_port = htons(SSDP_PORT);
            // =============================
            // bind 実行
            // =============================
            ret = bind(ssdp_socket, (struct sockaddr *)&ssdp_server_addr, sizeof(ssdp_server_addr));
            if ( SERROR(ret) ){
                debug_log_output("AutoDetect: bind() error.");
                sClose( ssdp_socket );
                // スレッド終了
                ExitThread(TRUE);
                return NULL;
            }
            // 2004/12/10 Add start
            else if ( ret != 0){
                ExitThread(TRUE);
                return NULL;
            }
            // ===============================
            // ソケットにオプションをセット
            // ===============================
            
            // REUSEADDR 設定
            //sock_opt_val = 1;
            //ret = setsockopt(ssdp_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val, sizeof(sock_opt_val));
            //if( SERROR(ret) ){
            //    sClose( ssdp_socket );
            //    ExitThread(TRUE);
            //    return NULL;
            //}  
            
            // SSDPマルチキャスト メンバシップ設定
            ssdp_mreq.imr_multiaddr.s_addr = inet_addr(SSDP_IP_ADDRESS);
            
            
            #if 0 //def linux
            if ( strlen(global_param.auto_detect_bind_ip_address) == 0 ){
                debug_log_output("SSDP Listen:INADDR_ANY");
                ssdp_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            }else{
                debug_log_output("SSDP Listen:%s", global_param.auto_detect_bind_ip_address);
                ssdp_mreq.imr_interface.s_addr = inet_addr(global_param.auto_detect_bind_ip_address);
            }
            #else
            memcpy( &inaddr , lpHost->h_addr_list[count] , 4 );
            strcpy( server_ip , inet_ntoa(inaddr) );
            ssdp_mreq.imr_interface.s_addr = inet_addr( server_ip );
            #endif
            // ===============================
            // ソケットにオプションをセット
            // ===============================
            ret = setsockopt(ssdp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ssdp_mreq, sizeof(ssdp_mreq)); // メンバシップ
            if( SERROR(ret) ){
                sClose( ssdp_socket );
                ExitThread(TRUE);
                return NULL;
            }    

            // =====================
            // メインループ
            // =====================
            // SSDPパケット受信
            memset(recv_buf, '\0', sizeof(recv_buf));
            recv_len = recvfrom(ssdp_socket, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&ssdp_client_addr, (socklen_t*)&ssdp_client_addr_len );
            //読み取ろうとおもったら、読み取れなかった
            if( recv_len == SOCKET_ERROR ){
                //sClose( ssdp_socket );
                continue;
            }
            debug_log_output("=============== SSDP Receive. ================");
            // ssdp_client_addr 情報表示
            debug_log_output("SSDP client addr = %s\n", inet_ntoa(ssdp_client_addr.sin_addr) );
            debug_log_output("SSDP client port = %d\n", ntohs(ssdp_client_addr.sin_port) );
            // 受け取った内容を表示
            debug_log_output("SSDP recv_len=%d, recv_buf=(ry ", recv_len);
            //debug_log_output("SSDP recv = '%s'", recv_buf );
            // ======================
            // MediaWizかチェック。
            // ======================
            flag_media_wiz = 0;
            ////debug_log_output("");
            ////debug_log_output("--- SSDP ReceiveData Info Start. -----------------------------");
            work_p = recv_buf;
            while ( work_p != NULL ){
                // 受信バッファから、１行切り出す。
                work_p = buffer_distill_line(work_p, line_buf, sizeof(line_buf) );
                debug_log_output("recv:'%s'", line_buf );
                //  ':'の前(Key)を切り出し
                strncpy(ssdp_check_key_buf, line_buf, sizeof(ssdp_check_key_buf) );
                cut_after_character( ssdp_check_key_buf, ':');
                // Key が "Server" かチェック
                if ( strcasecmp(ssdp_check_key_buf, SSDP_CHECK_KEY ) == 0 ){
                    // valueに SSDP_CHECK_VALUE が含まれるかチェック。
                    cut_before_character(line_buf, ':');
                    if ( strstr(line_buf, SSDP_CHECK_VALUE) != NULL ){ //!=
                        // 含まれてた。MediaWizである。
                        debug_log_output( "HIT!!  This is MediaWiz!!", SSDP_CHECK_VALUE );
                        flag_media_wiz = 1;
                        break;
                    }
                }
            }
            debug_log_output("--- SSDP ReceiveData Info End. -----------------------------");
            debug_log_output("");
            // MediaWizじゃなかった。なにもしない。
            if ( flag_media_wiz == 0 ){
                //sClose( ssdp_socket );
                debug_log_output("Not MeidaWiz.");
                debug_log_output("=============== SSDP process end. ================ \n");
                continue;
            }
            // ======================
            // 自動登録実行
            // ======================
            // ----------------------------
            // 登録用ソケット生成
            // ----------------------------
            regist_socket = socket(AF_INET, SOCK_STREAM, 0 );
            if ( SERROR( regist_socket ) ){ // ソケット生成失敗チェック
                //sClose( ssdp_socket );
                sClose( regist_socket );
                debug_log_output("AutoDetect: socket() error.");
                continue;
            }
            // ----------------------
            // MediaWizにCONNECT実行
            // ----------------------
            memset(&connect_addr, 0, sizeof(connect_addr) );
            connect_addr.sin_family = AF_INET;
            connect_addr.sin_addr = ssdp_client_addr.sin_addr;
            connect_addr.sin_port = ssdp_client_addr.sin_port;
            ret = connect(regist_socket, (struct sockaddr *)&connect_addr, sizeof(connect_addr));
            if( SERROR(recv_len) ){
                //sClose( ssdp_socket );
                sClose( regist_socket );
                debug_log_output("AutoDetect: connect() error.");
                continue;
            }
            // ------------------------------------
            // MediaWizと接続した自分自身のIPをGet.
            // ------------------------------------
            memset(&self_addr, 0, sizeof(self_addr) );
            self_addr_len = sizeof(self_addr);
            ret = getsockname(regist_socket, (struct sockaddr *)&self_addr, (socklen_t*)&self_addr_len );
            if( SERROR(recv_len) ){
                // 2004/12/10 Update end
                //sClose( ssdp_socket );
                sClose( regist_socket );
                ////debug_log_output("getsockname() error. ret = %d", ret );
                continue;
            }
            strncpy( self_ip_address, inet_ntoa(self_addr.sin_addr), sizeof(self_ip_address) );
            debug_log_output("SSDP self addr = %s\n", self_ip_address );
            // --------------------------
            // MediaWizへの送信内容生成。
            // --------------------------
            ////debug_log_output("to MediaWiz Registration Start.");
            memset(send_buf, '\0', sizeof(send_buf));
            snprintf(send_buf, sizeof(send_buf),
            "GET /myiBoxUPnP/description.xml?POSTSyabasiBoxURLpeername=%s&peeraddr=%s:%d& HTTP/1.1\r\n",
            global_param.server_name,
            self_ip_address,
            global_param.server_port                    );
            snprintf(work_buf, sizeof(work_buf), "User-Agent: %s\r\n", SERVER_NAME);
            strncat(send_buf, work_buf, sizeof(send_buf) - strlen(send_buf));
            snprintf(work_buf, sizeof(work_buf), "Host: %s:%d\r\n", inet_ntoa(ssdp_client_addr.sin_addr), ntohs(ssdp_client_addr.sin_port) );
            strncat(send_buf, work_buf, sizeof(send_buf) - strlen(send_buf));
            strncat(send_buf, "\r\n", sizeof(send_buf) - strlen(send_buf));
            ////debug_log_output("%s", send_buf);
            // --------------------------
            // 送信実行
            // --------------------------
            ret = send(regist_socket, send_buf, strlen(send_buf), 0);
            ////debug_log_output("regist_socket send result_len=%d" , ret);
            // --------------------------
            // 返信を受信する。
            // --------------------------
            while( loop_flag ){
                recv_len = recv(regist_socket, recv_buf, sizeof(recv_buf), 0);
                if( ( recv_len > 0 ) && ( host_idx != count ) ){
                    // IP    添番保持
                    host_idx = count;
                    // SSDP  okフラグ
                    detect_flag = 1;
                }
                ////debug_log_output("regist_socket recv result_len=%d" , recv_len);
                if ( recv_len <= 0 ){
                    break;
                }
            }
            // --------------------------
            // 登録用Socketをclose.
            // --------------------------
            sClose( regist_socket );
            debug_log_output("=============== SSDP process end. ================ \n");
        }
        ret = setsockopt(ssdp_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&ssdp_mreq, sizeof(ssdp_mreq)); // メンバシップ
        if( SERROR(ret) ){
            sClose( ssdp_socket );
            ExitThread(TRUE);
            return NULL;
        }    

        // ネットワーク2枚ざしの時のIP変化に対応
        sClose( ssdp_socket );
        // ネットワーク2枚ざしの時のIP変化に対応
        Sleep(1000);
    }
    // スレッド終了
    ExitThread(TRUE);
    return NULL;
}
// =============================================================
// 検出部終了処理
// =============================================================
void    detect_finalize(void)
{
    sClose( ssdp_socket );
}