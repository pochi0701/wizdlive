// ==========================================================================
//code=UTF-8	tab=4
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
int er=0;
int ccc=0;
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
	if( copy_descriptors(in_fd,accept_socket,content_length)<0 )
	{
		return ( -1 );
	}
	// 正常終了
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
COPY_QUEUE    queue_copy[MAX_QUEUE];
int           queue_num;
int           queue_root;
/////////////////////////////////////////////////////////////////////////////
void   queue_init(void)
{
     for( int i = 0 ; i < MAX_QUEUE ; i++ )
     {
         queue_copy[i].prev = -1;
         queue_copy[i].next = -1;
         queue_copy[i].done = 1;
     }
     queue_num  = 0;
     queue_root = -1;
}
 
/////////////////////////////////////////////////////////////////////////////
//CUE追加
int  enqueue_memory(int out_fd, off_t content_length, unsigned char* buffer)
{
    COPY_QUEUE* queue;
    int queue_end=-1;
    int ptr;
    //QUEUEが満杯の場合にはとにかく送出する
    while( queue_num>=MAX_QUEUE){
        queue_do_copy();
    }
    if( queue_root == -1 ){
        queue_root = 0;
        queue_end = queue_root;
        queue_copy[queue_end].prev = -1;
        queue_copy[queue_end].next = -1;
    }else{
        //queue_rootから最後を探す
        ptr = queue_root;
        //最終queue検索
        for( int i = 0 ; i < MAX_QUEUE ; i++ ){
            if( queue_copy[ptr].next == -1){
                queue_end = (i+1)%MAX_QUEUE;
                //最終queueに繋げる要素を検索
                for( int j = 0 ; j < MAX_QUEUE ; j++ ){
                    if( queue_copy[queue_end].done == 1 ){
                        queue_copy[queue_end].prev = ptr;
                        queue_copy[queue_end].next = -1;
                        queue_copy[ptr].next       = queue_end;
                        break;
                    }
                    queue_end = (queue_end+1)%MAX_QUEUE;
                }
                //見つかったら終了
                if( queue_copy[queue_end].done == 1 ){
                    break;
                }else{
                    debug_log_output("NO MORE QUEUE CAN ASSIGN");
                    return (-1);
                }
            }else{
                ptr = queue_copy[ptr].next;
            }
        }
    }
    queue = &queue_copy[queue_end];
    // ======================
    // 送信バッファを確保
    // ======================
    queue->send_buf_p = buffer;
    queue->in_fd          = -1;
    queue->out_fd         = out_fd;
    queue->content_length = content_length;
    queue->read_size      = content_length;
    queue->write_size     = 0;
    queue->buffer_ptr     = 0;
    queue->done           = 0;
    queue_num += 1;
    debug_log_output("queue add (%d-%d)/%d", queue_root, queue_end, queue_num);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
//CUE追加
int  enqueue(int in_fd, int out_fd, off_t content_length)
{
    COPY_QUEUE* queue;
    int queue_end=-1;
    int ptr;
    //QUEUEが満杯の場合にはとにかく送出する
    while( queue_num>=MAX_QUEUE){
        queue_do_copy();
    }
    if( queue_root == -1 ){
        queue_root = 0;
        queue_end = queue_root;
        queue_copy[queue_end].prev = -1;
        queue_copy[queue_end].next = -1;
    }else{
        //queue_rootから最後を探す
        ptr = queue_root;
        //最終queue検索
        for( int i = 0 ; i < MAX_QUEUE ; i++ ){
            if( queue_copy[ptr].next == -1){
                queue_end = (i+1)%MAX_QUEUE;
                //最終queueに繋げる要素を検索
                for( int j = 0 ; j < MAX_QUEUE ; j++ ){
                    if( queue_copy[queue_end].done == 1 ){
                        queue_copy[queue_end].prev = ptr;
                        queue_copy[queue_end].next = -1;
                        queue_copy[ptr].next       = queue_end;
                        break;
                    }
                    queue_end = (queue_end+1)%MAX_QUEUE;
                }
                //見つかったら終了
                if( queue_copy[queue_end].done == 1 ){
                    break;
                }else{
                    debug_log_output("NO MORE QUEUE CAN ASSIGN");
                    return (-1);
                }
            }else{
                ptr = queue_copy[ptr].next;
            }
        }
    }
    queue = &queue_copy[queue_end];
    // ======================
    // 送信バッファを確保
    // ======================
    queue->send_buf_p = (unsigned char*)malloc(SEND_BUFFER_SIZE);
    if ( queue->send_buf_p == NULL )
    {
        debug_log_output("malloc() error.\n");
        return (-1);
    }
    queue->in_fd          = in_fd;
    queue->out_fd         = out_fd;
    queue->content_length = content_length;
    queue->read_size      = 0;
    queue->write_size     = 0;
    queue->buffer_ptr     = 0;
    queue->done           = 0;
    queue_num += 1;
    debug_log_output("queue add (%d-%d)/%d", queue_root, queue_end, queue_num);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
//CUE削除
int  dequeue(int num)
{
     if( queue_copy[num].prev == -1 ){
         queue_root = queue_copy[num].next;
         if( queue_root >= 0 ){
             queue_copy[queue_root].prev = -1;
         }
     }else if( queue_copy[num].next == -1 ){
         queue_copy[queue_copy[num].prev].next = -1;
     }else{
         queue_copy[queue_copy[num].prev].next = queue_copy[num].next;
         queue_copy[queue_copy[num].next].prev = queue_copy[num].prev;
     }
     queue_copy[num].in_fd =0;
     queue_copy[num].out_fd =0;
     queue_copy[num].done = 1;
     queue_num -= 1;
     return 0;
}
/////////////////////////////////////////////////////////////////////////////
//queue個数
int  queue_get_num(void)
{
     return queue_num;
}
/////////////////////////////////////////////////////////////////////////////
//CUEコピー
int  queue_do_copy(void)
{
     int ptr = queue_root;
     //実行可能な場合
     if( queue_num > 0 ){
         //要素を順に実行。
         while( ptr >= 0 ){
            //エラーまたは正常終了
            if( copy_body(&queue_copy[ptr]) <= 0 ){
                dequeue(ptr);
            }
            //最後のcueが削除されても大丈夫
            ptr = queue_copy[ptr].next;
         }
         return 0;
     }else{
         return (-1);
     }
}
/////////////////////////////////////////////////////////////////////////////
// データコピー登録
int copy_descriptors(int in_fd,
                     int out_fd,
                     off_t content_length)
{
#ifdef linux
        //ノンブロッキングモードに設定
        set_blocking_mode(in_fd, 1); /* non_blocking */
        set_blocking_mode(out_fd,1); /* non_blocking */
#endif
        enqueue(in_fd,out_fd,content_length);
        //queue_do_copy();
        return 0;
}
/////////////////////////////////////////////////////////////////////////////
//コピー本体
int copy_body(COPY_QUEUE* cc)
{
        int  read_length;
        int  write_length;
        // ================
        // 実体転送開始
        // ================
        if( cc->read_size <= cc->write_size ){
            // 目標readサイズ計算 content_length==0も考慮
            if ( (cc->content_length - cc->write_size) >= SEND_BUFFER_SIZE || cc->content_length == 0 )
            {
                cc->read_size = SEND_BUFFER_SIZE;
            }
            else
            {
                cc->read_size = (cc->content_length - cc->write_size);
            }

            // ファイルからデータを読み込む。
            read_length = read(cc->in_fd, cc->send_buf_p, cc->read_size);
            cc->read_size = read_length;
            //read end
            if ( read_length == 0 )
            {
                //読み終わった。
                shutdown(cc->out_fd,SHUT_RDWR);
                if( cc->in_fd>=0) close(cc->in_fd);
                close(cc->out_fd);
                free(cc->send_buf_p);
                cc->send_buf_p = 0;
                cc->done = 1;
                return 0;
            }
            //EAGAINの場合は再度読み込み
            else if ( read_length < 0 )
            {
                if( errno == EAGAIN ){
                    return 1;
                }
                debug_log_output("read() error.%d %s\n", errno,strerror(errno));
                shutdown(cc->out_fd,SHUT_RDWR);
                if( cc->in_fd>=0) close(cc->in_fd);
                close(cc->out_fd);
                free(cc->send_buf_p);
                cc->send_buf_p = 0;
                cc->done = 1;
                debug_log_output("read error");
                return ( -1 );
             }
             cc->buffer_ptr = 0;
        }
        // SOCKET にデータを送信
        if( cc->read_size > 0 )
        {
            write_length = write(cc->out_fd, cc->send_buf_p+cc->buffer_ptr, cc->read_size-cc->buffer_ptr);
            if ( write_length < 0)
            {
                if( errno == EAGAIN ){
                    return 1;
                }
                debug_log_output("write() error.%d %s\n", errno,strerror(errno));
                free(cc->send_buf_p);       // Memory Free.
                shutdown(cc->out_fd,SHUT_RDWR);
                if( cc->in_fd>=0) close(cc->in_fd);   // File Close.
                close(cc->out_fd);
                cc->send_buf_p = 0;
                cc->done = 1;
                return ( -1 );
            }

            //書き込み量更新
            cc->write_size += write_length;
            cc->buffer_ptr += write_length;
            if ( cc->content_length != 0 )
            {
                debug_log_output("Streaming..  %lld / %lld ( %lld.%lld%% )\n", 
                                  cc->write_size, 
                                  cc->content_length, cc->write_size * 100 / cc->content_length,  
                                  (cc->write_size * 1000 / cc->content_length ) % 10 );
                if ( cc->write_size >= cc->content_length)
                {
                    debug_log_output("send() end.(cc->content_length=%d)\n", cc->content_length );
                    //読み終わった。
                    shutdown(cc->out_fd,SHUT_RDWR);
                    if( cc->in_fd>=0) close(cc->in_fd);
                    close(cc->out_fd);
                    free(cc->send_buf_p);
                    cc->send_buf_p = 0;
                    cc->done = 1;
                    return 0;
                }
            }
        }
	return 1;
}
