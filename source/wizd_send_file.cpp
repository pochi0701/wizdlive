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

// **************************************************************************
// ファイル実体の返信。
// ヘッダ生成＆送信準備
// **************************************************************************
int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
	int	send_header_data_len;
	int	result_len;

	unsigned char	send_http_header_buf[2048];
	unsigned char	work_buf[1024];

	off_t	content_length;
	
	struct	stat	file_stat;
	int				result;



	// ---------------
	// 作業用変数初期化
	// ---------------
	memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));



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
	strncpy((char*)send_http_header_buf, HTTP_OK, sizeof(send_http_header_buf));

	strncat((char*)send_http_header_buf, HTTP_CONNECTION, sizeof(send_http_header_buf) - strlen((char*)send_http_header_buf));

	snprintf((char*)work_buf, sizeof(work_buf), HTTP_SERVER_NAME, SERVER_NAME);
	strncat((char*)send_http_header_buf, (char*)work_buf, sizeof(send_http_header_buf) - strlen((char*)send_http_header_buf));

	snprintf((char*)work_buf, sizeof(work_buf), HTTP_CONTENT_LENGTH, content_length);
	strncat((char*)send_http_header_buf, (char*)work_buf, sizeof(send_http_header_buf) - strlen((char*)send_http_header_buf) );

	snprintf((char*)work_buf, sizeof(work_buf), HTTP_CONTENT_TYPE, http_recv_info_p->mime_type);
	strncat((char*)send_http_header_buf, (char*)work_buf, sizeof(send_http_header_buf) - strlen((char*)send_http_header_buf) );
	strncat((char*)send_http_header_buf, HTTP_END, sizeof(send_http_header_buf) - strlen((char*)send_http_header_buf) );


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
        off_t                  file_read_len;
        int                     data_send_len;
        int                     buffer_ptr;
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
        set_blocking_mode(in_fd, 1); /* non_blocking */
        set_blocking_mode(out_fd,1); /* non_blocking */
#endif
        // 一応バッファクリア
        memset(send_buf_p, 0, SEND_BUFFER_SIZE);
        // ================
        // 実体転送開始
        // ================
        while ( 1 )
        {
                if( target_read_size == total_read_size ){
                    // 目標readサイズ計算 content_length==0も考慮
                    if ( (content_length - total_read_size) > SEND_BUFFER_SIZE )
                    {
                        target_read_size = SEND_BUFFER_SIZE;
                    }
                    else
                    {
                        target_read_size = (size_t)(content_length - total_read_size);
                    }


                    // ファイルからデータを読み込む。
                    file_read_len = read(in_fd, send_buf_p, target_read_size);
                    //read end
                    if ( file_read_len == 0 )
                    {
                       //読み終わった。contents_length変えるべき
                       //if (next_file(&in_fd, joint_file_info_p)){
                       //        debug_log_output("EOF detect.\n");
                       //        break;
                       //}
                       close(in_fd);
                       free(send_buf_p);
                       send_buf_p = 0;
                       return 0;
                    //read error
                    }else if ( file_read_len < 0 ){
                       close(in_fd);
                       free(send_buf_p);
                       debug_log_output("read error");
                       return ( -1 );
                    }
                    buffer_ptr = 0;
                }else if ( target_read_size < total_read_size ){
                    debug_log_output( "read write error");
                    close(in_fd);
                    free(send_buf_p);
                    return ( -1 );
                }
                    
                // SOCKET にデータを送信
                data_send_len = write(out_fd, send_buf_p+buffer_ptr, file_read_len-buffer_ptr);
                if ( data_send_len < 0)
                {
                        if( errno == EAGAIN ){
                            continue;
                        }
                        debug_log_output("send() error.%d %s\n", errno,strerror(errno));
        		free(send_buf_p);       // Memory Free.
                        close(in_fd);   // File Close.
                        return ( -1 );
                }

                total_read_size += data_send_len;
                buffer_ptr += data_send_len;
                if ( content_length != 0 )
                {
                        debug_log_output("Streaming..  %lld / %lld ( %lld.%lld%% )\n", total_read_size, content_length, total_read_size * 100 / content_length,  (total_read_size * 1000 / content_length ) % 10 );
                }
                if ( total_read_size >= content_length)
                {
                        debug_log_output("send() end.(content_length=%d)\n", content_length );
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
