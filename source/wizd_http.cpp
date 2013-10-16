// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_http.c
//											$Revision: 1.19 $
//											$Date: 2004/07/19 04:37:32 $
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
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include "wizd.h"
typedef enum {_OPENDIR = -2,_NOTFOUND = -1,_FILE = 0, _DIR = 1, _PLW = 3, _TSV = 4, _VOB = 5, _CGI=6} FILETYPES;
static int http_header_receive(int accept_socket, HTTP_RECV_INFO *http_recv_info);
static FILETYPES http_file_check( HTTP_RECV_INFO *http_recv_info_p);
static FILETYPES http_index( HTTP_RECV_INFO * http_recv_infp_p );
static int http_redirect_response(int accept_socket, HTTP_RECV_INFO *http_recv_info, char *location);
static int http_not_found_response(int accept_socket, HTTP_RECV_INFO *http_recv_info);
int line_receive(int accept_socket, unsigned char *line_buf_p, int line_max);
// **************************************************************************
// * サーバ HTTP処理部
// **************************************************************************
void 	server_http_process(int accept_socket)
{
    FILETYPES		result;
    HTTP_RECV_INFO	http_recv_info;	//HTTP受信情報保存構造体
    int			i;
    int			flag_allow_user_agent_check;
    int                 ret;
    memset(&http_recv_info, 0, sizeof(http_recv_info));
    // ----------------------------------------
    // HTTP リクエストヘッダ受信
    // ----------------------------------------
    debug_log_output("HTTP Header receive start!\n");
    ret = http_header_receive(accept_socket,  &http_recv_info);
    if ( ret != 0 ){ // エラーチェック
        // エラーメッセージ
        debug_log_output("http_header_receive() Error. result=%d\n", ret);
        // ソケットクローズ
        close(accept_socket);
        return;
    }
    debug_log_output("HTTP Header receive end!\n");
    debug_log_output("recv_uri:'%s'\n", http_recv_info.recv_uri);
    debug_log_output("user_agent:'%s'\n", http_recv_info.user_agent);
    debug_log_output("range_start_pos=%d\n", http_recv_info.range_start_pos);
    debug_log_output("range_end_pos=%d\n", http_recv_info.range_end_pos);
    // ==========================
    // = User-Agent チェック
    // ==========================
    flag_allow_user_agent_check = 0;
    if ( strlen(allow_user_agent[0].user_agent)  == 0 ){ // User-Agnet指示が無し。
        debug_log_output("user_agent: allow_user_agent No List. All Allow.");
        flag_allow_user_agent_check = 1; // OK
    }else{
        // User-Agent チェック実行
        for ( i=0; i<ALLOW_USER_AGENT_LIST_MAX; i++){
            if ( strlen(allow_user_agent[i].user_agent)  == 0 ){
                break;
            }
            // 一致チェック
            debug_log_output("user_agent: Check[%d] '%s' in '%s'",i, allow_user_agent[i].user_agent, http_recv_info.user_agent);
            if ( strstr( http_recv_info.user_agent, allow_user_agent[i].user_agent ) != NULL ){
                debug_log_output("user_agent: '%s' OK.", allow_user_agent[i].user_agent );
                flag_allow_user_agent_check = 1; // 一致。OK
                break;
            }
        }
    }
    // User-Agentチェック NGならば、ソケットクローズ。終了。
    if ( flag_allow_user_agent_check == 0 ){
        debug_log_output("allow_user_agent check. Deny. Socket close.");
        // ソケットクローズ
        close(accept_socket);
        return;
    }
    // クライアントがPCかどうか判断
    http_recv_info.flag_pc = (global_param.user_agent_pc[0]
                              && !strncmp(http_recv_info.user_agent, global_param.user_agent_pc
                                          , strlen(global_param.user_agent_pc))
                                         ) ? 1 : 0;
    debug_log_output("flag_pc: %d", http_recv_info.flag_pc);
    //PROXY判定
    if (!strncmp(http_recv_info.recv_uri, "/-.-", 4)){
        // proxy
        if (http_proxy_response(accept_socket, &http_recv_info) < 0){
            debug_log_output("error close not found");
            http_not_found_response(accept_socket, &http_recv_info);
        }
        // ソケットクローズ
        //close(accept_socket);
        return;
    }
    if (path_sanitize(http_recv_info.recv_uri, sizeof(http_recv_info.recv_uri)) == NULL){
        // BAD REQUEST!
        debug_log_output("BAD REQUEST!");
        http_not_found_response(accept_socket, &http_recv_info);
        close(accept_socket);
        return;
    }
    debug_log_output("sanitized recv_uri: %s", http_recv_info.recv_uri);
    // ----------------------------------------
    // 受け取ったURIの拡張子がrename対象ならばrename
    // ----------------------------------------
    extension_del_rename(http_recv_info.recv_uri, sizeof(http_recv_info.recv_uri));
    // ============================
    // ファイルチェック
    //  種類に応じて分岐
    // ============================
    result = http_file_check(&http_recv_info);
    if (result == _OPENDIR){ // ディレクトリだが終端が '/' ではない
        char buffer[FILENAME_MAX];
        sprintf(buffer, "%s/", http_recv_info.recv_uri);
        http_redirect_response(accept_socket, &http_recv_info, buffer);
    }else if ( result < _FILE ){ // ファイルが見つからない
        http_not_found_response(accept_socket, &http_recv_info);
    }else if ( result == _FILE ){ // ファイル実体ならば、実体転送。
        // actionに、ImageViewerが指示されている？
        if ( strcasecmp(http_recv_info.action, "ImageView" ) == 0){
            // ----------------------------------------
            // イメージファイルビューアー
            // ----------------------------------------
            debug_log_output("Image Viewer start!\n");
            http_image_viewer(accept_socket, &http_recv_info);
            debug_log_output("Image Viewer end!\n");
            // actionに、SinglePlayが指示されている？
        }else if ( strcasecmp(http_recv_info.action, "SinglePlay" ) == 0){
            // ----------------------------------------
            // Musicファイル 単独プレイ
            // ----------------------------------------
            debug_log_output("Single Play start!\n");
            http_music_single_play(accept_socket, &http_recv_info);
            debug_log_output("Single Play end!\n");
        }else{ // アクションに指定無し。
            // ----------------------------------------
            // ファイルの実体
            // HTTPリクエストヘッダに従ってデータを返信。
            // ----------------------------------------
            debug_log_output("HTTP response start!\n");
            http_file_response(accept_socket, &http_recv_info);
            debug_log_output("HTTP response end!\n");
        }
    }else if ( result == _PLW ){
        // ---------------------------------------------
        // plw/uplファイル(`･ω･´)
        // リストファイルから、プレイリスト生成して返信
        // ---------------------------------------------
        debug_log_output("HTTP wizd play list create and response start!\n");
        http_listfile_to_playlist_create(accept_socket, &http_recv_info);
        debug_log_output("HTTP wizd play list create and response end!\n");
    }else if ( result == _VOB ){
        // ---------------------------------------------
        // vobファイル 連結
        // vobを連結して返信
        // ---------------------------------------------
        // actionに、SinglePlayが指示されている？
        if ( strcasecmp(http_recv_info.action, "SinglePlay" ) == 0){
            // ----------------------------------------
            // Musicファイル 単独プレイ
            // ----------------------------------------
            debug_log_output("Single Play start!(VOB)\n");
            http_music_single_play(accept_socket, &http_recv_info);
            debug_log_output("Single Play end!(VOB)\n");
        }else{ // アクションに指定無し。
            debug_log_output("HTTP vob file response start!\n");
            http_vob_file_response(accept_socket, &http_recv_info);
            debug_log_output("HTTP vob file response end!\n");
        }
    }else if ( result == _CGI ){
        // ---------------------------------------------
        // cgiファイル
        // cgiを実行して結果を返信
        // ---------------------------------------------
        debug_log_output("HTTP CGI response start! %d\n",accept_socket);
        http_cgi_response(accept_socket, &http_recv_info);
        debug_log_output("HTTP CGI response end!\n");
    }else{
        // ----------------------------------------
        // ディレクトリ 内にindex.html or index.htm or index.cgi があったら、そちらを表示する。
        // ----------------------------------------
        result = http_index( &http_recv_info );
        if( result == _FILE ){
            // ----------------------------------------
            // ファイルの実体
            // HTTPリクエストヘッダに従ってデータを返信。
            // ----------------------------------------
            debug_log_output("HTTP response start!\n");
            http_file_response(accept_socket, &http_recv_info);
            debug_log_output("HTTP response end!\n");
        }else if ( result == _CGI ){
            debug_log_output("HTTP CGI response start! %d\n",accept_socket);
            http_cgi_response(accept_socket, &http_recv_info);
            debug_log_output("HTTP CGI response end!\n"); 
        // ----------------------------------------
        // ディレクトリ
        // ----------------------------------------
        // actionに、OptionMenuが指示されている？
        }else if ( strcasecmp(http_recv_info.action, "OptionMenu" ) == 0){
            debug_log_output("HTTP Option menu create.\n");
            http_option_menu(accept_socket, &http_recv_info);
            debug_log_output("HTTP Option menu end.\n");
        }else{	// アクションに指定無し。
            // -------------------------------------
            // ディレクトリ検出
            // ファイルリストを生成して返信。
            // -------------------------------------
            debug_log_output("HTTP file menu create.\n");
            http_menu(accept_socket, &http_recv_info, result == _DIR ? _FILE : _DIR);
            debug_log_output("HTTP file menu end.\n");
        }
    }
    // ソケットクローズ
    //close(accept_socket);
    return;
}
/////////////////////////////////////////////////////////////////////////////////
FILETYPES http_index( HTTP_RECV_INFO* http_recv_info_p )
{
    unsigned char       document_path[MYFILENAME_MAX];
    unsigned char       read_filename[MYFILENAME_MAX];
    //Path Normalize.
    strncpy((char*)document_path, (char*)http_recv_info_p->send_filename, sizeof(document_path) );
    if ( document_path[ strlen((char*)document_path)-1 ] != DELIMITER[0] ){// 最後が'/'じゃなかったら、'/'を追加
        strncat((char*)document_path, DELIMITER, sizeof(document_path) -strlen((char*)document_path) );
    }
    // ----------------------------------------------
    // document_root/index.* のフルパス生成
    // ----------------------------------------------
    snprintf((char*)read_filename, sizeof( read_filename),"%sindex.html",(char*)document_path );
    if( access( (char*)read_filename , 0 ) == 0 ){
         strcat(http_recv_info_p->request_uri,"index.html");
         strcpy(http_recv_info_p->send_filename,read_filename);
         return _FILE;
    }
    snprintf((char*)read_filename, sizeof( read_filename),"%sindex.htm",(char*)document_path );
    if( access( (char*)read_filename , 0 ) == 0 ){
         strcat(http_recv_info_p->request_uri,"index.htm");
         strcpy(http_recv_info_p->send_filename,read_filename);
         return _FILE;
    }
    snprintf((char*)read_filename, sizeof( read_filename),"%sindex.php",(char*)document_path );
    if( access( (char*)read_filename , 0 ) == 0 ){
         strcat(http_recv_info_p->request_uri,"index.php");
         strcpy(http_recv_info_p->send_filename,read_filename);
         return _CGI;
    }
    snprintf((char*)read_filename, sizeof( read_filename),"%sindex.cgi",(char*)document_path );
    if( access( (char*)read_filename , 0 ) == 0 ){
         strcat(http_recv_info_p->request_uri,"index.cgi");
         strcpy(http_recv_info_p->send_filename,read_filename);
         return _CGI;
    }
    return _DIR;
}
// **************************************************************************
// HTTPヘッダを受信して解析する。
//
// 処理するのはGETのみ。GET以外のメソッドが来たらエラー
// 今のところ、URIとuser_agent、Range、Hostを解析。
// URIは、URIデコードもやる。
//
//	return: 0 		正常終了
//	return: 0以外 	エラー
// **************************************************************************
static int http_header_receive(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    int result = 0;
    int	recv_len;
    unsigned char	line_buf[1024];	// 大きめに。
    unsigned char 	work_buf[1024];
    unsigned char 	work_buf2[1024];
    unsigned char 	split1[1024];
    unsigned char 	split2[1024];
    int		ret;
    int		i;
    // ================================
    // １行づつ HTTPヘッダを受信
    // ================================
    for (i=0;;i++){
        // 1行受信 実行。
        memset(line_buf, '\0', sizeof(line_buf));
        recv_len = line_receive(accept_socket, line_buf, sizeof(line_buf));
        // debug. 受信したヘッダ表示
        debug_log_output("'%s'(%d byte)\n", line_buf, recv_len );
        // 受信した内容をチェック。
        if ( recv_len == 0 ){ // 空行検知。ヘッダ受信終了。
            break;
        }else if ( recv_len < 0 ){ // 受信失敗
            return ( -1 );
        }
        // --------------------------
        // GETメッセージチェック
        // --------------------------
        if ( i == 0 ){ // １行目のみチェック
            debug_log_output("URI Check start.'%s'\n", line_buf);
            // GETある？
            if ( strstr(line_buf, "GET") != NULL ){
                http_recv_info_p->isGet = 1;
            }else if ( strstr(line_buf, "HEAD") != NULL ){
                http_recv_info_p->isGet = 2;
            }else if ( strstr(line_buf, "POST") != NULL ){
                http_recv_info_p->isGet = 3;
            }else{
                http_recv_info_p->isGet = 0;
            }
            if ( http_recv_info_p->isGet == 0){
                debug_log_output("'GET' not found. error.");
                return ( -1 );
            }
            // 最初のスペースまでを削除。
            cut_before_character(line_buf, ' ');
            // 次にスペースが出てきたところの後ろまでを削除。
            cut_after_character(line_buf, ' ');
            // ===========================
            // GETオプション部解析
            // ===========================
            // REQUEST_URI用・Proxy用に値を保存
            strncpy(http_recv_info_p->request_uri, line_buf, sizeof(http_recv_info_p->request_uri));
            // '?'が存在するかチェック。
            if ( strchr(line_buf, '?') != NULL ){
                strncpy(work_buf, line_buf, sizeof(work_buf));
                // '?'より前をカット
                cut_before_character(work_buf, '?' );
                debug_log_output("work_buf = '%s'", work_buf );
                while ( 1 ){
                    memset(split1, '\0', sizeof(split1));
                    memset(split2, '\0', sizeof(split2));
                    // 最初に登場する'&'で分割
                    ret = sentence_split(work_buf, '&', split1, split2 );
                    if ( ret == 0 ){ // 分割成功
                        strncpy(work_buf, split2, sizeof(work_buf));
                    }else if (strlen(work_buf) > 0){ // まだwork_bufに中身ある？
                        strncpy( split1, work_buf, sizeof(split1));
                        strncpy( work_buf, "", sizeof(work_buf));
                    }else{ // 処理終了
                        break;
                    }
                    // -------------------------------------
                    // GETした内容 解析開始
                    // 超安直。いいのかこんな比較で。
                    // -------------------------------------
                    // URIデコード
                    uri_decode(work_buf2, sizeof(work_buf2), split1, sizeof(split1) );
                    // "page="あるか調査。
                    if (strncasecmp( work_buf2, "page=", strlen("page=") ) == 0 ){
                        // = より前を削除
                        cut_before_character(work_buf2, '=');
                        if ( strlen(work_buf2) > 0 ){
                            // 構造体に値を保存。
                            http_recv_info_p->page = atoi(work_buf2);
                        }
                        continue;
                    }
                    // "action="あるか調査。
                    if (strncasecmp( work_buf2, "action=", strlen("action=") ) == 0 ){
                        // = より前を削除
                        cut_before_character(work_buf2, '=');
                        // 構造体に値を保存。
                        strncpy(http_recv_info_p->action, work_buf2, sizeof(http_recv_info_p->action));
                        continue;
                    }
                    // "option="あるか調査
                    if (strncasecmp( work_buf2, "option=", strlen("option=") ) == 0 ){
                        // = より前を削除
                        cut_before_character(work_buf2, '=');
                        // 構造体に値を保存。
                        strncpy(http_recv_info_p->option, work_buf2, sizeof(http_recv_info_p->option));
                        continue;
                    }
                    // "sort="あるか調査
                    if (strncasecmp( work_buf2, "sort=", strlen("sort=") ) == 0 ){
                        // = より前を削除
                        cut_before_character(work_buf2, '=');
                        // 構造体に値を保存。
                        strncpy(http_recv_info_p->sort, work_buf2, sizeof(http_recv_info_p->sort));
                        continue;
                    }
                    // "focus="あるか調査
                    if (strncasecmp( work_buf2, "focus=", strlen("focus=") ) == 0 ){
                        // = より前を削除
                        cut_before_character(work_buf2, '=');
                        // 構造体に値を保存。
                        strncpy(http_recv_info_p->focus, work_buf2, sizeof(http_recv_info_p->focus));
                        continue;
                    }
                }
            }
            debug_log_output("http_recv_info_p->page = '%d'", http_recv_info_p->page);
            debug_log_output("http_recv_info_p->action = '%s'", http_recv_info_p->action);
            // URIデコード
            cut_after_character(line_buf, '?');
            uri_decode(work_buf, sizeof(work_buf), line_buf, sizeof(line_buf) );
            strncpy(line_buf, work_buf, sizeof(line_buf));
            debug_log_output("URI(decoded):'%s'\n", line_buf);
            convert_language_code(line_buf, work_buf, sizeof(work_buf), CODE_AUTO, CODE_EUC);
            debug_log_output("URI(decoded,euc,FYI):'%s'\n", work_buf);
            // 構造体に保存
            strncpy(http_recv_info_p->recv_uri, line_buf, sizeof(http_recv_info_p->recv_uri));
            continue;
        }
        // User-agent切り出し
        if ( strncasecmp(line_buf, HTTP_USER_AGENT, strlen(HTTP_USER_AGENT) ) == 0 ){
            debug_log_output("User-agent: Detect.\n");
            // ':'より前を切る
            cut_before_character(line_buf, ':');
            cut_first_character(line_buf, ' ');
            // 構造体に保存
            strncpy( http_recv_info_p->user_agent, line_buf, sizeof(http_recv_info_p->user_agent));
            continue;
        }
        // Rangeあるかチェック
        if ( strncasecmp(line_buf, HTTP_RANGE,	strlen(HTTP_RANGE) ) == 0 ){
            debug_log_output("%s Detect.\n", HTTP_RANGE);
            // ':' より前を切る。
            cut_before_character(line_buf, ':');
            cut_first_character(line_buf, ' ');
            // recv_range にRangeの中身保存
            strncpy(http_recv_info_p->recv_range, line_buf, sizeof(http_recv_info_p->recv_range));
            // '=' より前を切る
            cut_before_character(line_buf, '=');
            // '-'で前後に分割。
            sentence_split(line_buf, '-', work_buf, work_buf2);
            debug_log_output("wrok_buf='%s'\n", work_buf);
            debug_log_output("wrok_buf2='%s'\n", work_buf2);
            // 値を文字列→数値変換
            http_recv_info_p->range_start_pos  = strtoull((char*)work_buf, NULL, 10);
            if ( strlen(work_buf2) > 0 ){
                http_recv_info_p->range_end_pos = strtoull((char*)work_buf2, NULL, 10);
            }
            debug_log_output("range_start_pos=%d\n", http_recv_info_p->range_start_pos);
            debug_log_output("range_end_pos=%d\n", http_recv_info_p->range_end_pos);
            continue;
        }
        // Hostあるかチェック
        if ( strncasecmp(line_buf, HTTP_HOST,	strlen(HTTP_HOST) ) == 0 ){
            // ':' より前を切る。
            cut_before_character(line_buf, ':');
            cut_first_character(line_buf, ' ');
            strncpy(http_recv_info_p->recv_host, line_buf, sizeof(http_recv_info_p->recv_host));
            debug_log_output("%s Detect. %s '%s'", HTTP_HOST, HTTP_HOST, http_recv_info_p->recv_host);
            continue;
        }
        // Content-Lengthあるかチェック
        if ( strncasecmp(line_buf, HTTP_CONTENT_LENGTH1, strlen(HTTP_CONTENT_LENGTH1) ) == 0 ){
            // ':' より前を切る。
            cut_before_character(line_buf, ':');
            cut_first_character(line_buf, ' ');
            strncpy(http_recv_info_p->content_length, line_buf, sizeof(http_recv_info_p->content_length));
            debug_log_output("%s Detect. %s '%s'", HTTP_CONTENT_LENGTH1, HTTP_CONTENT_LENGTH1, http_recv_info_p->content_length);
            continue;
        }
        // Content-TYPEあるかチェック
        if ( strncasecmp(line_buf, HTTP_CONTENT_TYPE1, strlen(HTTP_CONTENT_TYPE1) ) == 0 ){
            // ':' より前を切る。
            cut_before_character(line_buf, ':');
            cut_first_character(line_buf, ' ');
            strncpy(http_recv_info_p->content_type, line_buf, sizeof(http_recv_info_p->content_type));
            debug_log_output("%s Detect. %s '%s'", HTTP_CONTENT_TYPE1, HTTP_CONTENT_TYPE1, http_recv_info_p->content_type);
            continue;
        }
    }
    return result;
}
// **************************************************************************
// リクエストされたURIのファイルをチェック
// documet_rootと、skin置き場をセットで探す。
//
// ret		 0:実体
//			 1:ディレクトリ
//			 3:plw/uplファイル(`･ω･´)
//			 4:tsvファイル(´・ω・`)
//			 5:VOBファイル
//			 6:CGIファイル
// **************************************************************************
static FILETYPES http_file_check( HTTP_RECV_INFO *http_recv_info_p)
{
    struct stat send_filestat={0};
    int result;
    unsigned char	file_extension[16];
    debug_log_output("http_file_check() start.");
    // ---------------
    // 作業用変数初期化
    // ---------------
    memset(http_recv_info_p->send_filename, '\0', sizeof(http_recv_info_p->send_filename));
    memset(file_extension, '\0', sizeof(file_extension));
    // -------------------------
    // ファイルチェック
    // -------------------------
    // 要求パスのフルパス生成。
    strncpy(http_recv_info_p->send_filename, global_param.document_root, sizeof(http_recv_info_p->send_filename));
    strncat(http_recv_info_p->send_filename, http_recv_info_p->recv_uri, sizeof(http_recv_info_p->send_filename));
    // '/' が重なってるところの重複を排除。
    duplex_character_to_unique(http_recv_info_p->send_filename, '/');
    debug_log_output("http_recv_info_p->send_filename='%s'\n", http_recv_info_p->send_filename);
    // ------------------------------------------------------------
    // ファイルあるかチェック。
    // ------------------------------------------------------------
    result = stat(http_recv_info_p->send_filename, &send_filestat);
    debug_log_output("stat: result=%d, st_mode=0x%04X, S_ISREG=%d, S_ISDIR=%d\n",
        result, send_filestat.st_mode, S_ISREG(send_filestat.st_mode), S_ISDIR(send_filestat.st_mode) );
    // stat()の結果で分岐。
    if ( ( result == 0 ) && ( S_ISREG(send_filestat.st_mode) == 1 ) ){
        // ファイル実体と検知
        debug_log_output("'%s' is File!!", http_recv_info_p->send_filename);
        debug_log_output("send_filestat.st_size= %lld\n", send_filestat.st_size);
        // -------------------------------------------
        // ファイルの拡張子より、Content-type を決定
        // -------------------------------------------
        filename_to_extension(http_recv_info_p->send_filename, file_extension, sizeof(file_extension));
        debug_log_output("http_recv_info_p->send_filename='%s', file_extension='%s'\n", http_recv_info_p->send_filename, file_extension);
        // 拡張子から、mime_typeを導く。
        check_file_extension_to_mime_type(file_extension, http_recv_info_p->mime_type,  sizeof(http_recv_info_p->mime_type));
        if (( strcasecmp(file_extension, "plw") == 0  ) ||
            ( strcasecmp(file_extension, "m3u") == 0  ) ||
            ( strcasecmp(file_extension, "upl") == 0  ) ){
            return ( _PLW );	// plw/upl ファイル
        }else if ( strcasecmp(file_extension, "vob") == 0  ){
            return ( _VOB );	// vobファイル
        }else if ( strcasecmp(file_extension, "tsv") == 0  ){
            return ( _TSV );	// tsvファイル
        }else if ( strcasecmp(file_extension, "pl") == 0  ){
            // CGIの実行が不許可なら、Not Found.
            return ( global_param.flag_execute_cgi ? _CGI : _NOTFOUND );  // cgiファイ
        }else if ( strcasecmp(file_extension, "cgi") == 0  ){
            // CGIの実行が不許可なら、Not Found.
            return ( global_param.flag_execute_cgi ? _CGI : _NOTFOUND );	// cgiファイル
        }else if ( strcasecmp(file_extension, "php") == 0  ){
            // CGIの実行が不許可なら、Not Found.
            return ( global_param.flag_execute_cgi ? _CGI : _NOTFOUND );  // cgiファイル
        }else{
            return ( _FILE );	// File実体
        }
    //ディレクトリチェック
    }else if ( ( result == 0 ) && ( S_ISDIR(send_filestat.st_mode) == 1 ) ){ // ディレクトリ
        int len = strlen(http_recv_info_p->recv_uri);
        if (len > 0 && http_recv_info_p->recv_uri[len - 1] != '/'){
            // '/' で終端していないディレクトリ要求の場合...
            return ( _OPENDIR );
        }
        // ディレクトリと検知
        debug_log_output("'%s' is Dir!!", http_recv_info_p->send_filename);
        return ( _DIR ) ;	// ディレクトリ
    //ファイルリストチェック
    }else{
        debug_log_output("stat() error\n", result);
        // ----------------------------------------------------------------------------
        // もし、実体が存在しなかったら、skin置き場に変えてもう一度チェック
        // Skin置き場は実体ファイルのみ認める。
        // ----------------------------------------------------------------------------
        debug_log_output("DocumentRoot not found. SkinDir Check.");
        debug_log_output("global_param.skin_root='%s'", global_param.skin_root);
        debug_log_output("global_param.skin_name='%s'", global_param.skin_name);
        // skin置き場にあるモノとして、フルパス生成。
        strncpy(http_recv_info_p->send_filename, global_param.skin_root, sizeof(http_recv_info_p->send_filename));
        strncat(http_recv_info_p->send_filename, global_param.skin_name, sizeof(http_recv_info_p->send_filename));
        strncat(http_recv_info_p->send_filename, http_recv_info_p->recv_uri, sizeof(http_recv_info_p->send_filename));
        // '/' が重なってるところの重複を排除。
        duplex_character_to_unique(http_recv_info_p->send_filename, '/');
        debug_log_output("SkinDir:http_recv_info_p->send_filename='%s'\n", http_recv_info_p->send_filename);
        // ------------------------------------------------------------
        // Skin置き場にファイルあるかチェック。
        // ------------------------------------------------------------
        result = stat(http_recv_info_p->send_filename, &send_filestat);
        debug_log_output("stat: result=%d, st_mode=%04X, S_ISREG=%d\n", result, send_filestat.st_mode, S_ISREG(send_filestat.st_mode));
        // ファイル実体と検知。
        if ( ( result == 0 ) && (S_ISREG(send_filestat.st_mode) == 1 ) ){
            // ファイル実体と検知
            debug_log_output("'%s' is File!!", http_recv_info_p->send_filename);
            debug_log_output("send_filestat.st_size= %lld\n", send_filestat.st_size);
            // -------------------------------------------
            // ファイルの拡張子より、Content-type を決定
            // -------------------------------------------
            filename_to_extension(http_recv_info_p->send_filename, file_extension, sizeof(file_extension));
            debug_log_output("http_recv_info_p->send_filename='%s', file_extension='%s'\n", http_recv_info_p->send_filename, file_extension);
            check_file_extension_to_mime_type(file_extension, http_recv_info_p->mime_type,  sizeof(http_recv_info_p->mime_type));
            return ( _FILE );	// File実体
        }else{
            // -------------------------------------
            // File Not Found.
            // やっぱり、404にしよう。
            // -------------------------------------
            return ( _NOTFOUND ) ;
        }
    }
    return ( _DIR );
}
// **************************************************************************
// accept_socketから、１行(CRLFか、LF単独が現れるまで)受信
// CRLFは削除する。
// 受信したサイズをreturnする。
// **************************************************************************
int line_receive(int accept_socket, unsigned char *line_buf_p, int line_max)
{
    unsigned char byte_buf;
    int 	line_len=0;
    int		recv_len;
    unsigned char *line_buf_work_p;
    line_buf_work_p = line_buf_p;
    line_len = 0;
    // １行受信実行
    while ( 1 ){
        recv_len = read(accept_socket, &byte_buf, 1);
        if ( recv_len != 1 ){ // 受信失敗チェック
            //debug_log_output("line_receive: read_len == -1");
            return ( -1 );
        }
        // CR/LFチェック
        if ( byte_buf == '\r' ){
            continue;
        }else if ( byte_buf == '\n' ){
            break;
        }
        // バッファにセット
        *line_buf_work_p = byte_buf;
        line_buf_work_p++;
        line_len++;
        // printf("line_len=%d, buf='%s'\n", line_len, line_buf_p);
        // 受信バッファサイズチェック
        if ( line_len >= line_max){
            // バッファオーバーフロー検知
            debug_log_output("line_buf over flow.");
            return line_len;
        }
    }
    return line_len;
}
static int http_redirect_response(int accept_socket, HTTP_RECV_INFO *http_recv_info, char *location)
{
    char buffer[FILENAME_MAX];
    snprintf(buffer, sizeof(buffer),
    "HTTP/1.1 301 Found\r\n"
    "Location: %s\r\n"
    "\r\n", location
    );
    write(accept_socket, buffer, strlen(buffer)+1);
    debug_log_output("Redirect to %s",location);
    return 0;
}
static int http_not_found_response(int accept_socket, HTTP_RECV_INFO *http_recv_info)
{
    char buffer[1024];
    sprintf(buffer,  "HTTP/1.x 404 Not Found\r\n"
                     "Content-Length: 9\r\n"
                     "Connection: Close\r\n"
                     "Content-Type: text/html; charset=UTF-8\r\n"
                     "\r\n"
                     "Not Found");
    write(accept_socket, buffer, strlen(buffer)+1);
    debug_log_output("Not Found %s", http_recv_info->request_uri);
    return 0;
    //return http_redirect_response(accept_socket, http_recv_info, "/");
}
