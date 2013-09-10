// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_file.c
//											$Revision: 1.7 $
//											$Date: 2003/12/21 03:21:10 $
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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <error.h>
#include <cerrno>
#include "wizd.h"

static int http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );
// 2004/08/13 Update end
static int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p);



int http_header_response(int accept_socket, HTTP_RECV_INFO* http_recv_info_p,off_t content_length)
{
        int     send_header_data_len;
        int     result_len;

        unsigned char   send_http_header_buf[2048];
        int             ptr=0;


        // ---------------
        // 作業用変数初期化
        // ---------------
        memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));

        // --------------
        // OK ヘッダ生成
        // --------------
        ptr = snprintf( (char*)send_http_header_buf, sizeof(send_http_header_buf), "%s%s"HTTP_SERVER_NAME ,HTTP_OK, HTTP_CONNECTION, SERVER_NAME);
        if( content_length ){
            ptr += snprintf( (char*)send_http_header_buf+ptr, sizeof(send_http_header_buf) - ptr, HTTP_CONTENT_LENGTH, content_length );
        }
        ptr += snprintf( (char*)send_http_header_buf+ptr, sizeof(send_http_header_buf) - ptr, HTTP_CONTENT_TYPE HTTP_END, http_recv_info_p->mime_type);

        send_header_data_len = strlen((char*)send_http_header_buf);
        debug_log_output("send_header_data_len = %d\n", send_header_data_len);
        debug_log_output("--------\n");
        debug_log_output("%s", send_http_header_buf);
        debug_log_output("--------\n");


        // --------------
        // ヘッダ返信
        // --------------
        result_len = send(accept_socket, send_http_header_buf, send_header_data_len, 0);
        debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_header_data_len);
        return 0;
}

// **************************************************************************
// ファイル実体の返信。
// ヘッダ生成＆送信準備
// **************************************************************************
int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	off_t	content_length;
	
	struct	stat	file_stat;
	int				result;



	// -------------------------------
	// ファイルサイズチェック
	// -------------------------------
	if ( http_recv_info_p->range_end_pos > 0 )	// end位置指定有り。
	{
		content_length = (http_recv_info_p->range_end_pos - http_recv_info_p->range_start_pos) + 1;
	}
	else // end位置指定無し。
	{
		result = stat((char*)http_recv_info_p->send_filename, &file_stat); // ファイルサイズチェック。
		if ( result != 0 )
		{
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
	http_file_send(	accept_socket, 	http_recv_info_p->send_filename,content_length,http_recv_info_p->range_start_pos );
	return 0;
}



// **************************************************************************
// ファイルの実体の送信実行部
// **************************************************************************
static int http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos )
{
	int		in_fd;
	off_t			seek_ret;
        JOINT_FILE_INFO_T *joint_file_info_p=NULL;

	// ---------------------
	// ファイルオープン
	// ---------------------
	in_fd = open((char*)filename, O_RDONLY);
	if ( in_fd < 0 )
	{	
		debug_log_output("open() error.");
		return ( -1 );
	}


	// ------------------------------------------
	// range_start_posへファイルシーク
	// ------------------------------------------
	seek_ret = lseek(in_fd, range_start_pos, SEEK_SET);
	if ( seek_ret < 0 )	// lseek エラーチェック
	{
		debug_log_output("lseek() error.");
		close(in_fd);
		return ( -1 );
	}


	// ================
	// 実体転送開始
	// ================
	if( copy_descriptors(in_fd,accept_socket,content_length,joint_file_info_p,range_start_pos)<0 )
	{
		return ( -1 );
	}
	// 正常終了
	return 0;
}
// **************************************************************************
// データコピーループ
// **************************************************************************
int copy_descriptors(int in_fd,
                     int out_fd,
                     off_t content_length,
                     JOINT_FILE_INFO_T *joint_file_info_p,
                     off_t range_start_pos)
{

        unsigned char   *send_buf_p;
        off_t                  read_length;
        int                     write_length;
        int                     buffer_ptr;
        off_t                  total_write_size=0;
        off_t                  total_read_size=0;
        off_t                  target_read_size=0;
        // ======================
        // 送信バッファを確保
        // ======================

        send_buf_p = (unsigned char*)malloc(SEND_BUFFER_SIZE);
        if ( send_buf_p == NULL )
        {
                debug_log_output("malloc() error.\n");
                return (-1 );
        }
#ifdef linux
        set_blocking_mode(in_fd, 0); /* non_blocking */
        set_blocking_mode(out_fd,0); /* non_blocking */
#endif
        // 一応バッファクリア
        memset(send_buf_p, 0, SEND_BUFFER_SIZE);
        // ================
        // 実体転送開始
        // ================
        while ( 1 )
        {
                if( total_read_size == total_write_size ){
                    // 目標readサイズ計算 content_length==0も考慮
                    if ( (content_length - total_write_size) > SEND_BUFFER_SIZE || content_length == 0)
                    {
                        target_read_size = SEND_BUFFER_SIZE;
                    }
                    else
                    {
                        target_read_size = (size_t)(content_length - total_write_size);
                    }


                    // ファイルからデータを読み込む。
                    read_length = read(in_fd, send_buf_p, target_read_size);
                    //read end
                    if ( read_length == 0 )
                    {
                       //読み終わった。contents_length変えるべき
                       //if (next_file(&in_fd, joint_file_info_p)){
                       //        debug_log_output("EOF detect.\n");
                       //        break;
                       //}
                       debug_log_output( "read end");
                       close(in_fd);
                       close(out_fd);
                       free(send_buf_p);
                       send_buf_p = 0;
                       return 0;
                    //read error
                    }else if ( read_length < 0 ){
                       close(in_fd);
                       close(out_fd);
                       free(send_buf_p);
                       debug_log_output("read error");
                       return ( -1 );
                    }else{
                       total_read_size += read_length;
                    }
                    buffer_ptr = 0;
                }else if ( total_read_size < total_write_size ){
                    debug_log_output( "read write error");
                    close(in_fd);
                    close(out_fd);
                    free(send_buf_p);
                    return ( -1 );
                }
                    
                // SOCKET にデータを送信
                write_length = write(out_fd, send_buf_p+buffer_ptr, read_length-buffer_ptr);
                if ( write_length < 0)
                {
                        if( errno == EAGAIN ){
                            debug_log_output("EAGAIN");
                            continue;
                        }
                        debug_log_output("send() error.%d %s\n", errno,strerror(errno));
        		free(send_buf_p);       // Memory Free.
                        close(in_fd);   // File Close
                        close(out_fd);
                        return ( -1 );
                }

                total_write_size += write_length;
                buffer_ptr += write_length;
                if ( content_length != 0 )
                {
                    debug_log_output("Streaming..  %lld / %lld ( %lld.%lld%% )\n", 
                      total_write_size, content_length, total_read_size * 100 / content_length,  (total_read_size * 1000 / content_length ) % 10 );
                    if ( total_write_size >= content_length)
                    {
                        debug_log_output("send() end.(content_length=%d)\n", content_length );
                    }
                }
        }
	return 0;
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
