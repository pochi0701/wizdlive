//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_menu.c
//											$Revision: 1.39 $
//											$Date: 2004/07/24 05:01:45 $
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
#include <fcntl.h>
#include <time.h>
#include "wizd.h"
#include "wizd_aviread.h"
#define SORT_FILE_MASK   ( 0x000000FF )
#define SORT_DIR_MASK   ( 0x00000F00 )
#define SORT_DIR_FLAG(_A_)  ( ( _A_ & SORT_DIR_MASK ) >> 8 )
#define SORT_DIR_UP    ( 0x00000100 )
#define SORT_DIR_DOWN   ( 0x00000200 )
#define DEFAULT_SORT_RULE  SORT_NONE
typedef struct {
    unsigned char	name[255];		// 表示用ファイル名
    unsigned char	org_name[255];	// オリジナルファイル名
    mode_t			type;			// 種類
    size_t			size;			// サイズ
    // use size_t instead of size_t, since it contains st_size of stat.
    time_t			time;			// 日付
} FILE_INFO_T;
// スキン置換用データ（グローバル）
typedef struct  {
    unsigned char	current_path_name[FILENAME_MAX];	// 現パス 表示用(文字コード調整済み)
    unsigned char	current_directory_name[FILENAME_MAX];	// 現ディレクトリ 表示用(文字コード調整済み)
    unsigned char	current_directory_link[FILENAME_MAX];	// 現ディレクトリLink用（URIエンコード済み）
    unsigned char	current_directory_link_no_param[FILENAME_MAX];	// 現ディレクトリLink用（URIエンコード済み）
    unsigned char	parent_directory_link[FILENAME_MAX];	// 親ディレクトリLink用（URIエンコード済み）
    unsigned char	file_num_str[16];	// 現ディレクトリのファイル数表示用
    unsigned char	now_page_str[16];	// 現在のページ番号表示用
    unsigned char	max_page_str[16];	// 最大ページ番号表示用
    unsigned char	start_file_num_str[16];	// 現在ページの表示開始ファイル番号表示用
    unsigned char	end_file_num_str[16];	// 現在ページの表示開始ファイル番号表示用
    unsigned char	next_page_str[16];	// 次のページ(無ければmax_page)表示用
    unsigned char	prev_page_str[16];	// 一つ前のページ（無ければ 1)表示用
    unsigned char	focus[64];			// BODYタグ用 onloadset="$focus"
    int		stream_files;	// 再生可能ファイル数
    // 隠しディレクトリ情報
    unsigned char	secret_dir_link_html[512];
    // EUC現ディレクトリ名 (ディレクトリ同文字列削除機能用)
    unsigned char euc_current_dir_name[FILENAME_MAX];
    // クライアントがPCかどうか
    int		flag_pc;
} SKIN_REPLASE_GLOBAL_DATA_T;
// スキン置換用データ。（ファイル）
typedef struct  {
    int				stream_type;			// ストリームファイルか否か
    int				menu_file_type;			// ファイルの種類
    unsigned char	file_name[255];			// ファイル名表示用(文字コード調整済み)
    unsigned char	file_name_no_ext[255];	// 拡張子無しファイル名表示用(文字コード調整済み)
    unsigned char	file_extension[16];	// 拡張子のみ(文字コード調整済み)
    unsigned char	file_uri_link[FILENAME_MAX];	// ファイルへのLink(URIエンコード済み)
    unsigned char	file_timestamp[32];		// タイムスタンプ表示用
    unsigned char	file_timestamp_date[32];	// タイムスタンプ表示用 日付のみ
    unsigned char	file_timestamp_time[32];	// タイムスタンプ表示用 日時のみ
    unsigned char	file_size_string[32];	// ファイルサイズ表示用
    unsigned char	tvid_string[16];	// TVID表示用
    unsigned char	vod_string[32];		// vod="0" or vod="playlist"  必要に応じて付く
    int				row_num;		// 行番号
    unsigned char	image_width[16];	// 画像データ 横幅
    unsigned char	image_height[16];	// 画像データ 高さ
    // MP3 ID3v1 タグ情報
    unsigned char	mp3_id3v1_flag;			// MP3 タグ 存在フラグ
    unsigned char	mp3_id3v1_title[128];	// MP3 曲名
    unsigned char	mp3_id3v1_album[128];	// MP3 アルバム名
    unsigned char	mp3_id3v1_artist[128];	// MP3 アーティスト
    unsigned char	mp3_id3v1_year[128];		// MP3 制作年度
    unsigned char	mp3_id3v1_comment[128];	// MP3 コメント
    unsigned char	mp3_id3v1_title_info[128*4];			// MP3 曲名[アルバム名/アーティスト] まとめて表示
    unsigned char	mp3_id3v1_title_info_limited[128*4];	// MP3 曲名[アルバム名/アーティスト] まとめて表示(字数制限あり)
    unsigned char	avi_fps[16];
    unsigned char	avi_duration[32];
    unsigned char	avi_vcodec[128];
    unsigned char	avi_acodec[128];
    unsigned char	avi_hvcodec[128];
    unsigned char	avi_hacodec[128];
    unsigned char	avi_is_interleaved[32];
} SKIN_REPLASE_LINE_DATA_T;
// ImageViewer 置換用データ
typedef struct  {
    unsigned char	current_uri_name[FILENAME_MAX];	// 現URI 表示用(文字コード調整済み)
    unsigned char	current_uri_link[FILENAME_MAX];	// 現URI Link用（URIエンコード済み）
    unsigned char	parent_directory_link[FILENAME_MAX];	// 親ディレクトリLink用（URIエンコード済み）
    unsigned char	now_page_str[16];	// 現在のページ番号表示用
    unsigned char	file_timestamp[32];			// タイムスタンプ表示用(日時)
    unsigned char	file_timestamp_date[32];	// タイムスタンプ表示用(日付のみ)
    unsigned char	file_timestamp_time[32];		// タイムスタンプ表示用(時刻のみ)
    unsigned char	file_size_string[32];		// ファイルサイズ表示用
    unsigned char	image_width[16];			// 画像データ 横幅
    unsigned char	image_height[16];			// 画像データ 高さ
    unsigned char	image_viewer_width[16];		// 画像データ 表示横幅
    unsigned char	image_viewer_height[16];	// 画像データ 表示高さ
    unsigned char	image_viewer_mode[16];		// 表示モード
} SKIN_REPLASE_IMAGE_VIEWER_DATA_T;
#define  FILEMENU_BUF_SIZE (1024*16)
static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void  mp3_id3v1_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static int  mp3_id3v2_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void generate_mp3_title_info( SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code );
static int file_read_line( int fd, unsigned char *line_buf, int line_buf_size);
static void filename_adjustment_for_windows(unsigned char *filename, const unsigned char *pathname_plw);
// **************************************************************************
//
// HTTP_OKヘッダを生成して、メニュー画面返信実行
//
// **************************************************************************
static void http_filemenu_send(int accept_socket, unsigned char *filemenu_data)
{
    unsigned int	send_data_len;
    unsigned char	send_http_header_buf[1024];
    unsigned char	work_buf[1024];
    int	result_len;
    // --------------
    // OK ヘッダ生成
    // --------------
    memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));
    strncpy(send_http_header_buf, HTTP_OK, sizeof(send_http_header_buf));
    strncat(send_http_header_buf, HTTP_CONNECTION, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
    snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_TYPE, "text/html");
    strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf));
    snprintf(work_buf, sizeof(work_buf), HTTP_SERVER_NAME, SERVER_NAME);
    strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
    snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_LENGTH, strlen(filemenu_data) );
    strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
    strncat(send_http_header_buf, HTTP_END, sizeof(send_http_header_buf) - strlen(send_http_header_buf));
    send_data_len = strlen(send_http_header_buf);
    debug_log_output("header_send_data_len = %d\n", send_data_len);
    debug_log_output("--------\n");
    debug_log_output("%s", send_http_header_buf);
    debug_log_output("--------\n");
    // --------------
    // ヘッダ返信
    // --------------
    result_len = send(accept_socket, send_http_header_buf, send_data_len, 0);
    debug_log_output("header result_len=%d, send_data_len=%d\n", result_len, send_data_len);
    // --------------
    // 実体返信
    // --------------
    send_data_len = strlen(filemenu_data);
    result_len = send(accept_socket, filemenu_data, send_data_len, 0);
    debug_log_output("body result_len=%d, send_data_len=%d\n", result_len, send_data_len );
    return;
}
#define  SKIN_KEYWORD_IMAGE_VIEWER_WIDTH  "<!--WIZD_INSERT_IMAGE_VIEWER_WIDTH-->"  // ImageViewerの表示横幅
#define  SKIN_KEYWORD_IMAGE_VIEWER_HEIGHT "<!--WIZD_INSERT_IMAGE_VIEWER_HEIGHT-->" // ImageViewerの表示高さ
#define  SKIN_KEYWORD_IMAGE_VIEWER_MODE  "<!--WIZD_INSERT_IMAGE_VIEWER_MODE-->"  // ImageViewerの現在のモード
// FITモードの時、以下の範囲を削除
#define  SKIN_KEYWORD_DEL_IS_FIT_MODE "<!--WIZD_DELETE_IS_FIT_MODE-->"
#define  SKIN_KEYWORD_DEL_IS_FIT_MODE_E "<!--/WIZD_DELETE_IS_FIT_MODE-->"
// FITモード以外の時、以下の範囲を削除
#define  SKIN_KEYWORD_DEL_IS_NO_FIT_MODE  "<!--WIZD_DELETE_IS_NO_FIT_MODE-->"
#define  SKIN_KEYWORD_DEL_IS_NO_FIT_MODE_E "<!--/WIZD_DELETE_IS_NO_FIT_MODE-->"
#define  FIT_TERGET_WIDTH (533)
#define  FIT_TERGET_HEIGHT (400)
// **************************************************************************
// * Single Play Listを生成して返信
// **************************************************************************
void http_music_single_play(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    unsigned char	work_data[FILENAME_MAX*2];
    unsigned char	file_name[FILENAME_MAX];
    unsigned char	file_extension[16];
    unsigned char	file_uri_link[FILENAME_MAX];
    SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3タグ読み込み用
    // =========================================
    // URIからファイル名を取り出す。
    // =========================================
    strncpy( work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
    cut_before_last_character(work_data, '/');
    debug_log_output("file_name:'%s'", work_data);
    filename_to_extension(work_data, file_extension, sizeof(file_extension));
    // =========================================
    // 拡張子がmp3なら、ID3タグチェック。
    // =========================================
    mp3_id3tag_data.mp3_id3v1_flag = 0;
    if ( strcasecmp(file_extension, "mp3" ) == 0 )
    {
        // ID3タグチェック
        memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
        mp3_id3_tag_read( http_recv_info_p->send_filename, &mp3_id3tag_data );
    }
    // MP3 ID3タグが存在したならば、playlist 表示ファイル名をID3タグで置き換える。
    if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
    {
        strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, 	sizeof(work_data));
        strncat(work_data, ".mp3" ,sizeof(work_data) - strlen(work_data));	// ダミーの拡張子。playlist_filename_adjustment()で削除される。
        // =========================================
        // playlist表示用 ID3タグを調整
        // EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
        // =========================================
        playlist_filename_adjustment( work_data, file_name, sizeof(file_name), CODE_AUTO );
    }
    else
    {
        // =========================================
        // playlist表示用ファイル名 を調整
        // EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
        // =========================================
        playlist_filename_adjustment( work_data, file_name, sizeof(file_name), global_param.server_language_code );
    }
    debug_log_output("file_name(adjustment):'%s'", file_name );
    // =========================================
    // Link用URI(エンコード済み) を生成
    // =========================================
    strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
    uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );
    debug_log_output("file_uri_link='%s'\n", file_uri_link);
    // URIの拡張子置き換え処理。
    extension_add_rename(file_uri_link, sizeof(file_uri_link));
    // =========================================
    // プレイリストを生成
    // =========================================
    strncpy(work_data, file_name, sizeof(work_data));
    strncat(work_data, "|0|0|http://", sizeof(work_data) - strlen(work_data) );
    strncat(work_data, http_recv_info_p->recv_host, sizeof(work_data) - strlen(work_data) );
    strncat(work_data, file_uri_link, sizeof(work_data) - strlen(work_data) );
    strncat(work_data, "|\r\n", sizeof(work_data) - strlen(work_data) );
    debug_log_output("work_data='%s'", work_data);
    // =================
    // 返信実行
    // =================
    http_filemenu_send( accept_socket, work_data );
    return;
}
// **************************************************************************
// * wizd play listファイル(*.plw)より、PlayListを生成して返信
// **************************************************************************
void http_listfile_to_playlist_create(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    unsigned char *playlist_buf_malloc_p;
    int		fd;
    int		ret;
    unsigned char	buf[FILENAME_MAX];
    unsigned char	listfile_path[FILENAME_MAX];
    unsigned char	file_extension[32];
    unsigned char	file_name[255];
    unsigned char	file_uri_link[FILENAME_MAX];
    unsigned char	work_data[FILENAME_MAX *2];
    SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3タグ読み込み用
    // plwデータ、open
    fd = open(http_recv_info_p->send_filename, O_RDONLY);
    if ( fd < 0 )
    {
        debug_log_output("'%s' can't open.", http_recv_info_p->send_filename);
        return;
    }
    // listfileがあるパスを生成
    strncpy( listfile_path, http_recv_info_p->recv_uri, sizeof(listfile_path));
    cut_after_last_character( (unsigned char*)listfile_path, '/' );
    if ( listfile_path[strlen(listfile_path)-1] != '/' )
    {
        strncat( listfile_path, "/", sizeof(listfile_path) );
    }
    debug_log_output( "listfile_path: '%s'", listfile_path );
    // =============================
    // 返信データ生成領域をmalloc
    // =============================
    playlist_buf_malloc_p = (unsigned char*)malloc( FILEMENU_BUF_SIZE );
    if (playlist_buf_malloc_p == NULL)
    {
        debug_log_output("malloc() error");
        return;
    }
    memset( playlist_buf_malloc_p, '\0', FILEMENU_BUF_SIZE );
    //=====================================
    // プレイリスト 生成開始
    //=====================================
    while ( 1 )
    {
        // ファイルから、１行読む
        ret = file_read_line( fd, buf, sizeof(buf) );
        if ( ret < 0 )
        {
            debug_log_output("listfile EOF Detect.");
            break;
        }
        debug_log_output("-------------");
        debug_log_output("read_buf:'%s'", buf);
        // コメント削除
        if ( buf[0] == '#' )
        {
            buf[0] = '\0';
        }
        // 行末のスペースを削除。
        cut_character_at_linetail((char*)buf, ' ');
        debug_log_output("read_buf(comment cut):'%s'", buf);
        // 空行なら、continue
        if ( strlen( buf ) == 0 )
        {
            debug_log_output("continue.");
            continue;
        }
        // Windows用にプレイリスト内のファイル名を調整
        if (global_param.flag_filename_adjustment_for_windows){
            filename_adjustment_for_windows(buf, http_recv_info_p->send_filename);
        }
        // 拡張子 生成
        filename_to_extension(buf, file_extension, sizeof(file_extension) );
        debug_log_output("file_extension:'%s'", file_extension);
        // 表示ファイル名 生成
        strncpy(work_data, buf, sizeof(work_data));
        cut_before_last_character( work_data, '/' );
        strncpy( file_name, work_data, sizeof(file_name));
        debug_log_output("file_name:'%s'", file_name);
        // URI生成
        if ( buf[0] == '/' ) // 絶対パス
        {
            strncpy( file_uri_link, buf, sizeof(file_uri_link) );
        }
        else // 相対パス
        {
            strncpy( file_uri_link, listfile_path, sizeof(file_uri_link) );
            strncat( file_uri_link, buf, sizeof(file_uri_link) - strlen(file_uri_link) );
        }
        debug_log_output("listfile_path:'%s'", listfile_path);
        debug_log_output("file_uri_link:'%s'", file_uri_link);
        // -----------------------------------------
        // 拡張子がmp3なら、ID3タグチェック。
        // -----------------------------------------
        mp3_id3tag_data.mp3_id3v1_flag = 0;
        if ( strcasecmp(file_extension, "mp3" ) == 0 )
        {
            // MP3ファイルのフルパス生成
            strncpy(work_data, global_param.document_root, sizeof(work_data) );
            if ( work_data[strlen(work_data)-1] == '/' )
            work_data[strlen(work_data)-1] = '\0';
            strncat( work_data, file_uri_link, sizeof(work_data) );
            debug_log_output("full_path(mp3):'%s'", work_data); // フルパス
            // ID3タグチェック
            memset( &mp3_id3tag_data, 0, sizeof(mp3_id3tag_data));
            mp3_id3_tag_read(work_data , &mp3_id3tag_data );
        }
        // MP3 ID3タグが存在したならば、playlist 表示ファイル名をID3タグで置き換える。
        if ( mp3_id3tag_data.mp3_id3v1_flag == 1 )
        {
            strncpy(work_data, mp3_id3tag_data.mp3_id3v1_title_info, sizeof(work_data));
            strncat(work_data, ".mp3", sizeof(work_data) - strlen(work_data));	// ダミーの拡張子。playlist_filename_adjustment()で削除される。
            // =========================================
            // playlist表示用 ID3タグを調整
            // EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
            // =========================================
            playlist_filename_adjustment( work_data, file_name, sizeof(file_name), CODE_AUTO );
        }
        else
        // MP3 ID3タグが存在しないならば、ファイル名をそのまま使用する。
        {
            // ---------------------------------
            // 表示ファイル名 調整
            // EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を削除。
            // ---------------------------------
            strncpy( work_data, file_name, sizeof(work_data) );
            playlist_filename_adjustment(work_data, file_name, sizeof(file_name), global_param.server_language_code);
        }
        debug_log_output("file_name(adjust):'%s'", file_name);
        // ------------------------------------
        // Link用URI(エンコード済み) を生成
        // ------------------------------------
        strncpy(work_data, file_uri_link, sizeof(work_data) );
        uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );
        debug_log_output("file_uri_link(encoded):'%s'", file_uri_link);
        // URIの拡張子置き換え処理。
        extension_add_rename(file_uri_link, sizeof(file_uri_link));
        // ------------------------------------
        // プレイリストを生成
        // ------------------------------------
        strncpy(work_data, file_name, sizeof(work_data));
        strncat(work_data, "|0|0|http://", sizeof(work_data) - strlen(work_data) );
        strncat(work_data, http_recv_info_p->recv_host, sizeof(work_data) - strlen(work_data) );
        strncat(work_data, file_uri_link, sizeof(work_data) - strlen(work_data) );
        strncat(work_data, "|\r\n", sizeof(work_data) - strlen(work_data) );
        debug_log_output("work_data='%s'", work_data);
        // ------------------------------------
        // 送信バッファに追加
        // ------------------------------------
        strncat(playlist_buf_malloc_p, work_data,  FILEMENU_BUF_SIZE - strlen(playlist_buf_malloc_p) );
    }
    // =================
    // 返信実行
    // =================
    http_filemenu_send( accept_socket, playlist_buf_malloc_p );
    close( fd );
    free( playlist_buf_malloc_p );
    return;
}
// *****************************************************
// fd から１行読み込む
// 読み込んだ文字数がreturnされる。
// 最後まで読んだら、-1が戻る。
// *****************************************************
static int file_read_line( int fd, unsigned char *line_buf, int line_buf_size)
{
    int read_len;
    int	total_read_len;
    unsigned char	read_char;
    unsigned char *p;
    p = line_buf;
    total_read_len = 0;
    while ( 1 )
    {
        // 一文字read.
        read_len  = read(fd, &read_char, 1);
        if ( read_len <= 0 ) // EOF検知
        {
            return ( -1 );
        }
        else if ( read_char == '\r' )
        {
            continue;
        }
        else if ( read_char == '\n' )
        {
            break;
        }
        *p = read_char;
        p++;
        total_read_len++;
        if ( total_read_len >= line_buf_size )
        {
            break;
        }
    }
    *p = '\0';
    return total_read_len;
}
/********************************************************************************/
// 日本語文字コード変換。
// (libnkfのラッパー関数)
//
//	サポートされている形式は以下の通り。
//		in_flag:	CODE_AUTO, CODE_SJIS, CODE_EUC, CODE_UTF8, CODE_UTF16
//		out_flag: 	CODE_SJIS, CODE_EUC
/********************************************************************************/
void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag)
{
    unsigned char	nkf_option[8];
    memset(nkf_option, '\0', sizeof(nkf_option));
    //=====================================================================
    // in_flag, out_flagをみて、libnkfへのオプションを組み立てる。
    //=====================================================================
    switch( in_flag )
    {
    case CODE_SJIS:
        strncpy(nkf_option, "S", sizeof(nkf_option));
        break;
    case CODE_EUC:
        strncpy(nkf_option, "E", sizeof(nkf_option));
        break;
    case CODE_UTF8:
        strncpy(nkf_option, "W", sizeof(nkf_option));
        break;
    case CODE_UTF16:
        strncpy(nkf_option, "W16", sizeof(nkf_option));
        break;
    case CODE_AUTO:
    default:
        strncpy(nkf_option, "", sizeof(nkf_option));
        break;
    }
    switch( out_flag )
    {
    case CODE_EUC:
        strncat(nkf_option, "e", sizeof(nkf_option) - strlen(nkf_option) );
        break;
    case CODE_SJIS:
        strncat(nkf_option, "s", sizeof(nkf_option) - strlen(nkf_option) );
        break;
    case CODE_UTF8:
    default:
        strncat(nkf_option, "w", sizeof(nkf_option) - strlen(nkf_option) );
        break;
    }
    //=================================================
    // libnkf 実行
    //=================================================
    nkf((char*)in, (char*)out, len, (char*)nkf_option);
    //memcpy((char*)in,(char*)out,len);
    return;
}
static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
    if (mp3_id3v2_tag_read(mp3_filename, skin_rep_data_line_p) != 0) {
        mp3_id3v1_tag_read(mp3_filename, skin_rep_data_line_p);
    }
    if ( skin_rep_data_line_p->mp3_id3v1_flag > 0 ) {
        if ( skin_rep_data_line_p->mp3_id3v1_title[0] == '\0' ) {
            skin_rep_data_line_p->mp3_id3v1_flag = 0;
        } else {
            generate_mp3_title_info(skin_rep_data_line_p);
        }
    }
}
// **************************************************************************
// * MP3ファイルから、ID3v1形式のタグデータを得る
// **************************************************************************
static void  mp3_id3v1_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
    int	fd;
    unsigned char	buf[128];
    size_t		length;
    memset(buf, 			'\0', sizeof(buf));
    fd = open(mp3_filename,  O_RDONLY);
    if ( fd < 0 )
    {
        return;
    }
    // 最後から128byteへSEEK
    length = lseek(fd, -128, SEEK_END);
    if( length < 0 ){
        return;
    }
    // ------------------
    // "TAG"文字列確認
    // ------------------
    // 3byteをread.
    read(fd, buf, 3);
    debug_log_output("buf='%s'", buf);
    // "TAG" 文字列チェック
    if ( strncmp( buf, "TAG", 3 ) != 0 )
    {
        debug_log_output("NO ID3 Tag.");
        close(fd);
        return;		// MP3 タグ無し。
    }
    // ------------------------------------------------------------
    // Tag情報read
    //
    //	文字列最後に0xFFと' 'が付いていたら削除。
    //  client文字コードに変換。
    // ------------------------------------------------------------
    // 曲名
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    cut_character_at_linetail((char*)buf, 0xFF);
    cut_character_at_linetail((char*)buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_title,
    sizeof(skin_rep_data_line_p->mp3_id3v1_title),
    CODE_AUTO, global_param.client_language_code);
    // アーティスト
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    cut_character_at_linetail((char*)buf, 0xFF);
    cut_character_at_linetail((char*)buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_artist,
    sizeof(skin_rep_data_line_p->mp3_id3v1_artist),
    CODE_AUTO, global_param.client_language_code);
    // アルバム名
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    cut_character_at_linetail((char*)buf, 0xFF);
    cut_character_at_linetail((char*)buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_album,
    sizeof(skin_rep_data_line_p->mp3_id3v1_album),
    CODE_AUTO, global_param.client_language_code);
    // 制作年度
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 4);
    cut_character_at_linetail((char*)buf, 0xFF);
    cut_character_at_linetail((char*)buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_year,
    sizeof(skin_rep_data_line_p->mp3_id3v1_year),
    CODE_AUTO, global_param.client_language_code);
    // コメント
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 28);
    cut_character_at_linetail((char*)buf, 0xFF);
    cut_character_at_linetail((char*)buf, ' ');

    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_comment,
    sizeof(skin_rep_data_line_p->mp3_id3v1_comment),
    CODE_AUTO, global_param.client_language_code);
    // ---------------------
    // 存在フラグ
    // ---------------------
    skin_rep_data_line_p->mp3_id3v1_flag = 1;
    close(fd);
}
static unsigned long id3v2_len(unsigned char *buf)
{
    return buf[0] * 0x200000 + buf[1] * 0x4000 + buf[2] * 0x80 + buf[3];
}
// **************************************************************************
// * MP3ファイルから、ID3v2形式のタグデータを得る
// * 0: 成功  -1: 失敗(タグなし)
// **************************************************************************
static int  mp3_id3v2_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
    int	fd;
    unsigned char	buf[1024];
    unsigned char	*frame;
    size_t		len;
    struct _copy_list {
        unsigned char id[5];
        unsigned char *container;
        size_t maxlen;
    } copy_list[] = {
        { "TIT2", skin_rep_data_line_p->mp3_id3v1_title
        , sizeof(skin_rep_data_line_p->mp3_id3v1_title) },
        { "TPE1", skin_rep_data_line_p->mp3_id3v1_artist
        , sizeof(skin_rep_data_line_p->mp3_id3v1_artist) },
        { "TALB", skin_rep_data_line_p->mp3_id3v1_album
        , sizeof(skin_rep_data_line_p->mp3_id3v1_album) },
        { "TCOP", skin_rep_data_line_p->mp3_id3v1_year
        , sizeof(skin_rep_data_line_p->mp3_id3v1_year) },
        { "TYER", skin_rep_data_line_p->mp3_id3v1_year
        , sizeof(skin_rep_data_line_p->mp3_id3v1_year) },
        { "COMM", skin_rep_data_line_p->mp3_id3v1_comment
        , sizeof(skin_rep_data_line_p->mp3_id3v1_comment) },
    };
    int list_count = sizeof(copy_list) / sizeof(struct _copy_list);
    int i;
    int flag_extension = 0;
    memset(buf, '\0', sizeof(buf));
    fd = open(mp3_filename,  O_RDONLY);
    if ( fd < 0 )
    {
        return -1;
    }
    // ------------------
    // "ID3"文字列確認
    // ------------------
    // 10byteをread.
    read(fd, buf, 10);
    // debug_log_output("buf='%s'", buf);
    // "ID3" 文字列チェック
    if ( strncmp( buf, "ID3", 3 ) != 0 )
    {
        /*
        *  ファイルの後ろにくっついてる ID3v2 タグとか
        *  ファイルの途中にあるのとか 面倒だから 読まないよ。
        */
        debug_log_output("NO ID3v2 Tag.");
        close(fd);
        return -1;		// v2 タグ無し。
    }
    debug_log_output("ID3 v2.%d.%d Tag found", buf[3], buf[4]);
    debug_log_output("ID3 flag: %02X", buf[5]);
    if (buf[5] & 0x40) {
        debug_log_output("ID3 flag: an extended header exist.");
        flag_extension = 1;
    }
    len = id3v2_len(buf + 6);
    if (flag_extension) {
        int exlen;
        if (read(fd, buf, 6) != 6) {
            close(fd);
            return -1;
        }
        exlen = id3v2_len(buf);
        debug_log_output("ID3 ext. flag: len = %d", exlen);
        if (exlen < 6) {
            debug_log_output("invalid ID3 ext. header.");
            close(fd);
            return -1;
        } else if (exlen > 6) {
            debug_log_output("large ID3 ext. header.");
            lseek(fd, exlen - 6, SEEK_CUR);
        }
        len -= exlen;
    }
    // ------------------------------------------------------------
    // Tag情報read
    //
    //  client文字コードに変換。
    // ------------------------------------------------------------
    while (len > 0) {
        int frame_len;
        /* フレームヘッダを 読み込む */
        if (read(fd, buf, 10) != 10) {
            close(fd);
            return -1;
        }
        /* フレームの長さを算出 */
        frame_len = id3v2_len(buf + 4);
        /* フレーム最後まで たどりついた */
        unsigned long *ppp = (unsigned long*)buf;
        if (frame_len == 0 || *ppp == 0){
             break;
        }
        for (i=0; i<list_count; i++) {
            if (!strncmp(buf, copy_list[i].id, 4)) break;
        }
        if (i < list_count) {
            // 解釈するタグ 発見
            // 存在フラグ
            skin_rep_data_line_p->mp3_id3v1_flag = 1;
            frame = (unsigned char*)malloc(frame_len + 1);
            memset(frame, '\0', frame_len + 1);
            if (read(fd, frame, frame_len) != frame_len) {
                debug_log_output("ID3v2 Tag[%s] read failed", copy_list[i].id);
                free(frame);
                close(fd);
                return -1;
            }
            debug_log_output("ID3v2 Tag[%s] found. '%s'", copy_list[i].id, frame + 1);
            cut_character_at_linetail((char*)frame + 1, ' ');
            convert_language_code(	frame + 1,
            copy_list[i].container,
            copy_list[i].maxlen,
            CODE_AUTO,
            global_param.client_language_code);
            free(frame);
        } else {
            /* マッチしなかった */
            buf[4] = '\0';
            debug_log_output("ID3v2 Tag[%s] skip", buf);
            lseek(fd, frame_len, SEEK_CUR);
        }
        len -= (frame_len + 10); /* フレーム本体 + フレームヘッダ */
    }
    close(fd);
    return skin_rep_data_line_p->mp3_id3v1_flag ? 0 : -1;
}
static void generate_mp3_title_info( SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p )
{
    unsigned char	work_title_info[128];
    memset(work_title_info, '\0', sizeof(work_title_info));
    // ---------------------
    // mp3_title_info生成
    // ---------------------
    strncpy(work_title_info, skin_rep_data_line_p->mp3_id3v1_title, sizeof(work_title_info));
    if ( (strlen(skin_rep_data_line_p->mp3_id3v1_album) > 0) && (strlen(skin_rep_data_line_p->mp3_id3v1_artist) > 0) )
    {
        strncat(work_title_info, " [", 									sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_album, sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, "/", 									sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_artist,sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, "]", 									sizeof(work_title_info) - strlen(work_title_info));
    }
    else if ( (strlen(skin_rep_data_line_p->mp3_id3v1_album) ==0) && (strlen(skin_rep_data_line_p->mp3_id3v1_artist) > 0) )
    {
        strncat(work_title_info, " [", 									sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_artist,sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, "]", 									sizeof(work_title_info) - strlen(work_title_info));
    }
    else if ( (strlen(skin_rep_data_line_p->mp3_id3v1_album) > 0) && (strlen(skin_rep_data_line_p->mp3_id3v1_artist) ==0) )
    {
        strncat(work_title_info, " [", 									sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, skin_rep_data_line_p->mp3_id3v1_album, sizeof(work_title_info) - strlen(work_title_info));
        strncat(work_title_info, "]", 									sizeof(work_title_info) - strlen(work_title_info));
    }
    // mp3_title_info (制限無し)を保存。
    strncpy(skin_rep_data_line_p->mp3_id3v1_title_info, work_title_info, sizeof(skin_rep_data_line_p->mp3_id3v1_title_info) );
    // EUCに変換
    convert_language_code( 	work_title_info,
    skin_rep_data_line_p->mp3_id3v1_title_info,
    sizeof(skin_rep_data_line_p->mp3_id3v1_title_info),
    CODE_AUTO, CODE_EUC);
    strncpy(work_title_info, skin_rep_data_line_p->mp3_id3v1_title_info, sizeof(work_title_info));
    // CUT実行
    euc_string_cut_n_length(work_title_info, global_param.menu_filename_length_max);
    debug_log_output("mp3_title_info(cut)='%s'\n", work_title_info);
    // クライアント文字コードに。
    convert_language_code( 	work_title_info,
    skin_rep_data_line_p->mp3_id3v1_title_info_limited,
    sizeof(skin_rep_data_line_p->mp3_id3v1_title_info_limited),
    CODE_EUC, global_param.client_language_code);
    return;
}
// *************************************************************************************
// playlistに渡す表示ファイル名を、問題ない形式に調整する
// *************************************************************************************
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code )
{
    unsigned char	work_data[FILENAME_MAX];
    // ---------------------------------
    // ファイル名 生成 (表示用)
    // EUCに変換 → 拡張子削除 → (必要なら)半角文字を全角に変換 → MediaWizコードに変換 → SJISなら、不正文字コード0x7C('|')を含む文字を伏せ字に。
    // ---------------------------------
    convert_language_code(	src_name,
    dist_name,
    dist_size,
    input_code,
    CODE_EUC);
    if ( strchr(dist_name, '.') != NULL )
    {
        cut_after_last_character((unsigned char*)dist_name, '.');
    }
    debug_log_output("dist_name(euc)='%s'\n", dist_name);
    if (global_param.flag_allplay_filelist_adjust == TRUE)
    {
        han2euczen(dist_name, work_data, sizeof(work_data) );
        debug_log_output("dist_name(euczen)='%s'\n", work_data);
    }
    else
    {
        strncpy(work_data, dist_name, sizeof(work_data));
    }
    convert_language_code( work_data, dist_name, dist_size, CODE_AUTO, global_param.client_language_code);
    debug_log_output("dist_name(MediaWiz Code)='%s'\n", dist_name);
    if ( global_param.client_language_code == CODE_SJIS )
    {
        sjis_code_thrust_replace(dist_name, '|');
        debug_log_output("dist_name(SJIS '|' replace)='%s'\n", dist_name);
    }
    replace_character(dist_name, "|", "!");
    return;
}
static void http_sclist_send(int accept_socket, unsigned char *filemenu_data)
{
    unsigned int	send_data_len;
    unsigned char	send_http_header_buf[1024];
    unsigned char	work_buf[1024];
    int	result_len;
    // --------------
    // OK ヘッダ生成
    // --------------
    memset(send_http_header_buf, '\0', sizeof(send_http_header_buf));
    strncpy(send_http_header_buf, HTTP_OK, sizeof(send_http_header_buf));
    strncat(send_http_header_buf, HTTP_CONNECTION, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
    snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_TYPE, "audio/x-scpls");
    strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf));
    snprintf(work_buf, sizeof(work_buf), HTTP_SERVER_NAME, SERVER_NAME);
    strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
    snprintf(work_buf, sizeof(work_buf), HTTP_CONTENT_LENGTH, strlen(filemenu_data) );
    strncat(send_http_header_buf, work_buf, sizeof(send_http_header_buf) - strlen(send_http_header_buf) );
    strncat(send_http_header_buf, HTTP_END, sizeof(send_http_header_buf) - strlen(send_http_header_buf));
    send_data_len = strlen(send_http_header_buf);
    debug_log_output("header_send_data_len = %d\n", send_data_len);
    debug_log_output("--------\n");
    debug_log_output("%s", send_http_header_buf);
    debug_log_output("--------\n");
    // --------------
    // ヘッダ返信
    // --------------
    result_len = send(accept_socket, send_http_header_buf, send_data_len, 0);
    debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_data_len);
    // --------------
    // 実体返信
    // --------------
    send_data_len = strlen(filemenu_data);
    result_len = send(accept_socket, filemenu_data, send_data_len, 0);
    debug_log_output("result_len=%d, send_data_len=%d\n", result_len, send_data_len );
    return;
}
void http_uri_to_scplaylist_create(int accept_socket, char *uri_string)
{
    unsigned char work_buf[1024];
    unsigned char *p;
    p = work_buf;
    p += sprintf((char*)p, "[playlist]\n");
    p += sprintf((char*)p, "numberofentries=1\n");
    p += sprintf((char*)p, "File1=%s\n", uri_string);
    p += sprintf((char*)p, "Length1=-1\n");
    http_sclist_send(accept_socket, work_buf);
}
// Windows用にプレイリスト内のファイル名を調整
//
// 具体的には、以下を行う
// ・パス区切りの'\'を'/'に変更
// ・Windowsはcase insensitiveなので対応
//
// なお、playlist_filename_adjustmentという表示文字列用関数があるが混同しないこと!!!
static void filename_adjustment_for_windows(unsigned char *filename, const unsigned char *pathname_plw)
{
#define isleadbyte(c) ((0x81 <= (unsigned char)(c) && (unsigned char)(c) <= 0x9f) || (0xe0 <= (unsigned char)(c) && (unsigned char)(c) <= 0xfc))
    unsigned char pathname[FILENAME_MAX];
    unsigned char basename[FILENAME_MAX];
    unsigned char *curp;
    int sjis_f;
    if (filename == NULL || *filename == '\0')
    return;
    debug_log_output("filename_adjustment_for_windows: filename = [%s], pathname_plw = [%s]\n", filename, pathname_plw );
    // パス区切りの'\'を'/'に変更
    // sjisであることを前提にした処理です
    curp = filename;
    sjis_f = FALSE;
    while (*curp){
        if (!sjis_f){
            if (isleadbyte(*curp)){
                // sjisの1byte目
                sjis_f = TRUE;
            } else if (*curp == '\\'){
                *curp = '/';			// 置き換え
            }
        } else {
            // sjisの2byte目
            sjis_f = FALSE;
        }
        curp++;
    }
    // 先頭から一段目のディレクトリ名を取得
    if (*filename == '/'){
        // 絶対パス
        pathname[0] = '\0';
        curp = filename + 1;
    } else {
        // 相対パス
        strncpy(pathname, pathname_plw, sizeof(pathname));
        cut_after_last_character((unsigned char*)pathname, '/');
        curp = filename;
    }
    // 次のベース名を取得
    strncpy(basename, curp, sizeof(basename));
    cut_after_character(basename, '/');
    // ディレクトリ階層毎のループ
    while (1){
        int found = FALSE;
        DIR *dir;
        struct dirent *dent;
        // ディレクトリからcase insensitiveで探す
        debug_log_output("  SEARCH (case-insensitive). pathname = [%s], basename = [%s]\n", pathname, basename );
        dir = opendir((char*)pathname);
        if ( dir == NULL ){
            debug_log_output("Can't Open dir. pathname = [%s]\n", pathname );
            break;
        }
        // ディレクトリ内のループ
        while (1){
            dent = readdir(dir);
            if (dent == NULL){
                // 見つからない(おそらくプレイリストの記述ミス)
                debug_log_output("  NOT FOUND!!! [%s]\n", basename);
                break;
            }
            debug_log_output("    [%s]\n", dent->d_name);
            if (strcasecmp(dent->d_name, basename) == 0){
                // 見つけた
                debug_log_output("  FOUND!!! [%s]->[%s]\n", basename, dent->d_name);
                strncpy(curp, dent->d_name, strlen(dent->d_name));		// 該当部分を本来の名前に置き換え
                strncpy(basename, dent->d_name, sizeof(basename));
                found = TRUE;
                break;
            }
        }
        closedir(dir);
        if (found){
            // 次の階層に進む
            strncat(pathname, "/", sizeof(pathname));
            strncat(pathname, basename, sizeof(pathname));
            curp += strlen(basename);
            if (*curp == '\0'){
                // 終了
                debug_log_output("Loop end.\n");
                break;
            }
            curp++;
            strncpy(basename, curp, sizeof(basename));
            cut_after_character(basename, '/');
            if (*basename == '\0'){
                // 最後が'/'なのは変だけど、終了
                debug_log_output("Loop end ? (/)\n");
                break;
            }
        } else {
            // 結局見つからなかった
            // どうせ再生できないが、とりあえずそのまま...
            debug_log_output("NOT Found. pathname = %s, basename = [%s]\n", pathname, basename );
            break;
        }
    }
    debug_log_output("filename_adjustment_for_windows: EXIT. filename = [%s]\n", filename );
}
