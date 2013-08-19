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


#include "wizd.h"

static int http_file_send(int accept_socket, unsigned char *filename, off_t content_length, off_t range_start_pos );


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
	if( copy_descriptors(in_fd,accept_socket,content_length,joint_file_info_p,(char*)filename,range_start_pos)<0 )
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
                     char* infilename,
                     off_t range_start_pos)
{

        unsigned char   *send_buf_p;
        ssize_t                 file_read_len;
        int                     data_send_len;
        off_t                   total_read_size;
        size_t                  target_read_size;

        // ======================
        // 送信バッファを確保
        // ======================

        send_buf_p = (unsigned char*)malloc(SEND_BUFFER_SIZE);
        if ( send_buf_p == NULL )
        {
                debug_log_output("malloc() error.\n");
                return (-1 );
        }
        // ================
        // 実体転送開始
        // ================
        while ( 1 )
        {
                // 一応バッファクリア
                memset(send_buf_p, 0, SEND_BUFFER_SIZE);

                // 目標readサイズ計算
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
                if ( file_read_len <= 0 )
                {
                        debug_log_output("EOF detect.\n");
                        break;
                }

                // SOCKET にデータを送信
                data_send_len = send(out_fd, send_buf_p, file_read_len, 0);
                if ( data_send_len != file_read_len )
                {
                        debug_log_output("send() error.\n");
        		free(send_buf_p);       // Memory Free.
                        close(in_fd);   // File Close.
                        return ( -1 );
                }

                total_read_size += file_read_len;

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

