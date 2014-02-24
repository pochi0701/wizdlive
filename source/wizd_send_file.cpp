// ==========================================================================
//code=UTF-8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_send_file.c
//						$Revision: 1.0 $
//						$Date: 2013/10/15 13:41:10 $
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
#include <sys/epoll.h>
#include <time.h>
#include "wizd.h"
#include "wizd_String.h"

int http_file_send(int accept_socket, unsigned char *filename, size_t content_length, size_t range_start_pos );
// 2004/08/13 Update end
int next_file(int *in_fd_p, JOINT_FILE_INFO_T *joint_file_info_p);


int http_header_response(int accept_socket, HTTP_RECV_INFO* http_recv_info_p,size_t content_length)
{
        int     send_header_data_len;
        int     result_len;
#if 1 
        //wString::wStringInit();
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
        //wString::wStringEnd();
#else
        char    work[128];
        unsigned char   send_http_header_buf[2048]={0};
        int             ptr=0;


        // ---------------
        // 作業用変数初期化
        // ---------------
        //memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));

        // --------------
        // OK ヘッダ生成
        // --------------
        ptr = snprintf( (char*)send_http_header_buf, sizeof(send_http_header_buf), 
                         "%s%s%s"HTTP_SERVER_NAME,HTTP_OK, HTTP_CONNECTION, HTTP_ACCEPTRANGE, SERVER_NAME);
        //Date
        time_t timer;
        time(&timer);
        struct tm *utc;
        utc = gmtime(&timer);
        char you[7][4]={"Sun", "Mon","Tue", "Wed", "Thu", "Fri", "Sat"};
        char mon[12][4]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        sprintf( work, "%s, %d %s %d %02d:%02d:%02d",you[utc->tm_wday],utc->tm_mday,mon[utc->tm_mon],utc->tm_year+1900, utc->tm_hour, utc->tm_min, utc->tm_sec); 
        ptr += snprintf( (char*)send_http_header_buf+ptr, sizeof(send_http_header_buf) - ptr, "Date: %s GMT\r\n", work);

        //Last-Modified
        //struct  stat file_stat;
        //int result;
        //result = stat((char*)http_recv_info_p->send_filename, &file_stat);
        //utc = gmtime(&file_stat.st_mtime);
        timer += 60*60;
        utc = gmtime(&timer);
        sprintf( work, "%s, %d %s %d %02d:%02d:%02d",you[utc->tm_wday],utc->tm_mday,mon[utc->tm_mon],utc->tm_year+1900, utc->tm_hour, utc->tm_min, utc->tm_sec);
        ptr += snprintf( (char*)send_http_header_buf+ptr, sizeof(send_http_header_buf) - ptr, "Expires: %s GMT\r\n", work);


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
#endif
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
int http_file_send(int accept_socket, unsigned char *filename, size_t content_length, size_t range_start_pos )
{
	int		in_fd;
	size_t		seek_ret;
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
#ifndef use_thread
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
int  enqueue_memory(int out_fd, size_t content_length, unsigned char* buffer)
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
    queue->in_enabled     = false;
    queue->out_fd         = out_fd;
    queue->out_enabled    = false;
    queue->content_length = content_length;
    queue->total_read_size      = content_length;
    queue->total_write_size     = 0;
    queue->current_write_size     = 0;
    queue->done           = 0;
    queue_num += 1;
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
//CUE追加
int  enqueue(int in_fd, int out_fd, size_t content_length,int flag)
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
    //ここにheaderをコピしてから送出する
    queue->in_fd          = in_fd;
    queue->out_fd         = out_fd;
    queue->out_enabled    = false;
    queue->content_length = content_length;
    queue->total_read_size      = 0;
    queue->total_write_size     = 0;
    queue->current_read_size    = 0;
    queue->current_write_size     = 0;
    queue->done           = 0;
    queue_num += 1;
    //入力がSOCKETの場合
    if( flag ){
        add_epoll(in_fd, EPOLLIN );
        queue->in_enabled = false;
        queue->in_enqueue = true;
    //入力がFILEの場合
    }else{
        queue->in_enabled = true;
        queue->in_enqueue = false; 
    }
    add_epoll(out_fd, EPOLLOUT );
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
     //del_epoll( queue_copy[num].in_fd);
     //del_epoll( queue_copy[num].out_fd);
     queue_copy[num].in_fd =0;
     queue_copy[num].in_enabled = false;
     queue_copy[num].out_fd =0;
     queue_copy[num].out_enabled = false;
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
int  queue_check(int socket)
{
     int ptr = queue_root;
     int flag = 0;
     //実行可能な場合
     if( queue_num > 0 ){
         //要素を順に実行。
         while( ptr >= 0 ){
            //エラーまたは正常終了
            if( queue_copy[ptr].in_fd == socket ){
                queue_copy[ptr].in_enabled = true;
                del_epoll(socket);
                flag = 1;
                break;
            }
            if( queue_copy[ptr].out_fd == socket ){
                queue_copy[ptr].out_enabled = true;
                del_epoll(socket);
                flag = 1;
                break;
            }
            //最後のcueが削除されても大丈夫
            ptr = queue_copy[ptr].next;
         }
         return flag;
     }else{
         return (-1);
     }
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
            if( queue_copy[ptr].in_enabled && queue_copy[ptr].out_enabled ){
                if( copy_body(&queue_copy[ptr]) <= 0 ){
                    dequeue(ptr);
                }
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
                     size_t content_length,
                     JOINT_FILE_INFO_T *joint_file_info_p,
                     size_t range_start_pos,
                     int flag)
{
#ifdef linux
        //ノンブロッキングモードに設定
        set_nonblocking_mode(in_fd, 1); /* nonblocking */
        set_nonblocking_mode(out_fd,1); /* nonblocking */
#endif
        enqueue(in_fd,out_fd,content_length,flag);
        //queue_do_copy();
        return 0;
}
// -1:ERROR  0:END 1:EAGAIN
int copy_body(COPY_QUEUE *qc)
//int in_fd, int out_fd, int total_read_size,int total_write_size,int current_write_size, size_t content_length, unsigned char* send_buf_p )
{
        int                    read_length=0;
        int                    write_length=0;
        int                    target_read_size=0;
        // ================
        // 実体転送開始
        // ================
        while ( qc->in_enabled || qc->out_enabled )
        {
            //debug_log_output("read %d write %d", qc->current_read_size, qc->current_write_size );
            if( qc->in_enabled ){
                if( qc->current_read_size == qc->current_write_size ){
                    // 目標readサイズ計算 content_length==0も考慮
                    if ( (qc->content_length - qc->total_write_size) > SEND_BUFFER_SIZE || qc->content_length == 0)
                    {
                        target_read_size = SEND_BUFFER_SIZE;
                    }
                    else
                    {
                        target_read_size = (size_t)(qc->content_length - qc->total_write_size);
                    }


                    // ファイルからデータを読み込む。
                    read_length = read(qc->in_fd, qc->send_buf_p, target_read_size);
                    //read end
                    if ( read_length == 0 )
                    {
                        //読み終わった。contents_length変えるべき
                        //if (next_file(&in_fd, joint_file_info_p)){
                        //        debug_log_output("EOF detect.\n");
                        //        break;
                        //}
                        debug_log_output("rw end %d %d %d", qc->in_fd, qc->in_enqueue, qc->out_fd );
                        if(qc->in_enqueue){
                        }
                        close(qc->in_fd);
                        if( shutdown(qc->out_fd,SHUT_WR) < 0 ){
                            debug_log_output("Shutdown Error");
                        }
                        close(qc->out_fd);
                        free(qc->send_buf_p);
                        qc->send_buf_p = 0;
                        return 0;
                    //read error
                    }else if ( read_length < 0 ){
                        if( errno == EAGAIN ){
                            qc->in_enabled = false;
                            add_epoll(qc->in_fd, EPOLLIN);
                            debug_log_output("READ EAGAIN");
                            return 1;
                        }
                        if(qc->in_enqueue){
                        }
                        close(qc->in_fd);
                        if( shutdown(qc->out_fd,SHUT_WR) < 0 ){
                            debug_log_output("Shutdown Error");
                        }
                        close(qc->out_fd);
                        free(qc->send_buf_p);
                        qc->send_buf_p = 0;
                        debug_log_output("read error error=%s\n", strerror(errno));
                        return ( -1 );
                    //読み込み正常終了
                    }else{
                        debug_log_output("Normal read %d", read_length );
                        qc->total_read_size += read_length;
                        qc->current_read_size = read_length;
                        qc->current_write_size = 0;
                    }
                }else if ( qc->total_read_size < qc->total_write_size ){
                    debug_log_output( "read write error");
                    if(qc->in_enqueue){
                    }
                    close(qc->in_fd);
                    if( shutdown(qc->out_fd,SHUT_WR) < 0 ){
                        debug_log_output("Shutdown Error");
                    }
                    close(qc->out_fd);
                    free(qc->send_buf_p);
                    qc->send_buf_p = 0;
                    return ( -1 );
                }
            }
            if( qc->out_enabled ){       
                // SOCKET にデータを送信
                write_length = write(qc->out_fd, qc->send_buf_p+qc->current_write_size, qc->current_read_size-qc->current_write_size);
                if ( write_length < 0)
                {
                    if( errno == EAGAIN ){
                        qc->out_enabled = false;
                        add_epoll(qc->out_fd, EPOLLOUT);
                        debug_log_output("WRITE EAGAIN");
                        return 1;
                    }
                    debug_log_output("send() error.%d %s\n", errno,strerror(errno));
        	    free(qc->send_buf_p);       // Memory Free.
                    qc->send_buf_p=0;
                    if(qc->in_enqueue){
                    }
                    close(qc->in_fd);   // File Close
                    if( shutdown(qc->out_fd,SHUT_WR) < 0 ){
                        debug_log_output("Shutdown Error");
                    }
                    close(qc->out_fd);
                    return ( -1 );
                }
                //書き込み更新
                qc->total_write_size += write_length;
                qc->current_write_size += write_length;
                if ( qc->content_length != 0 )
                {
                    debug_log_output("Streaming..  %ld / %ld ( %ld.%ld%% )\n", 
                                                   qc->total_write_size, qc->content_length,
                                                   qc->total_write_size * 100 / qc->content_length,
                                                   (qc->total_write_size * 1000 / qc->content_length ) % 10 );
                    //if ( qc->total_write_size >= qc->content_length)
                    //{
                    //    debug_log_output("send() end.(content_length=%d)\n", qc->content_length );
                    //}
                }
            }
        }
	return 1;
}
#else
/////////////////////////////////////////////////////////////////////////////
// データコピー登録
int copy_descriptors(int in_fd,
                     int out_fd,
                     size_t content_length,
                     JOINT_FILE_INFO_T *joint_file_info_p,
                     size_t range_start_pos,
                     int flag)
{
#ifdef linux
        //ノンブロッキングモードに設定
        set_nonblocking_mode(in_fd, 0); /* blocking */
        set_nonblocking_mode(out_fd,0); /* blocking */
#endif
        copy_body(in_fd,out_fd,content_length);
        return 0;
}
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
                debug_log_output("rw end %d %d", in_fd, out_fd );
                close(in_fd);
                if( shutdown(out_fd,SHUT_WR) < 0 ){
                    debug_log_output("Shutdown Error");
                }
                close(out_fd);
                delete[] send_buf_p;
                send_buf_p=0;
                return 0;
                //read error
            }else if ( read_length < 0 ){
                close(in_fd);
                if( shutdown(out_fd,SHUT_WR) < 0 ){
                    debug_log_output("Shutdown Error");
                }
                close(out_fd);
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
            if ( write_length < 0)
            {
                debug_log_output("send() error.%d %s\n", errno,strerror(errno));
                delete[] send_buf_p;       // Memory Free.
                send_buf_p=0;
                close(in_fd);   // File Close
                if( shutdown(out_fd,SHUT_WR) < 0 ){
                    debug_log_output("Shutdown Error");
                }
                close(out_fd);
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
#endif
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
