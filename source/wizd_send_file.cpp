// ==========================================================================
//code=UTF-8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_file.c
//              $Revision: 1.8 $
//              $Date: 2004/05/10 23:33:39 $
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
#ifdef linux
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <time.h>
#else
#include <errno.h>
#include <windows.h>
#include <io.h>
#include <process.h>
#include <signal.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>


#include "wizd.h"
#include "wizd_tools.h"
#include "const.h"
#include "wizd_String.h"

int http_file_send(int accept_socket, char *filename, size_t content_length, size_t range_start_pos );
// 2004/08/13 Update end
int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p);


int http_header_response(int accept_socket, HTTP_RECV_INFO* http_recv_info_p,size_t content_length)
{
    int     send_header_data_len;
    int     result_len;
    wString send_http_header_buf;
    send_http_header_buf.headerInit(content_length, 1, (char*)http_recv_info_p->mime_type);
    send_http_header_buf.header( "Accept-Ranges: bytes");
    send_header_data_len = send_http_header_buf.length()+2;
    debug_log_output("send_header_data_len = %d\n", send_header_data_len);
    debug_log_output("--------\n");
    debug_log_output("%s", send_http_header_buf.headerPrintMem().c_str());
    debug_log_output("--------\n");
    
    // --------------
    // ヘッダ返信
    // --------------
    result_len = send(accept_socket, send_http_header_buf.headerPrintMem().c_str(), send_header_data_len, 0);
    debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_header_data_len);
    return 0;
}

// **************************************************************************
// ファイル実体の返信。
// ヘッダ生成＆送信準備
// **************************************************************************
int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    size_t	content_length;
    
    struct	stat file_stat;
    int	result;
    // -------------------------------
    // ファイルサイズチェック
    // -------------------------------
    if ( http_recv_info_p->range_end_pos > 0 ){	// end位置指定有り。
        content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
    // end位置指定無し。
    } else { 
        result = stat((char*)http_recv_info_p->send_filename, &file_stat); // ファイルサイズチェック。
        if ( result != 0 ) {
            debug_log_output("file not found.");
            return ( -1 );
        }
        content_length = file_stat.st_size - http_recv_info_p->range_start_pos;
    }
    
    // --------------
    // OK ヘッダ生成
    // --------------
    http_header_response(accept_socket, http_recv_info_p,content_length);
    
    // --------------
    // 実体返信
    // --------------
    http_file_send(accept_socket, http_recv_info_p->send_filename,content_length,http_recv_info_p->range_start_pos );
    return 0;
}



// **************************************************************************
// ファイルの実体の送信実行部
// **************************************************************************
int http_file_send(int accept_socket, char *filename, size_t content_length, size_t range_start_pos )
{
    int		in_fd;
    size_t		seek_ret;
    
    // ---------------------
    // ファイルオープン
    // ---------------------
    in_fd = open(filename, O_RDONLY);
    if ( in_fd < 0 ) {
        debug_log_output("open() error.");
        return ( -1 );
    }
    
    
    // ------------------------------------------
    // range_start_posへファイルシーク
    // ------------------------------------------
    seek_ret = lseek(in_fd, range_start_pos, SEEK_SET);
    if ( seek_ret < 0 ) {	// lseek エラーチェック
        debug_log_output("lseek() error.");
        close(in_fd);
        return ( -1 );
    }
    
    // ================
    // 実体転送開始
    // ================
    if( copy_descriptors(in_fd,accept_socket,content_length)<0 ) {
        return ( -1 );
    }
    // 正常終了
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// データコピー登録
int copy_descriptors(int in_fd, int out_fd, size_t content_length)
{
    #ifdef linux
    //ノンブロッキングモードに設定
    set_nonblocking_mode(in_fd, 0); /* blocking */
    set_nonblocking_mode(out_fd,0); /* blocking */
    #endif
    copy_body(in_fd,out_fd,content_length);
    return 0;
}
//ノンブロッキングモード対応のコピー
// -1:ERROR  0:END
int copy_body(int in_fd, int out_fd, size_t content_length )
{
    int                    read_length=0;
    int                    write_length=0;
    int                    target_read_size=0;
    unsigned char*         send_buf_p = (unsigned char*)new char[SEND_BUFFER_SIZE];
    int                    current_write_size=0;
    int                    current_read_size=0;
    size_t                 total_read_size=0;
    size_t                 total_write_size=0;
    // ================
    // 実体転送開始
    // ================
    while ( 1 )
    {
        // 目標readサイズ計算 content_length==0も考慮
        if ( (content_length - total_write_size) > SEND_BUFFER_SIZE || content_length == 0) {
            target_read_size = SEND_BUFFER_SIZE;
        } else {
            target_read_size = (size_t)(content_length - total_write_size);
        }
        
        // ファイルからデータを読み込む。必ず読める前提
        read_length = read(in_fd, send_buf_p, target_read_size);
        //read end
        if ( read_length == 0 ){
            debug_log_output("rw end %d %d", in_fd, out_fd );
            close(in_fd);
            sClose(out_fd);
            delete[] send_buf_p;
            send_buf_p=0;
            return 0;
        //read error
        }else if ( read_length < 0 ){
            close(in_fd);
            sClose(out_fd);
            delete[] send_buf_p;
            send_buf_p = 0;
            debug_log_output("read error error=%s\n", strerror(errno));
            return ( -1 );
        //読み込み正常終了
        }else{
            debug_log_output("Normal read %d", read_length );
            total_read_size += read_length;
            current_read_size = read_length;
            current_write_size = 0;
        }
        // SOCKET にデータを送信
        write_length = write(out_fd, send_buf_p, current_read_size);
        //write error
        if ( write_length < 0) {
            debug_log_output("send() error.%d %s\n", errno,strerror(errno));
            delete[] send_buf_p;       // Memory Free.
            send_buf_p=0;
            close(in_fd);   // File Close
            sClose(out_fd);
            return ( -1 );
        }
        //書き込み更新
        total_write_size += write_length;
        current_write_size += write_length;
        if ( content_length != 0 )
        {
            debug_log_output("Streaming..  %ld / %ld ( %ld.%ld%% )\n",
            total_write_size, content_length,
            total_write_size * 100 / content_length,
            (total_write_size * 1000 / content_length ) % 10 );
        }
    }
    return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////
int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p)
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
        set_nonblocking_mode(*in_fd_p, 0); /* nonblocking */
        #endif
        return 0;               // 次のファイルの準備完了
    } else {
        // パラメータがNULLの場合には1ファイルのみの処理とする
        return 1;               // これで終了
    }
}
