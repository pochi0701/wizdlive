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
#include "test2.h"
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
    off_t			size;			// サイズ
    // use off_t instead of size_t, since it contains st_size of stat.
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
static int count_file_num(unsigned char *path);
static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);
static int count_file_num_in_tsv(unsigned char *path);
static int tsv_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num);
static int file_ignoral_check(char *name, unsigned char *path);
static void http_filemenu_send(int accept_socket, unsigned char *filemenu_data);
//static void create_system_filemenu(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size);
static void create_skin_filemenu(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size);
static void create_all_play_list(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size);
static unsigned char *skin_file_read(unsigned char *read_filename, int *malloc_size );
static void replace_skin_grobal_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p);
static void replace_skin_line_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p);
static void  mp3_id3_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void  mp3_id3v1_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static int  mp3_id3v2_tag_read(unsigned char *mp3_filename, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void generate_mp3_title_info( SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p );
static void playlist_filename_adjustment(unsigned char *src_name, unsigned char *dist_name, int dist_size, int input_code );
static int file_read_line( int fd, unsigned char *line_buf, int line_buf_size);
static void filename_adjustment_for_windows(unsigned char *filename, const unsigned char *pathname_plw);
static int read_avi_info(unsigned char *fname, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p);
static void 	file_info_sort( FILE_INFO_T *p, int num, unsigned long type );
static int 		_file_info_dir_sort( const void *in_a, const void *in_b, int order );
static int	 	_file_info_dir_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_dir_sort_order_down( const void *in_a, const void *in_b );
static int		_file_info_name_sort( const void *in_a, const void *in_b, int order );
static int 		_file_info_name_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_name_sort_order_down( const void *in_a, const void *in_b );
static int 		_file_info_size_sort( const void *in_a, const void *in_b, int order );
static int 		_file_info_size_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_size_sort_order_down( const void *in_a, const void *in_b );
static int 		_file_info_time_sort( const void *in_a, const void *in_b, int order );
static int 		_file_info_time_sort_order_up( const void *in_a, const void *in_b );
static int 		_file_info_time_sort_order_down( const void *in_a, const void *in_b );
static int 		_file_info_shuffle( const void *in_a, const void *in_b );
// ファイルソート用関数配列
static  void * file_sort_api[] = {
    NULL,
    (void *)_file_info_name_sort_order_up,
    (void *)_file_info_name_sort_order_down,
    (void *)_file_info_time_sort_order_up,
    (void *)_file_info_time_sort_order_down,
    (void *)_file_info_size_sort_order_up,
    (void *)_file_info_size_sort_order_down,
    (void *)_file_info_shuffle,
};
// ディレクトリソート用関数配列
static void * dir_sort_api[] = {
    NULL,
    (void *)_file_info_dir_sort_order_up,
    (void *)_file_info_dir_sort_order_down
};
// **************************************************************************
// ファイルリストを生成して返信
// **************************************************************************
void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p, int flag_pseudo)
{
    unsigned char	*filemenu_buf_malloc_p;
    int				file_num;	// DIR内のファイル数
    int				sort_rule;  // temp置き換え用
    unsigned char	*file_info_malloc_p;
    FILE_INFO_T		*file_info_p;
    if (!flag_pseudo) {
        // recv_uri の最後が'/'でなかったら、'/'を追加
        if (( strlen(http_recv_info_p->recv_uri) > 0 ) &&
        ( http_recv_info_p->recv_uri[strlen(http_recv_info_p->recv_uri)-1] != '/' ))
        {
            strncat(http_recv_info_p->recv_uri, "/", sizeof(http_recv_info_p->recv_uri) - strlen(http_recv_info_p->recv_uri) );
        }
        //  http_recv_info_p->send_filename の最後が'/'でなかったら、'/'を追加
        if (( strlen(http_recv_info_p->send_filename) > 0 ) &&
        ( http_recv_info_p->send_filename[strlen(http_recv_info_p->send_filename)-1] != '/' ))
        {
            strncat(http_recv_info_p->send_filename, "/", sizeof(http_recv_info_p->send_filename) - strlen(http_recv_info_p->send_filename) );
        }
    }
    // ==================================
    // ディレクトリ情報をＧＥＴ
    // ==================================
    if (flag_pseudo) {
        // recv_uri ディレクトリのファイル数を数える。
        file_num = count_file_num_in_tsv( http_recv_info_p->send_filename );
    } else {
        // recv_uri ディレクトリのファイル数を数える。
        file_num = count_file_num( http_recv_info_p->send_filename );
    }
    debug_log_output("file_num = %d", file_num);
    if ( file_num < 0 )
    {
        return;
    }
    // 必要な数だけ、ファイル情報保存エリアをmalloc()
    file_info_malloc_p = (unsigned char*)malloc( sizeof(FILE_INFO_T)*file_num );
    if ( file_info_malloc_p == NULL )
    {
        debug_log_output("malloc() error");
        return;
    }
    memset(file_info_malloc_p, 0, sizeof(FILE_INFO_T)*file_num);
    file_info_p = (FILE_INFO_T *)file_info_malloc_p;
    // -----------------------------------------------------
    // ファイル情報保存エリアに、ディレクトリ情報を読み込む。
    // -----------------------------------------------------
    if (flag_pseudo) {
        file_num = tsv_stat(http_recv_info_p->send_filename, file_info_p, file_num);
    } else {
        file_num = directory_stat(http_recv_info_p->send_filename, file_info_p, file_num);
    }
    debug_log_output("file_num = %d", file_num);
    // デバッグ。file_info_malloc_p 表示
    //for ( i=0; i<file_num; i++ )
    //{
        //	debug_log_output("file_info[%d] name='%s'", i, file_info_p[i].name );
        //	debug_log_output("file_info[%d] size='%d'", i, file_info_p[i].size );
        //	debug_log_output("file_info[%d] time='%d'", i, file_info_p[i].time );
    //}
    // ---------------------------------------------------
    // sort=でソートが指示されているか確認
    // 指示されていたら、それでglobal_paramを上書き
    // ---------------------------------------------------
    if ( strlen(http_recv_info_p->sort) > 0 )
    {
        if (strcasecmp(http_recv_info_p->sort ,"none") == 0 )
        global_param.sort_rule = SORT_NONE;
        else if (strcasecmp(http_recv_info_p->sort ,"name_up") == 0 )
        global_param.sort_rule = SORT_NAME_UP;
        else if (strcasecmp(http_recv_info_p->sort ,"name_down") == 0 )
        global_param.sort_rule = SORT_NAME_DOWN;
        else if (strcasecmp(http_recv_info_p->sort ,"time_up") == 0 )
        global_param.sort_rule = SORT_TIME_UP;
        else if (strcasecmp(http_recv_info_p->sort ,"time_down") == 0 )
        global_param.sort_rule = SORT_TIME_DOWN;
        else if (strcasecmp(http_recv_info_p->sort ,"size_up") == 0 )
        global_param.sort_rule = SORT_SIZE_UP;
        else if (strcasecmp(http_recv_info_p->sort ,"size_down") == 0 )
        global_param.sort_rule = SORT_SIZE_DOWN;
        else if (strcasecmp(http_recv_info_p->sort ,"shuffle") == 0 )
        global_param.sort_rule = SORT_SHUFFLE;
    } else if (flag_pseudo) global_param.sort_rule = SORT_NONE;
    
    sort_rule = global_param.sort_rule;
    
    // 0.12f4
    if (global_param.flag_specific_dir_sort_type_fix == TRUE) {
        // 特定ディレクトリでのSORT方法を固定
        unsigned char	*p;
        if ((p=(unsigned char*)strstr(http_recv_info_p->send_filename,"/video_ts/"))!=NULL) {
            if (*(p+strlen("/video_ts/"))=='\0')	//末尾?
            sort_rule = SORT_NAME_UP;
        }
        if ((p=(unsigned char*)strstr(http_recv_info_p->send_filename,"/TIMESHIFT/"))!=NULL) {
            if (*(p+strlen("/TIMESHIFT/"))=='\0')	//末尾?
            sort_rule = SORT_TIME_UP;
        }
        //if (strcmp(http_recv_info_p->send_filename,"video")==0)
        //	sort_rule = SORT_TIME_DOWN;
        // アーティスト、年、アルバム名、トラック+Sortクライテリア
        //
    }
    // 必要ならば、ソート実行
    if ( sort_rule != SORT_NONE )
    {
        file_info_sort( file_info_p, file_num, sort_rule | SORT_DIR_UP );
    }
    // =============================
    // 返信データ生成領域をmalloc
    // =============================
    filemenu_buf_malloc_p = (unsigned char*)malloc( FILEMENU_BUF_SIZE );
    if (filemenu_buf_malloc_p == NULL)
    {
        debug_log_output("malloc() error");
        return;
    }
    // -------------------------------------------
    // 自動再生
    // -------------------------------------------
    if ( strcasecmp(http_recv_info_p->action, "allplay") == 0 )
    {
        create_all_play_list(http_recv_info_p, file_info_p, file_num, filemenu_buf_malloc_p, FILEMENU_BUF_SIZE );
        debug_log_output("AllPlay List Create End!!! ");
    }
    // -------------------------------------------
    // ファイルメニュー生成
    // 外部スキンと組み込みスキンで分岐
    //	 （組み込みスキン、さっさと作れと小一時間……)
    // -------------------------------------------
    else
    {
        //	if ( global_param.flag_use_skin == TRUE )
        //	{
            create_skin_filemenu(http_recv_info_p, file_info_p, file_num, filemenu_buf_malloc_p, FILEMENU_BUF_SIZE );
        //	}
        //	else
        //	{
            //		create_system_filemenu(http_recv_info_p, file_info_p, file_num, filemenu_buf_malloc_p, FILEMENU_BUF_SIZE );
        //	}
        debug_log_output("Menu Create End!!! ");
    }
    debug_log_output("send_filemenu_size=%d", strlen(filemenu_buf_malloc_p)) ;
    // =================
    // 返信実行
    // =================
    http_filemenu_send( accept_socket, filemenu_buf_malloc_p );
    free(filemenu_buf_malloc_p);
    free(file_info_malloc_p);
    return;
}
// ****************************************************************************************
// メニュー生成用のdefineいろいろ。
// ****************************************************************************************
#define  SKIN_MENU_CONF      "wizd_skin.conf"
#define  SKIN_MENU_HEAD_HTML     "head.html"
#define  SKIN_MENU_LINE_MOVIE_FILE_HTML  "line_movie.html"
#define  SKIN_MENU_LINE_MUSIC_FILE_HTML  "line_music.html"
#define  SKIN_MENU_LINE_IMAGE_FILE_HTML  "line_image.html"
#define  SKIN_MENU_LINE_DOCUMENT_FILE_HTML "line_document.html"
#define  SKIN_MENU_LINE_UNKNOWN_FILE_HTML "line_unknown.html"
#define  SKIN_MENU_LINE_DIR_HTML    "line_dir.html"
#define  SKIN_MENU_LINE_PSEUDO_DIR_HTML  "line_pseudo.html"
#define  SKIN_MENU_TAIL_HTML     "tail.html"
#define  SKIN_KEYWORD_SERVER_NAME  "<!--WIZD_INSERT_SERVER_NAME-->"  // サーバ名＆バージョン。表示用
#define  SKIN_KEYWORD_CURRENT_PATH  "<!--WIZD_INSERT_CURRENT_PATH-->"  // 現PATH。表示用
#define  SKIN_KEYWORD_CURRENT_DIR_NAME "<!--WIZD_INSERT_CURRENT_DIR_NAME-->"  // 現ディレクトリ名。表示用
#define  SKIN_KEYWORD_CURRENT_DATE  "<!--WIZD_INSERT_CURRENT_DATE-->"    // 日付表示用
#define  SKIN_KEYWORD_CURRENT_TIME  "<!--WIZD_INSERT_CURRENT_TIME-->"    // 時刻表示用
#define  SKIN_KEYWORKD_PARLENT_DIR_LINK "<!--WIZD_INSERT_PARENT_DIR_LINK-->" // 親ディレクトリ。LINK用 URIエンコード済み
#define  SKIN_KEYWORKD_PARLENT_DIR_NAME "<!--WIZD_INSERT_PARENT_DIR_NANE-->" // 親ディレクトリ。表示用
#define  SKIN_KEYWORD_CURRENT_PATH_LINK "<!--WIZD_INSERT_CURRENT_PATH_LINK-->" // 現PATH。LINK用。URIエンコード済み
#define  SKIN_KEYWORD_CURRENT_PATH_LINK_NO_PARAM "<!--WIZD_INSERT_CURRENT_PATH_LINK_NO_PARAM-->" // 現PATH。LINK用。URIエンコード済み
#define  SKIN_KEYWORD_CURRENT_PAGE  "<!--WIZD_INSERT_CURRENT_PAGE-->"  // 現在のページ
#define  SKIN_KEYWORD_MAX_PAGE   "<!--WIZD_INSERT_MAX_PAGE-->"   // 全ページ数
#define  SKIN_KEYWORD_NEXT_PAGE   "<!--WIZD_INSERT_NEXT_PAGE-->"   // 次のページ
#define  SKIN_KEYWORD_PREV_PAGE   "<!--WIZD_INSERT_PREV_PAGE-->"   // 前のページ
#define  SKIN_KEYWORD_FILE_NUM   "<!--WIZD_INSERT_FILE_NUM-->"   // ディレクトリ存在ファイル数
#define  SKIN_KEYWORD_START_FILE_NUM  "<!--WIZD_INSERT_START_FILE_NUM-->"  // 次のページ
#define  SKIN_KEYWORD_END_FILE_NUM  "<!--WIZD_INSERT_END_FILE_NUM-->"  // 前のページ
#define  SKIN_KEYWORD_ONLOADSET_FOCUS  "<!--WIZD_INSERT_ONLOADSET_FOCUS-->"  // BODYタグ用 onloadset="$focus"
#define  SKIN_KEYWORD_LINE_FILE_NAME   "<!--WIZD_INSERT_LINE_FILE_NAME-->"   // ファイル名 表示用
#define  SKIN_KEYWORD_LINE_FILE_NAME_NO_EXT "<!--WIZD_INSERT_LINE_FILE_NAME_NO_EXT-->" // ファイル名(拡張子無し) 表示用
#define  SKIN_KEYWORD_LINE_FILE_EXT   "<!--WIZD_INSERT_LINE_FILE_EXT-->"   // ファイル拡張子 表示用
#define  SKIN_KEYWORD_LINE_FILE_LINK   "<!--WIZD_INSERT_LINE_FILE_LINK-->"   // ファイル名 リンク用 URIエンコード
#define  SKIN_KEYWORD_LINE_TIMESTAMP  "<!--WIZD_INSERT_LINE_TIMESTAMP-->"  // タイムスタンプ 日時(YYYY/MM/DD HH:MM) 表示用
#define  SKIN_KEYWORD_LINE_FILE_DATE  "<!--WIZD_INSERT_LINE_FILE_DATE-->"  // タイムスタンプ 日付のみ(YYYY/MM/DD) 表示用
#define  SKIN_KEYWORD_LINE_FILE_TIME  "<!--WIZD_INSERT_LINE_FILE_TIME-->"  // タイムスタンプ 時刻のみ(HH:MM) 表示用
#define  SKIN_KEYWORD_LINE_COLUMN_NUM "<!--WIZD_INSERT_LINE_COLUMN_NUM-->" // 行番号
#define  SKIN_KEYWORD_LINE_ROW_NUM "<!--WIZD_INSERT_LINE_ROW_NUM-->" // 行番号
#define  SKIN_KEYWORD_LINE_TVID   "<!--WIZD_INSERT_LINE_TVID-->"   // TVID
#define  SKIN_KEYWORD_LINE_FILE_VOD  "<!--WIZD_INSERT_LINE_FILE_VOD-->"  // vod="0"  必要に応じて付く。
#define  SKIN_KEYWORD_LINE_FILE_SIZE  "<!--WIZD_INSERT_LINE_FILE_SIZE-->"  // ファイルサイズ 表示用
#define  SKIN_KEYWORD_LINE_IMAGE_WIDTH "<!--WIZD_INSERT_LINE_IMAGE_WIDTH-->" // 画像の横幅
#define  SKIN_KEYWORD_LINE_IMAGE_HEIGHT "<!--WIZD_INSERT_LINE_IMAGE_HEIGHT-->" // 画像の高さ
#define  SKIN_KEYWORD_SECRET_DIR_LINK "<!--WIZD_INSERT_SECRET_DIR_LINK-->" // 隠しディレクトリ
#define  SKIN_KEYWORD_LINE_MP3TAG_TITLE  "<!--WIZD_INSERT_LINE_MP3TAG_TITLE-->"  // MP3タグ タイトル
#define  SKIN_KEYWORD_LINE_MP3TAG_ALBUM  "<!--WIZD_INSERT_LINE_MP3TAG_ALBUM-->"  // MP3タグ アルバム名
#define  SKIN_KEYWORD_LINE_MP3TAG_ARTIST  "<!--WIZD_INSERT_LINE_MP3TAG_ARTIST-->"  // MP3タグ アーティスト
#define  SKIN_KEYWORD_LINE_MP3TAG_YEAR  "<!--WIZD_INSERT_LINE_MP3TAG_YEAR-->"  // MP3タグ 制作年度
#define  SKIN_KEYWORD_LINE_MP3TAG_COMMENT "<!--WIZD_INSERT_LINE_MP3TAG_COMMENT-->" // MP3タグ コメント
#define  SKIN_KEYWORD_LINE_MP3TAG_TITLE_INFO "<!--WIZD_INSERT_LINE_MP3TAG_TITLE_INFO-->" // MP3タグ タイトル[アルバム名/アーティスト] 表示(menu_filename_length_maxによる制限も効く)
#define  SKIN_KEYWORD_LINE_AVI_FPS  "<!--WIZD_INSERT_LINE_AVI_FPS-->" // AVIのFPS
#define  SKIN_KEYWORD_LINE_AVI_DURATION "<!--WIZD_INSERT_LINE_AVI_DURATION-->" // AVIの再生時間
#define  SKIN_KEYWORD_LINE_AVI_VCODEC "<!--WIZD_INSERT_LINE_AVI_VCODEC-->" // AVIの動画コーデック
#define  SKIN_KEYWORD_LINE_AVI_ACODEC "<!--WIZD_INSERT_LINE_AVI_ACODEC-->" // AVIの音声コーデック
#define  SKIN_KEYWORD_LINE_AVI_HVCODEC "<!--WIZD_INSERT_LINE_AVI_HVCODEC-->" // AVIの動画コーデック(in the avi stream header)
#define  SKIN_KEYWORD_LINE_AVI_HACODEC "<!--WIZD_INSERT_LINE_AVI_HACODEC-->" // AVIの音声コーデック(in the avi stream header)
#define  SKIN_KEYWORD_LINE_AVI_IS_INTERLEAVED "<!--WIZD_INSERT_LINE_AVI_IS_INTERLEAVED-->" // AVIがインターリーブされているか
// 以下のキーワードで挟まれたエリアは、条件一致したときに削除される。
// ルートディレクトリの場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_ROOTDIR    "<!--WIZD_DELETE_IS_ROOTDIR-->"
#define  SKIN_KEYWORD_DEL_IS_ROOTDIR_E   "<!--/WIZD_DELETE_IS_ROOTDIR-->"
/* 後方互換 */
// 前ページが存在しない場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV  "<!--WIZD_DELETE_IS_NO_PAGE_PREV-->"
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV_E  "<!--/WIZD_DELETE_IS_NO_PAGE_PREV-->"
// 次ページが存在しない場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT  "<!--WIZD_DELETE_IS_NO_PAGE_NEXT-->"
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT_E  "<!--/WIZD_DELETE_IS_NO_PAGE_NEXT-->"
// 再生可能ファイルが存在しない場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES  "<!--WIZD_DELETE_IS_NO_STREAM_FILES-->"
#define  SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES_E "<!--/WIZD_DELETE_IS_NO_STREAM_FILES-->"
/* 新規, DELETE (IF THERE) IS NO... と読む */
// 前ページが存在しない場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV2  "<!--WIZD_IF_PAGE_PREV-->"
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV2_E  "<!--/WIZD_IF_PAGE_PREV-->"
// 次ページが存在しない場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT2  "<!--WIZD_IF_PAGE_NEXT-->"
#define  SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT2_E  "<!--/WIZD_IF_PAGE_NEXT-->"
// 再生可能ファイルが存在しない場合 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES2 "<!--WIZD_IF_STREAM_FILES-->"
#define  SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES2_E "<!--/WIZD_IF_STREAM_FILES-->"
// 前ページが存在する場合削除 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_PAGE_PREV  "<!--WIZD_IF_NO_PAGE_PREV-->"
#define  SKIN_KEYWORD_DEL_IS_PAGE_PREV_E  "<!--/WIZD_IF_NO_PAGE_PREV-->"
// 次ページが存在する場合削除 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_PAGE_NEXT  "<!--WIZD_IF_NO_PAGE_NEXT-->"
#define  SKIN_KEYWORD_DEL_IS_PAGE_NEXT_E  "<!--/WIZD_IF_NO_PAGE_NEXT-->"
// 再生可能ファイルが存在する場合削除 (HEAD/TAILのみ)
#define  SKIN_KEYWORD_DEL_IS_STREAM_FILES "<!--WIZD_IF_NO_STREAM_FILES-->"
#define  SKIN_KEYWORD_DEL_IS_STREAM_FILES_E "<!--/WIZD_IF_NO_STREAM_FILES-->"
// MP3タグが存在しないとき (LINEのみ)
#define  SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS   "<!--WIZD_DELETE_IS_NO_MP3_TAGS-->"
#define  SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS_E  "<!--/WIZD_DELETE_IS_NO_MP3_TAGS-->"
// MP3タグが存在するとき (LINEのみ)
#define  SKIN_KEYWORD_DEL_IS_HAVE_MP3_TAGS  "<!--WIZD_DELETE_IS_HAVE_MP3_TAGS-->"
#define  SKIN_KEYWORD_DEL_IS_HAVE_MP3_TAGS_E  "<!--/WIZD_DELETE_IS_HAVE_MP3_TAGS-->"
// 行が奇数/偶数のとき (LINEのみ)
#define  SKIN_KEYWORD_DEL_IF_LINE_IS_ODD   "<!--WIZD_IF_LINE_IS_EVEN-->" // 行が奇数のとき削除
#define  SKIN_KEYWORD_DEL_IF_LINE_IS_ODD_E  "<!--/WIZD_IF_LINE_IS_EVEN-->" // 行が奇数のとき削除
#define  SKIN_KEYWORD_DEL_IF_LINE_IS_EVEN  "<!--WIZD_IF_LINE_IS_ODD-->" // 行が偶数のとき削除
#define  SKIN_KEYWORD_DEL_IF_LINE_IS_EVEN_E  "<!--/WIZD_IF_LINE_IS_ODD-->" // 行が偶数のとき削除
// クライアントがPCのとき削除
#define  SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC  "<!--WIZD_IF_CLIENT_IS_NOT_PC-->"
#define  SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC_E  "<!--/WIZD_IF_CLIENT_IS_NOT_PC-->"
// クライアントがPCではないとき削除
#define  SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC  "<!--WIZD_IF_CLIENT_IS_PC-->"
#define  SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC_E  "<!--/WIZD_IF_CLIENT_IS_PC-->"
// focus が指定されているとき削除
#define  SKIN_KEYWORD_DEL_IF_FOCUS_IS_SPECIFIED  "<!--WIZD_IF_FOCUS_IS_NOT_SPECIFIED-->"
#define  SKIN_KEYWORD_DEL_IF_FOCUS_IS_SPECIFIED_E "<!--/WIZD_IF_FOCUS_IS_NOT_SPECIFIED-->"
// focus が指定されていないとき削除
#define  SKIN_KEYWORD_DEL_IF_FOCUS_IS_NOT_SPECIFIED  "<!--WIZD_IF_FOCUS_IS_SPECIFIED-->"
#define  SKIN_KEYWORD_DEL_IF_FOCUS_IS_NOT_SPECIFIED_E "<!--/WIZD_IF_FOCUS_IS_SPECIFIED-->"
// **************************************************************************
// スキンを使用したファイルメニューを生成
// **************************************************************************
static void create_skin_filemenu(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size)
{
    unsigned char	*menu_head_skin_p;
    int				menu_head_skin_malloc_size;
    unsigned char	*menu_tail_skin_p;
    int 			menu_tail_skin_malloc_size;
    unsigned char	*menu_line_movie_skin_p;
    int				menu_line_movie_skin_malloc_size;
    unsigned char	*menu_line_music_skin_p;
    int				menu_line_music_skin_malloc_size;
    unsigned char	*menu_line_image_skin_p;
    int				menu_line_image_skin_malloc_size;
    unsigned char	*menu_line_document_skin_p;
    int				menu_line_document_skin_malloc_size;
    unsigned char	*menu_line_unknown_skin_p;
    int				menu_line_unknown_skin_malloc_size;
    unsigned char	*menu_line_dir_skin_p;
    int				menu_line_dir_skin_malloc_size;
    unsigned char	*menu_line_pseudo_dir_skin_p;
    int				menu_line_pseudo_dir_skin_malloc_size;
    unsigned char	*menu_work_p;
    unsigned char	skin_path[FILENAME_MAX];
    unsigned char	read_filename[FILENAME_MAX];
    int		i, j;
    unsigned char	work_filename[FILENAME_MAX];
    unsigned char	work_data[FILENAME_MAX];
    unsigned char	work_data2[FILENAME_MAX];
    int		now_page;		// 現在のページ番号
    int		max_page;		// 最大ページ番号
    int		now_page_line;	// 現在のページの表示行数
    int		start_file_num;	// 現在ページの表示開始ファイル番号
    int		end_file_num;	// 現在ページの表示終了ファイル番号
    int		next_page;		// 次のページ(無ければmax_page)
    int		prev_page;		// 一つ前のページ（無ければ 1)
    SKIN_REPLASE_GLOBAL_DATA_T	*skin_rep_data_global_p;
    SKIN_REPLASE_LINE_DATA_T	*skin_rep_data_line_p;
    int skin_rep_line_malloc_size;
    int	count;
    unsigned int	image_width, image_height;
    struct	stat	dir_stat;
    int				result;
    // ==========================================
    // SKINコンフィグファイル読み込む
    // ==========================================
    // ----------------------------------------------
    // スキンのあるファイルパス生成（フルパス)
    // ----------------------------------------------
    strncpy(skin_path, global_param.skin_root, sizeof(skin_path) ); // スキン置き場
    if ( skin_path[ strlen(skin_path)-1 ] != '/' )// 最後が'/'じゃなかったら、'/'を追加
    {
        strncat(skin_path, "/", sizeof(skin_path) -strlen(skin_path) );
    }
    strncat(skin_path, global_param.skin_name, sizeof(skin_path) - strlen(skin_path) ); // スキン名（ディレクトリ）
    if ( skin_path[ strlen(skin_path)-1 ] != '/' ) // 最後が'/'じゃなかったら、'/'を追加
    {
        strncat(skin_path, "/", sizeof(skin_path) - strlen(skin_path) );
    }
    // ----------------------------------------------
    // SKINコンフィグファイルのフルパス生成
    // ----------------------------------------------
    strncpy(read_filename, skin_path, sizeof(read_filename) );
    strncat(read_filename, SKIN_MENU_CONF, sizeof(read_filename)- strlen(read_filename) );
    debug_log_output("skin: read_config_filename='%s'", read_filename);
    // ----------------------------------------------
    // SKIN コンフィグファイル 読み込み実行
    // ----------------------------------------------
    skin_config_file_read(read_filename);
    // ==========================================
    // HTML生成準備 各種計算等
    // ==========================================
    // ディレクトリ存在ファイル数
    debug_log_output("file_num = %d", file_num);
    // 最大ページ数計算
    if ( file_num == 0 )
    {
        max_page = 1;
    }
    else if ( (file_num % global_param.page_line_max) == 0 )
    {
        max_page = (file_num / global_param.page_line_max);
    }
    else
    {
        max_page = (file_num / global_param.page_line_max) + 1;
    }
    debug_log_output("max_page = %d", max_page);
    // 現在表示ページ番号 計算。
    if ( (http_recv_info_p->page <= 1 ) || (max_page < http_recv_info_p->page ) )
    now_page = 1;
    else
    now_page = http_recv_info_p->page;
    debug_log_output("now_page = %d", now_page);
    // 現在表示ページの表示行数計算。
    if ( max_page == now_page ) // 最後のページ
    now_page_line = file_num - (global_param.page_line_max * (max_page-1));
    else	// 最後以外なら、表示最大数。
    now_page_line = global_param.page_line_max;
    debug_log_output("now_page_line = %d", now_page_line);
    // 表示開始ファイル番号計算
    start_file_num = ((now_page - 1) * global_param.page_line_max);
    debug_log_output("start_file_num = %d", start_file_num);
    if ( max_page == now_page ) // 最後のページ
    end_file_num = file_num;
    else // 最後のページではなかったら。
    end_file_num = (start_file_num + global_param.page_line_max);
    debug_log_output("start_file_num = %d", start_file_num);
    // 前ページ番号 計算
    prev_page =  1 ;
    if ( now_page > 1 )
    prev_page = now_page - 1;
    // 次ページ番号 計算
    next_page = max_page ;
    if ( max_page > now_page )
    next_page = now_page + 1;
    debug_log_output("prev_page=%d  next_page=%d", prev_page ,next_page);
    // ===============================
    // スキン置換用データを準備
    // ===============================
    // 作業エリア確保
    skin_rep_data_global_p 	= (SKIN_REPLASE_GLOBAL_DATA_T*)malloc( sizeof(SKIN_REPLASE_GLOBAL_DATA_T) );
    if ( skin_rep_data_global_p == NULL )
    {
        debug_log_output("malloc() error.");
        return ;
    }
    memset(skin_rep_data_global_p, '\0', sizeof(SKIN_REPLASE_GLOBAL_DATA_T));
    skin_rep_line_malloc_size = sizeof(SKIN_REPLASE_LINE_DATA_T) * (global_param.page_line_max + 1);
    skin_rep_data_line_p      = (SKIN_REPLASE_LINE_DATA_T*)malloc( skin_rep_line_malloc_size );
    if ( skin_rep_data_line_p == NULL )
    {
        debug_log_output("malloc() error.");
        return ;
    }
    memset(skin_rep_data_line_p, '\0', sizeof(skin_rep_line_malloc_size));
    // ---------------------------------
    // グローバル 表示用情報 生成開始
    // ---------------------------------
    // 一つ上のディレクトリパス(親パス)を生成。(URIエンコード)
    strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) - strlen(work_data) );
    cut_after_last_character(work_data, '/'); //
    cut_after_last_character(work_data, '/');
    strncat(work_data, "/", sizeof(work_data) - strlen(work_data) ); // '/'を追加。
    debug_log_output("parent_directory='%s'", work_data);
    uri_encode(skin_rep_data_global_p->parent_directory_link, sizeof(skin_rep_data_global_p->parent_directory_link), work_data, strlen(work_data));
    // '?'を追加
    strncat(skin_rep_data_global_p->parent_directory_link, "?", sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
    // sort=が指示されていた場合、それを引き継ぐ。
    if ( strlen(http_recv_info_p->sort) > 0 )
    {
        snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
        strncat(skin_rep_data_global_p->parent_directory_link, work_data, sizeof(skin_rep_data_global_p->parent_directory_link) - strlen(skin_rep_data_global_p->parent_directory_link));
    }
    debug_log_output("parent_directory_link='%s'", skin_rep_data_global_p->parent_directory_link);
    // 現パス名 表示用生成(文字コード変換)
    strncpy( work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
    if ( global_param.flag_decode_samba_hex_and_cap == TRUE )
    {
        debug_log_output("current_path_name(cap/hex) = '%s'", work_data);
        decode_samba_hex_and_cap_coding( work_data );
        debug_log_output("current_path_name(bin) = '%s'", work_data);
    }
    // 現パス名 表示用生成(文字コード変換)
    convert_language_code(	work_data,
    skin_rep_data_global_p->current_path_name,
    sizeof(skin_rep_data_global_p->current_path_name),
    global_param.server_language_code,
    global_param.client_language_code );
    // 最後に'/'が付いていたら削除
    cut_character_at_linetail(work_data, '/');
    // '/'より前を削除
    cut_before_last_character(work_data, '/');
    // '/'を追加。
    strncat(work_data, "/", sizeof(work_data) - strlen(work_data) );
    // CUT実行
    euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);
    // 現ディレクトリ名 表示用生成(文字コード変換)
    convert_language_code(	work_data,
    skin_rep_data_global_p->current_directory_name,
    sizeof(skin_rep_data_global_p->current_directory_name),
    global_param.server_language_code,
    global_param.client_language_code );
    debug_log_output("current_path = '%s'", skin_rep_data_global_p->current_path_name );
    debug_log_output("current_dir = '%s'", skin_rep_data_global_p->current_directory_name );
    // 現パス名 Link用生成（URIエンコード）
    uri_encode(skin_rep_data_global_p->current_directory_link_no_param, sizeof(skin_rep_data_global_p->current_directory_link_no_param), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
    // 現パス名 Link用生成（URIエンコード）
    uri_encode(skin_rep_data_global_p->current_directory_link, sizeof(skin_rep_data_global_p->current_directory_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
    // ?を追加
    strncat(skin_rep_data_global_p->current_directory_link, "?", sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link)); // '?'を追加
    // sort=が指示されていた場合、それを引き継ぐ。
    if ( strlen(http_recv_info_p->sort) > 0 )
    {
        snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
        strncat(skin_rep_data_global_p->current_directory_link, work_data, sizeof(skin_rep_data_global_p->current_directory_link) - strlen(skin_rep_data_global_p->current_directory_link));
    }
    debug_log_output("current_directory_link='%s'", skin_rep_data_global_p->current_directory_link);
    // ディレクトリ存在ファイル数 表示用
    snprintf(skin_rep_data_global_p->file_num_str, sizeof(skin_rep_data_global_p->file_num_str), "%d", file_num );
    // 	現在のページ 表示用
    snprintf(skin_rep_data_global_p->now_page_str, sizeof(skin_rep_data_global_p->now_page_str), "%d", now_page );
    // 全ページ数 表示用
    snprintf(skin_rep_data_global_p->max_page_str, sizeof(skin_rep_data_global_p->max_page_str), "%d", max_page );
    // 次のページ 表示用
    snprintf(skin_rep_data_global_p->next_page_str, sizeof(skin_rep_data_global_p->next_page_str), "%d", next_page );
    // 前のページ 表示用
    snprintf(skin_rep_data_global_p->prev_page_str, sizeof(skin_rep_data_global_p->prev_page_str), "%d", prev_page );
    // 開始ファイル番号表示用
    if ( file_num == 0 )
    snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num );
    else
    snprintf(skin_rep_data_global_p->start_file_num_str, sizeof(skin_rep_data_global_p->start_file_num_str), "%d", start_file_num +1 );
    // 終了ファイル番号表示用
    snprintf(skin_rep_data_global_p->end_file_num_str, sizeof(skin_rep_data_global_p->end_file_num_str), "%d", end_file_num  );
    // ディレクトリ同名ファイル削除機能用
    if ( global_param.flag_filename_cut_same_directory_name == TRUE )
    {
        // 現バス名 (ディレクトリ同名ファイル削除機能で使用)
        convert_language_code(	http_recv_info_p->recv_uri,
        skin_rep_data_global_p->euc_current_dir_name,
        sizeof(skin_rep_data_global_p->euc_current_dir_name),
        global_param.server_language_code,
        CODE_EUC );
        // 最後に'/'が付いていたら削除
        cut_character_at_linetail(skin_rep_data_global_p->euc_current_dir_name, '/' );
        // '/'より前を削除
        cut_before_last_character(skin_rep_data_global_p->euc_current_dir_name, '/' );
        debug_log_output("euc_current_dir = '%s'", skin_rep_data_global_p->euc_current_dir_name );
    }
    skin_rep_data_global_p->stream_files = 0;	// 再生可能ファイル数カウント用
    // PCかどうかをスキン置換情報に追加
    skin_rep_data_global_p->flag_pc = http_recv_info_p->flag_pc;
    // BODYタグ用 onloadset="$focus"
    if (http_recv_info_p->focus[0]) {
        // 安全のため まず uri_encode する。
        // (to prevent, $focus = "start\"><a href=\"...."; hack. ;p)
        uri_encode(work_data, sizeof(work_data)
        , http_recv_info_p->focus, strlen(http_recv_info_p->focus));
        snprintf(skin_rep_data_global_p->focus
        , sizeof(skin_rep_data_global_p->focus)
        , " onloadset=\"%s\"", work_data);
    } else {
        skin_rep_data_global_p->focus[0] = '\0'; /* nothing :) */
    }
    // ---------------------------------
    // ファイル表示用情報 生成開始
    // ---------------------------------
    for ( i=start_file_num, count=0; i<(start_file_num + now_page_line) ; i++, count++ )
    {
        debug_log_output("-----< file info generate, count = %d >-----", count);
        // --------------------------------------------------------------------------------
        // 拡張子無しファイル名（表示用） 生成
        // ファイル名 長さ制限に合わせてCutもやる
        // ファイル名 → EUCコードに → 拡張子Cut → 文字列Cut → MediaWiz文字コードに
        // --------------------------------------------------------------------------------
        // EUCに。
        convert_language_code( 	file_info_p[i].name,
        work_data,
        sizeof(work_data),
        global_param.server_language_code,
        CODE_EUC);
        // 拡張子、有ればカット
        if ( strchr(work_data, '.') != NULL )
        {
            cut_after_last_character(work_data, '.');
        }
        debug_log_output("file_name_no_ext='%s'\n", work_data);
        // ()[]の削除フラグチェック
        // フラグがTRUEで、ファイルがディレクトリでなければ、括弧を削除する。
        if (( S_ISDIR( file_info_p[i].type ) == 0 ) &&
        ( global_param.flag_filename_cut_parenthesis_area == TRUE ))
        {
            cut_enclose_words(work_data, sizeof(work_data), "(", ")");
            cut_enclose_words(work_data, sizeof(work_data), "[", "]");
            debug_log_output("file_name_no_ext(enclose_words)='%s'\n", work_data);
        }
        // ディレクトリ同名文字列削除フラグチェック
        // フラグがTRUEで、ファイルがディレクトリでなければ、同一文字列を削除。
        if (( S_ISDIR( file_info_p[i].type ) == 0 ) 						&&
        ( global_param.flag_filename_cut_same_directory_name == TRUE ) 	&&
        ( strlen(skin_rep_data_global_p->euc_current_dir_name) > 0 )		)
        {
            // ディレクトリ同名文字列を""で置換
            replace_character_first(work_data, sizeof(work_data), skin_rep_data_global_p->euc_current_dir_name, "");
            // 頭に' 'が付いているようならば削除。
            cut_first_character(work_data, ' ');
            debug_log_output("file_name_no_ext(cut_same_directory_name)='%s'\n", work_data);
        }
        // 長さ制限を超えていたらCut
        debug_log_output("file_name_no_ext length=%d, menu_filename_length_max=%d", strlen(work_data) , global_param.menu_filename_length_max);
        // CUT実行
        euc_string_cut_n_length(work_data, global_param.menu_filename_length_max);
        debug_log_output("file_name_no_ext(cut)='%s'\n", work_data);
        // MediaWiz文字コードに
        convert_language_code(	work_data,
        skin_rep_data_line_p[count].file_name_no_ext,
        sizeof(skin_rep_data_line_p[count].file_name_no_ext),
        CODE_AUTO,
        global_param.client_language_code);
        debug_log_output("file_name_no_ext='%s'\n", skin_rep_data_line_p[count].file_name_no_ext);
        // ファイル名(拡張子無し) 生成終了
        // --------------------------------------
        // --------------------------------------------------------------------------------
        // 拡張子だけ生成
        // --------------------------------------------------------------------------------
        filename_to_extension(file_info_p[i].org_name, skin_rep_data_line_p[count].file_extension, sizeof(skin_rep_data_line_p[count].file_extension));
        cut_after_character(skin_rep_data_line_p[count].file_extension, '/');
        // filename_to_extension(file_info_p[i].name, skin_rep_data_line_p[count].file_extension, sizeof(skin_rep_data_line_p[count].file_extension));
        debug_log_output("file_extension='%s'\n", skin_rep_data_line_p[count].file_extension);
        // --------------------------------------------------------------------------------
        // ファイル名 生成 (表示用)  (no_extとextをくっつける)
        // --------------------------------------------------------------------------------
        strncpy(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_name_no_ext, sizeof(skin_rep_data_line_p[count].file_name));
        if ( strlen(skin_rep_data_line_p[count].file_extension) > 0 )
        {
            strncat(skin_rep_data_line_p[count].file_name, ".", sizeof(skin_rep_data_line_p[count].file_name));
            strncat(skin_rep_data_line_p[count].file_name, skin_rep_data_line_p[count].file_extension, sizeof(skin_rep_data_line_p[count].file_name));
        }
        // --------------------------------------------------------------------------------
        // Link用URI(エンコード済み) を生成
        // --------------------------------------------------------------------------------
        //strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );	// xxx
        //strncat(work_data, file_info_p[i].org_name, sizeof(work_data) - strlen(work_data) );
        strncpy(work_data, file_info_p[i].org_name, sizeof(work_data) );
        uri_encode(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link), work_data, strlen(work_data) );
        debug_log_output("file_uri_link='%s'\n", skin_rep_data_line_p[count].file_uri_link);
        // --------------------------------------------------------------------------------
        // ファイルスタンプの日時文字列を生成。
        // --------------------------------------------------------------------------------
        conv_time_to_string(skin_rep_data_line_p[count].file_timestamp, file_info_p[i].time );
        conv_time_to_date_string(skin_rep_data_line_p[count].file_timestamp_date, file_info_p[i].time );
        conv_time_to_time_string(skin_rep_data_line_p[count].file_timestamp_time, file_info_p[i].time );
        // --------------------------------------------------------------------------------
        // ファイルサイズ表示用文字列生成
        // --------------------------------------------------------------------------------
        conv_num_to_unit_string(skin_rep_data_line_p[count].file_size_string, file_info_p[i].size );
        debug_log_output("file_size=%llu", file_info_p[i].size );
        debug_log_output("file_size_string='%s'", skin_rep_data_line_p[count].file_size_string );
        // --------------------------------------------------------------------------------
        // tvid 表示用文字列生成
        // --------------------------------------------------------------------------------
        snprintf(skin_rep_data_line_p[count].tvid_string, sizeof(skin_rep_data_line_p[count].tvid_string), "%d", i+1 );
        // --------------------------------------------------------------------------------
        // vod_string 表示用文字列 とりあえず、""を。
        // --------------------------------------------------------------------------------
        strncpy(skin_rep_data_line_p[count].vod_string, "", sizeof(skin_rep_data_line_p[count].vod_string) );
        // --------------------------------------------------------------------------------
        // 行番号 記憶
        // --------------------------------------------------------------------------------
        skin_rep_data_line_p[count].row_num = count+1;
        // =========================================================
        // ファイルタイプ判定処理
        // =========================================================
        if ( S_ISDIR( file_info_p[i].type ) != 0 ) // ディレクトリか？
        {
            skin_rep_data_line_p[count].stream_type = TYPE_NO_STREAM;
            skin_rep_data_line_p[count].menu_file_type = TYPE_DIRECTORY;
        }
        else // ディレクトリ以外
        {
            skin_rep_data_line_p[count].stream_type = TYPE_NO_STREAM;
            skin_rep_data_line_p[count].menu_file_type = TYPE_UNKNOWN;
            for (j=0;;j++)
            {
                if (( mime_list[j].mime_name == NULL ) || ( strlen(skin_rep_data_line_p[count].file_extension) == 0 ) )
                break;
                if ( strcasecmp(mime_list[j].file_extension, skin_rep_data_line_p[count].file_extension) == 0 )
                {
                    skin_rep_data_line_p[count].stream_type = mime_list[j].stream_type;
                    skin_rep_data_line_p[count].menu_file_type = mime_list[j].menu_file_type;
                    break;
                }
            }
        }
        debug_log_output("menu_file_type=%d\n", skin_rep_data_line_p[count].menu_file_type);
        // =========================================================
        // ファイルタイプ毎に必要な文字列を追加で生成
        // =========================================================
        // ----------------------------
        // ディレクトリ 特定処理
        // ----------------------------
        if ( skin_rep_data_line_p[count].menu_file_type == TYPE_DIRECTORY ||
        skin_rep_data_line_p[count].menu_file_type == TYPE_PSEUDO_DIR )
        {
            // '?'を追加する。
            strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
            // sort=が指示されていた場合、それを引き継ぐ。
            if ( strlen(http_recv_info_p->sort) > 0 )
            {
                snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
                strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
            }
        }
        // -------------------------------------
        // ストリームファイル 特定処理
        // -------------------------------------
        if ( skin_rep_data_line_p[count].stream_type == TYPE_STREAM )
        {
            // vod_string に vod="0" をセット
            strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"0\"", sizeof(skin_rep_data_line_p[count].vod_string) );
            // 拡張子置き換え処理。
            extension_add_rename(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link));
            switch (skin_rep_data_line_p[count].menu_file_type) {
            case TYPE_MOVIE:
            case TYPE_MUSIC:
                // 再生可能ファイルカウント
                skin_rep_data_global_p->stream_files++;
                if (http_recv_info_p->flag_pc) {
                    snprintf(work_data, sizeof(work_data), "/-.-playlist.pls?http://%s%s%s"
                    , http_recv_info_p->recv_host
                    , http_recv_info_p->recv_uri
                    , file_info_p[i].org_name
                    );
                    uri_encode(skin_rep_data_line_p[count].file_uri_link, sizeof(skin_rep_data_line_p[count].file_uri_link), work_data, strlen(work_data) );
                    debug_log_output("file_uri_link(pc)='%s'\n", skin_rep_data_line_p[count].file_uri_link);
                } else if (strncmp(file_info_p[i].org_name, "/-.-", 4)) {
                    // SinglePlay モードにする。 使いたくない場合は この2行をばっさり削除
                    strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
                    strncat(skin_rep_data_line_p[count].file_uri_link, "?action=SinglePlay&", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
                }
                break;
            case TYPE_PLAYLIST:
            case TYPE_MUSICLIST:
                // ----------------------------
                // playlistファイル 特定処理
                // ----------------------------
                // vod_string に vod="playlist"をセット
                strncpy(skin_rep_data_line_p[count].vod_string, "vod=\"playlist\"", sizeof(skin_rep_data_line_p[count].vod_string) );
                break;
            default:
                // vod_string を 削除
                skin_rep_data_line_p[count].vod_string[0] = '\0';
                debug_log_output("unknown type");
                break;
            }
        }
        // ----------------------------
        // IMAGEファイル特定処理
        // ----------------------------
        if ( skin_rep_data_line_p[count].menu_file_type == TYPE_IMAGE )
        {
            // ----------------------------------
            // イメージファイルのフルパス生成
            // ----------------------------------
            strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
            if ( work_filename[strlen(work_filename)-1] != '/' )
            {
                strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
            }
            strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
            debug_log_output("work_filename(image) = %s", work_filename);
            // ------------------------
            // イメージのサイズをGET
            // ------------------------
            image_width = 0;
            image_height = 0;
            // 拡張子で分岐
            if ( (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpg" ) == 0 ) ||
            (strcasecmp( skin_rep_data_line_p[count].file_extension, "jpeg" ) == 0 ))
            {
                // JPEGファイルのサイズをGET
                jpeg_size( work_filename, &image_width, &image_height );
            }
            else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "gif" ) == 0 )
            {
                // GIFファイルのサイズをGET
                gif_size( work_filename, &image_width, &image_height );
            }
            else if (strcasecmp( skin_rep_data_line_p[count].file_extension, "png" ) == 0 )
            {
                // PNGファイルのサイズをGET
                png_size( work_filename, &image_width, &image_height );
            }
            // 画像サイズを文字列に、
            snprintf(skin_rep_data_line_p[count].image_width, sizeof(skin_rep_data_line_p[count].image_width), "%d", image_width );
            snprintf(skin_rep_data_line_p[count].image_height, sizeof(skin_rep_data_line_p[count].image_height), "%d", image_height );
            // ----------------------------------
            // リンクの最後に'?'を追加する。
            // ----------------------------------
            strncat(skin_rep_data_line_p[count].file_uri_link, "?", sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
            // sort=が指示されていた場合、それを引き継ぐ。
            if ( strlen(http_recv_info_p->sort) > 0 )
            {
                snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
                strncat(skin_rep_data_line_p[count].file_uri_link, work_data, sizeof(skin_rep_data_line_p[count].file_uri_link) - strlen(skin_rep_data_line_p[count].file_uri_link));
            }
        }
        // -------------------------------------
        // AVIファイル 特定処理
        // -------------------------------------
        if (skin_rep_data_line_p[count].menu_file_type == TYPE_MOVIE) {
            // set default value.
            snprintf(skin_rep_data_line_p[count].avi_is_interleaved, sizeof(skin_rep_data_line_p[count].avi_is_interleaved), "?");
            snprintf(skin_rep_data_line_p[count].image_width, sizeof(skin_rep_data_line_p[count].image_width), "???");
            snprintf(skin_rep_data_line_p[count].image_height, sizeof(skin_rep_data_line_p[count].image_height), "???");
            snprintf(skin_rep_data_line_p[count].avi_fps, sizeof(skin_rep_data_line_p[count].avi_fps), "???");
            snprintf(skin_rep_data_line_p[count].avi_duration, sizeof(skin_rep_data_line_p[count].avi_duration), "??:??:??");
            snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p[count].avi_vcodec), "[none]");
            snprintf(skin_rep_data_line_p[count].avi_acodec, sizeof(skin_rep_data_line_p[count].avi_acodec), "[none]");
            snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p[count].avi_hvcodec), "[none]");
            snprintf(skin_rep_data_line_p[count].avi_hacodec, sizeof(skin_rep_data_line_p[count].avi_hacodec), "[none]");
            if (strcasecmp(skin_rep_data_line_p[count].file_extension, "avi") == 0) {
                // ----------------------------------
                // AVIファイルのフルパス生成
                // ----------------------------------
                strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
                if ( work_filename[strlen(work_filename)-1] != '/' )
                {
                    strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
                }
                strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
                debug_log_output("work_filename(avi) = %s", work_filename);
                read_avi_info(work_filename, &skin_rep_data_line_p[count]);
            } else {
                // AVIじゃない
                snprintf(skin_rep_data_line_p[count].avi_vcodec, sizeof(skin_rep_data_line_p->avi_vcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
                snprintf(skin_rep_data_line_p[count].avi_hvcodec, sizeof(skin_rep_data_line_p->avi_hvcodec), "[%s]", skin_rep_data_line_p[count].file_extension);
            }
        }
        // -------------------------------------
        // MP3ファイル 特定処理
        // -------------------------------------
        if ( (skin_rep_data_line_p[count].menu_file_type == TYPE_MUSIC) &&
        (strcasecmp(skin_rep_data_line_p[count].file_extension, "mp3") == 0) )
        {
            // ----------------------------------
            // MP3ファイルのフルパス生成
            // ----------------------------------
            strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
            if ( work_filename[strlen(work_filename)-1] != '/' )
            {
                strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
            }
            strncat(work_filename, file_info_p[i].org_name, sizeof(work_filename) - strlen(work_filename));
            debug_log_output("work_filename(mp3) = %s", work_filename);
            // ------------------------
            // MP3のID3V1データをGET
            // ------------------------
            mp3_id3_tag_read(work_filename, &(skin_rep_data_line_p[count]) );
            // for Debug.
            if ( skin_rep_data_line_p[count].mp3_id3v1_flag > 0 )
            {
                debug_log_output("mp3 title:'%s'", 		skin_rep_data_line_p[count].mp3_id3v1_title);
                debug_log_output("mp3 album:'%s'", 		skin_rep_data_line_p[count].mp3_id3v1_album);
                debug_log_output("mp3 artist:'%s'", 	skin_rep_data_line_p[count].mp3_id3v1_artist);
                debug_log_output("mp3 year:'%s'", 		skin_rep_data_line_p[count].mp3_id3v1_year);
                debug_log_output("mp3 comment:'%s'",	skin_rep_data_line_p[count].mp3_id3v1_comment);
                debug_log_output("mp3 title_info:'%s'",	skin_rep_data_line_p[count].mp3_id3v1_title_info);
                debug_log_output("mp3 title_info_limited:'%s'",	skin_rep_data_line_p[count].mp3_id3v1_title_info_limited);
            }
        }
    }
    debug_log_output("-----< end file info generate, count = %d >-----", count);
    // ============================
    // 隠しディレクトリ検索
    // ============================
    memset(skin_rep_data_global_p->secret_dir_link_html, '\0', sizeof(skin_rep_data_global_p->secret_dir_link_html));
    // 隠しディレクトリが存在しているかチェック。
    for ( i=0; i<SECRET_DIRECTORY_MAX; i++)
    {
        if ( strlen(secret_directory_list[i].dir_name) > 0 )	// 隠しディレクトリ指定有り？
        {
            // ----------------------------------
            // 隠しディレクトリのフルパス生成
            // ----------------------------------
            strncpy(work_filename, http_recv_info_p->send_filename, sizeof(work_filename) );
            if ( work_filename[strlen(work_filename)-1] != '/' )
            {
                strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
            }
            strncat(work_filename, secret_directory_list[i].dir_name, sizeof(work_filename) - strlen(work_filename));
            debug_log_output("check: work_filename = %s", work_filename);
            // 存在チェック
            result = stat(work_filename, &dir_stat);
            if ( result == 0 )
            {
                if ( S_ISDIR(dir_stat.st_mode) != 0 ) // ディレクトリ存在！
                {
                    // 存在してたら、リンク用URI生成
                    strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
                    strncat(work_data, secret_directory_list[i].dir_name, sizeof(work_data) - strlen(work_data) );
                    uri_encode(work_data2, sizeof(work_data2), work_data, strlen(work_data) );
                    // HTML生成
                    snprintf(work_data, sizeof(work_data), "<a href=\"%s\" tvid=%d></a> ", work_data2, secret_directory_list[i].tvid);
                    debug_log_output("secret_dir_html='%s'", work_data);
                    strncat( skin_rep_data_global_p->secret_dir_link_html, work_data, sizeof(skin_rep_data_global_p->secret_dir_link_html) - strlen(skin_rep_data_global_p->secret_dir_link_html) );
                }
            }
        }
        else
        {
            break;
        }
    }
    debug_log_output("secret_dir_html='%s'", skin_rep_data_global_p->secret_dir_link_html);
    // ======================================================================================
    // ======================================================================================
    //  スキンデータ処理開始。
    // ======================================================================================
    // ======================================================================================
    // ===============================
    // HEAD スキンファイル 読み込み
    // ===============================
    // HEAD スキンファイル名生成
    strncpy(read_filename, skin_path, sizeof(read_filename) );
    strncat(read_filename, SKIN_MENU_HEAD_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    // ファイル読み込み
    menu_head_skin_p = skin_file_read(read_filename, &menu_head_skin_malloc_size );
    if ( menu_head_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return ;
    }
    // ------------------------------------------------------------
    //  HEAD 置換処理
    // ------------------------------------------------------------
    replace_skin_grobal_data(menu_head_skin_p, menu_head_skin_malloc_size, skin_rep_data_global_p);
    // ----------------------
    // 結果をコピー
    // ----------------------
    strncpy(send_filemenu_buf, menu_head_skin_p, buf_size );
    free(menu_head_skin_p);
    debug_log_output("skin: free() end." );
    // ====================================
    // LINE 置換処理
    // ====================================
    // 作業エリア確保
    menu_work_p = (unsigned char*)malloc(buf_size);
    if ( menu_work_p == NULL)
    {
        debug_log_output("malloc() error.");
        return;
    }
    // ===============================
    // スキンデータ読み込み
    // ===============================
    // DIRスキン 読み込み
    strncpy(read_filename, skin_path, sizeof(read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_DIR_HTML, sizeof(read_filename)-strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_dir_skin_p = skin_file_read( read_filename, &menu_line_dir_skin_malloc_size );
    if ( menu_line_dir_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // PSEUDO_DIRスキン 読み込み
    strncpy(read_filename, skin_path, sizeof(read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_PSEUDO_DIR_HTML, sizeof(read_filename)-strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_pseudo_dir_skin_p = skin_file_read( read_filename, &menu_line_pseudo_dir_skin_malloc_size );
    if ( menu_line_pseudo_dir_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // MOVIE スキン読み込み
    strncpy(read_filename, skin_path, sizeof( read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_MOVIE_FILE_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_movie_skin_p = skin_file_read( read_filename, &menu_line_movie_skin_malloc_size );
    if ( menu_line_movie_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // MUSIC スキン読み込み
    strncpy(read_filename, skin_path, sizeof( read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_MUSIC_FILE_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_music_skin_p = skin_file_read( read_filename, &menu_line_music_skin_malloc_size );
    if ( menu_line_music_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // IMAGE スキン読み込み
    strncpy(read_filename, skin_path, sizeof( read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_IMAGE_FILE_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_image_skin_p = skin_file_read( read_filename, &menu_line_image_skin_malloc_size );
    if ( menu_line_image_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // DOCUMENT スキン読み込み
    strncpy(read_filename, skin_path, sizeof( read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_DOCUMENT_FILE_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_document_skin_p = skin_file_read( read_filename, &menu_line_document_skin_malloc_size );
    if ( menu_line_document_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // UNKNOWN スキン読み込み
    strncpy(read_filename, skin_path, sizeof( read_filename) );
    strncat(read_filename, SKIN_MENU_LINE_UNKNOWN_FILE_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    menu_line_unknown_skin_p = skin_file_read( read_filename, &menu_line_unknown_skin_malloc_size );
    if ( menu_line_unknown_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // =================================
    //  処理開始。
    // =================================
    for ( count=0; count< now_page_line;  count++ )
    {
        // --------------------------------------------------------
        // ファイルタイプにより、対応するスキンデータをcopy
        // --------------------------------------------------------
        switch (skin_rep_data_line_p[count].menu_file_type) {
        case TYPE_DIRECTORY:
            strncpy(menu_work_p, menu_line_dir_skin_p, buf_size );
            break;
        case TYPE_PSEUDO_DIR:
            strncpy(menu_work_p, menu_line_pseudo_dir_skin_p, buf_size );
            break;
        case TYPE_MOVIE:
        case TYPE_PLAYLIST:
            strncpy(menu_work_p, menu_line_movie_skin_p, buf_size );
            break;
        case TYPE_MUSIC:
        case TYPE_MUSICLIST:
            strncpy(menu_work_p, menu_line_music_skin_p, buf_size );
            break;
        case TYPE_IMAGE:
            strncpy(menu_work_p, menu_line_image_skin_p, buf_size );
            break;
        case TYPE_DOCUMENT:
            strncpy(menu_work_p, menu_line_document_skin_p, buf_size );
            break;
        default:
            strncpy(menu_work_p, menu_line_unknown_skin_p, buf_size );
            break;
        }
        // -----------------------------
        // LINE 置換処理
        // -----------------------------
        replace_skin_line_data(menu_work_p, buf_size, &(skin_rep_data_line_p[count]) );
        replace_skin_grobal_data(menu_work_p, buf_size, skin_rep_data_global_p);
        // ------------------------------
        // FILE_NAME 結果をコピー
        // ------------------------------
        strncat(send_filemenu_buf, menu_work_p, buf_size );
    }
    // 作業領域解放
    free( menu_work_p );
    free( menu_line_movie_skin_p );
    free( menu_line_music_skin_p );
    free( menu_line_image_skin_p );
    free( menu_line_document_skin_p );
    free( menu_line_unknown_skin_p );
    free( menu_line_dir_skin_p );
    free( menu_line_pseudo_dir_skin_p );
    // =======================
    // TAIL ファイル名
    // =======================
    // ファイル名生成
    strncpy(read_filename, skin_path, sizeof(read_filename) );
    strncat(read_filename, SKIN_MENU_TAIL_HTML, sizeof(read_filename) - strlen(read_filename) );
    debug_log_output("skin: read_filename='%s'", read_filename);
    // ファイル読み込み
    menu_tail_skin_p = skin_file_read( read_filename,  &menu_tail_skin_malloc_size );
    if ( menu_tail_skin_p == NULL )
    {
        debug_log_output("skin_file_read() error.");
        return;
    }
    // ------------------------------------------------------------
    // ============================================================
    //  TAIL 置換処理
    // ============================================================
    // ------------------------------------------------------------
    replace_skin_grobal_data(menu_tail_skin_p, menu_tail_skin_malloc_size, skin_rep_data_global_p);
    // ---------------
    // 結果をコピー
    // ---------------
    strncat(send_filemenu_buf, menu_tail_skin_p, buf_size - strlen(send_filemenu_buf));
    free(menu_tail_skin_p);
    free( skin_rep_data_global_p );
    free( skin_rep_data_line_p );
    return;
}
// **************************************************************************
// 全体用データ まとめて置換
// **************************************************************************
static void replace_skin_grobal_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_GLOBAL_DATA_T *skin_rep_data_global_p)
{
    unsigned char	current_date[32];	// 現在の日付表示用
    unsigned char	current_time[32];	// 現在の時刻表示用
    // ===============================
    // = 削除キーワードを削除する
    // ===============================
    // ルートパスの場合。
    if ( strcmp(skin_rep_data_global_p->current_path_name, "/" ) == 0 )
    {
        //debug_log_output("SKIN_KEYWORD_DEL_IS_ROOTDIR hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_ROOTDIR
        , SKIN_KEYWORD_DEL_IS_ROOTDIR_E);
    }
    // １ページの場合
    if ( strcmp(skin_rep_data_global_p->now_page_str, "1" ) == 0 )
    {
        //debug_log_output("SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV_E);
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV2
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_PREV2_E);
    } else {
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_PAGE_PREV
        , SKIN_KEYWORD_DEL_IS_PAGE_PREV_E);
    }
    // 最後のページの場合
    if ( strcmp(skin_rep_data_global_p->now_page_str, skin_rep_data_global_p->max_page_str) == 0 )
    {
        //debug_log_output("SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT_E);
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT2
        , SKIN_KEYWORD_DEL_IS_NO_PAGE_NEXT2_E);
    } else {
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_PAGE_NEXT
        , SKIN_KEYWORD_DEL_IS_PAGE_NEXT_E);
    }
    // 再生可能ファイル数が０の場合
    if ( skin_rep_data_global_p->stream_files == 0 )
    {
        //debug_log_output("SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES
        , SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES_E);
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES2
        , SKIN_KEYWORD_DEL_IS_NO_STREAM_FILES2_E);
    } else {
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IS_STREAM_FILES
        , SKIN_KEYWORD_DEL_IS_STREAM_FILES_E);
    }
    // クライアントがPCのときとそうじゃないとき
    if ( skin_rep_data_global_p->flag_pc == 1 )
    {
        //debug_log_output("SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC
        , SKIN_KEYWORD_DEL_IF_CLIENT_IS_PC_E);
    } else {
        //debug_log_output("SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC
        , SKIN_KEYWORD_DEL_IF_CLIENT_IS_NOT_PC_E);
    }
    // ?focus が指定されているときとそうじゃないとき
    if ( skin_rep_data_global_p->focus[0] == '\0' ) {
        // focus が指定されていないときは 削除
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IF_FOCUS_IS_NOT_SPECIFIED
        , SKIN_KEYWORD_DEL_IF_FOCUS_IS_NOT_SPECIFIED_E);
    } else {
        // focus が指定されているときは 削除
        cut_enclose_words(menu_work_p, menu_work_buf_size
        , SKIN_KEYWORD_DEL_IF_FOCUS_IS_SPECIFIED
        , SKIN_KEYWORD_DEL_IF_FOCUS_IS_SPECIFIED_E);
    }
    // =============
    // = 置換実行
    // =============
    // SERVER_NAME
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_SERVER_NAME, SERVER_NAME);
    // CURRENT_DATE 現在の日付
    conv_time_to_date_string(current_date, time(NULL));
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_DATE, current_date);
    // CURRENT_TIME 現在の時刻
    conv_time_to_time_string(current_time, time(NULL));
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_TIME, current_time);
    // CURRENT_PATH 現在のパス（表示用）
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_PATH, skin_rep_data_global_p->current_path_name);
    // CURRENT_DIR_NAME 現在のディレクトリ名（表示用）
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_DIR_NAME, skin_rep_data_global_p->current_directory_name);
    // CURRENT_PATH_LINK	現在のパス（LINK用 URIエンコード）
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_PATH_LINK, skin_rep_data_global_p->current_directory_link);
    // CURRENT_PATH_LINK_NO_PARAM	現在のパス（LINK用 URIエンコード）
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_PATH_LINK_NO_PARAM, skin_rep_data_global_p->current_directory_link_no_param);
    // SKIN_KEYWORKD_PARLENT_DIR_LINK 	親ディレクトリ(LINK用 URIエンコード)
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORKD_PARLENT_DIR_LINK, skin_rep_data_global_p->parent_directory_link );
    // SKIN_KEYWORD_CURRENT_PAGE	 	現在のページ
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_CURRENT_PAGE, skin_rep_data_global_p->now_page_str);
    // SKIN_KEYWORD_MAX_PAGE			全ページ数
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_MAX_PAGE, skin_rep_data_global_p->max_page_str);
    // SKIN_KEYWORD_NEXT_PAGE			次のページ
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_NEXT_PAGE, skin_rep_data_global_p->next_page_str);
    // SKIN_KEYWORD_BACK_PAGE			前のページ
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_PREV_PAGE, skin_rep_data_global_p->prev_page_str);
    // SKIN_KEYWORD_FILE_NUM			 ディレクトリ存在ファイル数
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_FILE_NUM, skin_rep_data_global_p->file_num_str);
    // SKIN_KEYWORD_START_FILE_NUM		 表示開始ファイル番号
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_START_FILE_NUM, skin_rep_data_global_p->start_file_num_str);
    // SKIN_KEYWORD_END_FILE_NUM		 表示終了ファイル番号
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_END_FILE_NUM, skin_rep_data_global_p->end_file_num_str);
    // SKIN_KEYWORD_ONLOADSET_FOCUS		 BODYタグ用 onloadset="$focus"
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_ONLOADSET_FOCUS, skin_rep_data_global_p->focus);
    // SKIN_KEYWORD_SECRET_DIR_LINK		隠しディレクトリ
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_SECRET_DIR_LINK, skin_rep_data_global_p->secret_dir_link_html );
    return;
}
// **************************************************************************
// ライン用データ まとめて置換
// **************************************************************************
static void replace_skin_line_data(unsigned char *menu_work_p, int menu_work_buf_size, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p)
{
    unsigned char	row_string[16];	// 行番号
    // ===============================
    // = 削除キーワードを削除する
    // ===============================
    // MP3タグ無い場合
    if ( skin_rep_data_line_p->mp3_id3v1_flag == 0 )
    {
        // debug_log_output("SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS hit.");
        cut_enclose_words(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS, SKIN_KEYWORD_DEL_IS_NO_MP3_TAGS_E);
    }
    else // MP3タグ存在してる場合
    {
        cut_enclose_words(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_DEL_IS_HAVE_MP3_TAGS, SKIN_KEYWORD_DEL_IS_HAVE_MP3_TAGS_E);
    }
    // 行が奇数行、偶数行
    if ( skin_rep_data_line_p->row_num % 2 == 0 ) // 偶数行
    {
        cut_enclose_words(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_DEL_IF_LINE_IS_EVEN, SKIN_KEYWORD_DEL_IF_LINE_IS_EVEN_E);
    }
    else // 奇数行
    {
        cut_enclose_words(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_DEL_IF_LINE_IS_ODD, SKIN_KEYWORD_DEL_IF_LINE_IS_ODD_E);
    }
    // =============
    //  置換実行
    // =============
    // SKIN_KEYWORD_LINE_FILE_NAME	ファイル名 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_NAME, skin_rep_data_line_p->file_name );
    // SKIN_KEYWORD_LINE_FILE_NAME_NO_EXT	ファイル名(拡張子無し) 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_NAME_NO_EXT, skin_rep_data_line_p->file_name_no_ext );
    // SKIN_KEYWORD_LINE_FILE_EXT	ファイル拡張子名 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_EXT, skin_rep_data_line_p->file_extension );
    // SKIN_KEYWORD_LINE_FILE_LINK	ファイル名 LINK用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_LINK, skin_rep_data_line_p->file_uri_link );
    // SKIN_KEYWORD_LINE_TIMESTAMP	  日時 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_TIMESTAMP, skin_rep_data_line_p->file_timestamp );
    // SKIN_KEYWORD_LINE_FILE_DATE	  日付 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_DATE, skin_rep_data_line_p->file_timestamp_date );
    // SKIN_KEYWORD_LINE_FILE_TIME	  時刻 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_TIME, skin_rep_data_line_p->file_timestamp_time );
    // SKIN_KEYWORD_LINE_TVID		TVID 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_TVID, skin_rep_data_line_p->tvid_string );
    snprintf(row_string, sizeof(row_string), "%d", skin_rep_data_line_p->row_num );
    // SKIN_KEYWORD_LINE_COLUMN_NUM		行番号(backward compatibility..)
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_COLUMN_NUM, row_string );
    // SKIN_KEYWORD_LINE_ROW_NUM		行番号
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_ROW_NUM, row_string );
    // SKIN_KEYWORD_LINE_FILE_VOD	vod="0"  必要に応じて付く。
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_VOD, skin_rep_data_line_p->vod_string );
    // SKIN_KEYWORD_LINE_FILE_SIZE	ファイルサイズ 表示用
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_FILE_SIZE,  skin_rep_data_line_p->file_size_string );
    // SKIN_KEYWORD_LINE_IMAGE_WIDTH	画像の横幅
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_IMAGE_WIDTH,  skin_rep_data_line_p->image_width );
    // SKIN_KEYWORD_LINE_IMAGE_HEIGHT	画像の高さ
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_IMAGE_HEIGHT,  skin_rep_data_line_p->image_height );
    // SKIN_KEYWORD_LINE_MP3TAG_TITLE		MP3タグ タイトル
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_MP3TAG_TITLE,	skin_rep_data_line_p->mp3_id3v1_title	);
    // SKIN_KEYWORD_LINE_MP3TAG_ALBUM		MP3タグ アルバム名
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_MP3TAG_ALBUM,	skin_rep_data_line_p->mp3_id3v1_album	);
    // SKIN_KEYWORD_LINE_MP3TAG_ARTIST		MP3タグ アーティスト
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_MP3TAG_ARTIST,	skin_rep_data_line_p->mp3_id3v1_artist	);
    // SKIN_KEYWORD_LINE_MP3TAG_YEAR		MP3タグ 制作年度
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_MP3TAG_YEAR,	skin_rep_data_line_p->mp3_id3v1_year	);
    // SKIN_KEYWORD_LINE_MP3TAG_COMMENT		MP3タグ コメント
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_MP3TAG_COMMENT,skin_rep_data_line_p->mp3_id3v1_comment	);
    // SKIN_KEYWORD_LINE_MP3TAG_TITLE_INFO		MP3タグ タイトル情報(字数制限付き)
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_MP3TAG_TITLE_INFO,	skin_rep_data_line_p->mp3_id3v1_title_info_limited	);
    // SKIN_KEYWORD_LINE_AVI_DURATION		AVIのFPS
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_FPS, skin_rep_data_line_p->avi_fps	);
    // SKIN_KEYWORD_LINE_AVI_DURATION		AVIの再生時間
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_DURATION, skin_rep_data_line_p->avi_duration	);
    // SKIN_KEYWORD_LINE_AVI_VCODEC			AVIの動画コーデック
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_VCODEC, skin_rep_data_line_p->avi_vcodec	);
    // SKIN_KEYWORD_LINE_AVI_ACODEC			AVIの音声コーデック
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_ACODEC, skin_rep_data_line_p->avi_acodec	);
    // SKIN_KEYWORD_LINE_AVI_HVCODEC			AVIの動画コーデック
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_HVCODEC, skin_rep_data_line_p->avi_hvcodec	);
    // SKIN_KEYWORD_LINE_AVI_HACODEC			AVIの音声コーデック
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_HACODEC, skin_rep_data_line_p->avi_hacodec	);
    // SKIN_KEYWORD_LINE_AVI_IS_INTERLEAVED	AVIがインターリーブされているか
    replace_character(menu_work_p, menu_work_buf_size, SKIN_KEYWORD_LINE_AVI_IS_INTERLEAVED, skin_rep_data_line_p->avi_is_interleaved);
    return;
}
// **************************************************************************
// スキンファイルを読み込む。
//
//  ファイルサイズにあわせて、malloc()
//  文字コード変換。
// **************************************************************************
static unsigned char *skin_file_read(unsigned char *read_filename, int *malloc_size )
{
    int		fd;
    struct stat		file_stat;
    int				result;
    ssize_t			read_size;
    unsigned char 	*read_work_buf;
    unsigned char 	*read_buf;
    // ファイルサイズチェック
    result = stat(read_filename, &file_stat);
    debug_log_output("skin: head stat()=%d, st_size=%lld", result, file_stat.st_size );
    if ( result != 0 )
    {
        debug_log_output("stat(%s) error.", read_filename);
        return NULL;
    }
    // ファイルサイズの6倍の領域をmalloc(２つ)
    *malloc_size = file_stat.st_size * 6 ;
    read_work_buf = (unsigned char*)malloc( *malloc_size );
    if ( read_work_buf == NULL )
    {
        debug_log_output("maloc() error.");
        return NULL;
    }
    read_buf = (unsigned char*)malloc( *malloc_size );
    if ( read_buf == NULL )
    {
        debug_log_output("maloc() error.");
        return NULL;
    }
    memset(read_work_buf, '\0', *malloc_size);
    memset(read_buf, '\0', *malloc_size);
    // ファイル読み込み
    fd = open(read_filename, O_RDONLY );
    if ( fd < 0 )
    {
        debug_log_output("open() error.");
        free(read_buf);
        free(read_work_buf);
        return NULL;
    }
    read_size = read(fd, read_work_buf, file_stat.st_size );
    debug_log_output("skin: read() read_size=%d", read_size );
    if ( read_size != file_stat.st_size )
    {
        debug_log_output("read() error.");
        free(read_buf);
        free(read_work_buf);
        return NULL;
    }
    close( fd );
    // 読んだデータの文字コードを、MediaWiz用コードに変換
    convert_language_code(read_work_buf, read_buf, *malloc_size, CODE_AUTO, global_param.client_language_code);
    debug_log_output("skin: nkf end." );
    // ワークエリアをfree.
    free(read_work_buf);
    return read_buf;	// 正常終了
}
// **************************************************************************
// *path で指定されたディレクトリに存在するファイル数をカウントする
//
// return: ファイル数
// **************************************************************************
static int count_file_num(unsigned char *path)
{
    int		count;
    DIR	*dir;
    struct dirent	*dent;
    debug_log_output("count_file_num() start. path='%s'", path);
    dir = opendir((char*)path);
    if ( dir == NULL )	// エラーチェック
    {
        debug_log_output("opendir() error");
        return ( -1 );
    }
    count = 0;
    while ( 1 )
    {
        dent = readdir(dir);
        if ( dent == NULL  )
        break;
        // 無視ファイルチェック。
        if ( file_ignoral_check(dent->d_name, path) != 0 )
        continue;
        count++;
    }
    closedir(dir);
    debug_log_output("count_file_num() end. counter=%d", count);
    return count;
}
// **************************************************************************
// *path で指定されたTSVファイルに存在するファイル数をカウントする
//
// return: ファイル数
// **************************************************************************
static int count_file_num_in_tsv(unsigned char *path)
{
    int		count;
    int		fd;
    unsigned char	buf[1024];
    debug_log_output("count_file_num_in_tsv() start. path='%s'", path);
    fd = open(path, O_RDONLY);
    if ( fd < 0 )
    {
        debug_log_output("'%s' can't open.", path);
        return ( -1 );
    }
    count = 0;
    while ( 1 )
    {
        char *p;
        int ret;
        // ファイルから、１行読む
        ret = file_read_line( fd, buf, sizeof(buf) );
        if ( ret < 0 )
        {
            debug_log_output("tsv EOF Detect.");
            break;
        }
        p = strchr(buf, '\t');
        if (p == NULL) continue;
        p = strchr(p+1, '\t');
        if (p == NULL) continue;
        count++;
    }
    close(fd);
    debug_log_output("count_file_num_in_tsv() end. counter=%d", count);
    return count;
}
// **************************************************************************
// ディレクトリに存在する情報を、ファイルの数だけ読み込む。
//
// return: 読み込んだファイル情報数
// **************************************************************************
static int directory_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num)
{
    int	count;
    DIR	*dir;
    struct dirent	*dent;
    struct stat		file_stat;
    int				result;
    unsigned char	fullpath_filename[FILENAME_MAX];
    unsigned char	file_extension[8];
    debug_log_output("directory_stat() start. path='%s'", path);
    dir = opendir((char*)path);
    if ( dir == NULL )	// エラーチェック
    {
        debug_log_output("opendir() error");
        return ( -1 );
    }
    count = 0;
    while ( 1 )
    {
        if ( count >= file_num )
        break;
        // ディレクトリから、ファイル名を１個GET
        dent = readdir(dir);
        if ( dent == NULL  )
        break;
        // 無視ファイルチェック。
        if ( file_ignoral_check(dent->d_name, path) != 0 )
        continue;
        //debug_log_output("dent->d_name='%s'", dent->d_name);
        // フルパスファイル名生成
        strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
        strncat(fullpath_filename, dent->d_name, sizeof(fullpath_filename) - strlen(fullpath_filename) );
        //debug_log_output("fullpath_filename='%s'", fullpath_filename );
        // stat() 実行
        memset(&file_stat, 0, sizeof(file_stat));
        result = stat(fullpath_filename, &file_stat);
        if ( result < 0 ){
            continue;
	}
        // ファイル名を保存
        if (S_ISDIR( file_stat.st_mode )) {
            snprintf(file_info_p[count].org_name, sizeof(file_info_p[count].org_name), "%s/", dent->d_name);
        } else {
            strncpy(file_info_p[count].org_name, dent->d_name, sizeof(file_info_p[count].org_name) );
        }
        strncpy(file_info_p[count].name, dent->d_name, sizeof(file_info_p[count].name) );
        // 表示用ファイル名を、CAP/HEX 文字列変換
        if ( global_param.flag_decode_samba_hex_and_cap == TRUE )
        {
            debug_log_output("name(cap/hex) = '%s'", file_info_p[count].name);
            decode_samba_hex_and_cap_coding( file_info_p[count].name );
            debug_log_output("name(bin) = '%s'", file_info_p[count].name);
        }
        // その他情報を保存
        file_info_p[count].type = file_stat.st_mode;
        file_info_p[count].size = file_stat.st_size;
        file_info_p[count].time = file_stat.st_mtime;
        // vob先頭ファイルチェック v0.12f3
        if ( (global_param.flag_show_first_vob_only == TRUE)
        &&( strcasecmp(file_extension, "vob") == 0 ) ) {
            if (fullpath_filename[strlen(fullpath_filename)-5] == '1') {
                JOINT_FILE_INFO_T joint_file_info;
                if (analyze_vob_file(fullpath_filename, &joint_file_info ) == 0) {
                    file_info_p[count].size = joint_file_info.total_size;
                }
            } else
            continue;
        }
        count++;
    }
    closedir(dir);
    debug_log_output("directory_stat() end. count=%d", count);
    return count;
}
// **************************************************************************
// TSVファイルに存在する情報を、エントリの数だけ読み込む。
//
// return: 読み込んだファイル情報数
// **************************************************************************
static int tsv_stat(unsigned char *path, FILE_INFO_T *file_info_p, int file_num)
{
    int	count;
    struct stat		file_stat;
    int				result;
    unsigned char	fullpath_filename[FILENAME_MAX];
    unsigned char			buf[1024];
    int				fd;
    debug_log_output("tsv_stat() start. path='%s'", path);
    fd = open(path, O_RDONLY);
    if ( fd < 0 )
    {
        debug_log_output("'%s' can't open.", path);
        return ( -1 );
    }
    cut_after_last_character(path, '/');
    strcat(path, "/"); // XXX: there's no size check..
    count = 0;
    while ( count < file_num )
    {
        unsigned char *p;
        unsigned char *fname, *tvid, *title;
        int ret;
        // ファイルから、１行読む
        ret = file_read_line( fd, buf, sizeof(buf) );
        if ( ret < 0 )
        {
            debug_log_output("tsv EOF Detect.");
            break;
        }
        // 行末のスペースを削除。
        cut_character_at_linetail(buf, ' ');
        // 空行なら、continue
        if ( strlen( buf ) == 0 )
        {
            debug_log_output("continue.");
            continue;
        }
        p = buf;
        fname = p;
        p = (unsigned char*)strchr(p, '\t');
        if (p == NULL) {
            debug_log_output("\\t notfound");
            continue;
        }
        *p++ = '\0';
        tvid = p;
        p = (unsigned char*)strchr(p, '\t');
        if (p == NULL) {
            debug_log_output("\\t notfound 2");
            continue;
        }
        *p++ = '\0';
        title = p;
        // フルパスファイル名生成
        if (fname[0] == '/') {
            strncpy(fullpath_filename, global_param.document_root, sizeof(fullpath_filename) );
        } else {
            strncpy(fullpath_filename, path, sizeof(fullpath_filename) );
        }
        strncat(fullpath_filename, fname, sizeof(fullpath_filename) - strlen(fullpath_filename) );
        // ファイル名を保存
        strncpy(file_info_p[count].org_name, fname, sizeof(file_info_p[count].org_name) );
        strncpy(file_info_p[count].name, title, sizeof(file_info_p[count].name) );
        debug_log_output("fullpath_filename='%s'", fullpath_filename );
        // stat() 実行
        memset(&file_stat, 0, sizeof(file_stat));
        result = stat(fullpath_filename, &file_stat);
        if ( result >= 0 ) {
            // 実体発見
            file_info_p[count].type = file_stat.st_mode;
            file_info_p[count].size = file_stat.st_size;
            file_info_p[count].time = file_stat.st_mtime;
        } else {
            // 実体はない。よって、日付とサイズとファイルの種類不明
            file_info_p[count].type = 0;
            file_info_p[count].size = 0;
            file_info_p[count].time = 0;
        }
        count++;
    }
    close(fd);
    debug_log_output("tsv_stat() end. count=%d", count);
    return count;
}
// ******************************************************************
// ファイル無視チェック
// ディレクトリ内で、ファイルを、無視するかしないかを判断する。
// return: 0:OK  -1 無視
// ******************************************************************
static int file_ignoral_check( char *name, unsigned char *path )
{
    int				i;
    unsigned char	file_extension[16];
    char			flag;
    unsigned char	work_filename[FILENAME_MAX];
    struct stat		file_stat;
    int				result;
    const char *ignore_names[] = {
        ".",
        "..",
        "lost+found",
        "RECYCLER",
        "System Volume Information",
        "cgi-bin",
    };
    int ignore_count = sizeof(ignore_names) / sizeof(char*);
    // ==================================================================
    // 上記 ignore_names に該当するものをスキップ
    // ==================================================================
    for (i=0; i<ignore_count; i++) {
        if (!strcmp(name, ignore_names[i])) return -1;
    }
    // ==================================================================
    // MacOSX用 "._" で始まるファイルをスキップ（リソースファイル）
    // ==================================================================
    if ( strncmp(name, "._", 2 ) == 0 )
    {
        return ( -1 );
    }
    // ==================================================================
    // wizdが知らないファイル隠しフラグがたっていたら、拡張子チェック
    // ==================================================================
    if ( global_param.flag_unknown_extention_file_hide == TRUE )
    {
        filename_to_extension( (unsigned char*)name, file_extension, sizeof(file_extension) );
        flag = 0;
        if ( strlen(file_extension) > 0 ) // 拡張子無しは知らないと同義
        {
            for ( i=0; mime_list[i].file_extension != NULL; i++)
            {
                if ( strcasecmp(mime_list[i].file_extension, file_extension ) == 0 )
                {
                    //debug_log_output("%s Known!!!", file_extension );
                    flag = 1; // 知ってた
                    break;
                }
            }
        }
        if ( flag == 0 ) // 知らなかった。
        {
            // -----------------------------------------------
            // ファイルが、ホントにファイルかチェック。
            // もしもディレクトリなら、returnしない。
            // -----------------------------------------------
            // フルパス生成
            strncpy(work_filename, path, sizeof(work_filename) );
            if ( work_filename[strlen(work_filename)-1] != '/' )
            {
                strncat(work_filename, "/", sizeof(work_filename) - strlen(work_filename) );
            }
            strncat(work_filename, name, sizeof(work_filename) - strlen(work_filename) );
            debug_log_output("'%s' Unknown. directory check start.", work_filename );
            // stat() 実行
            result = stat(work_filename, &file_stat);
            if ( result != 0 )
            return ( -1 );
            if ( S_ISDIR(file_stat.st_mode) == 0 ) // ディレクトリじゃない。知らない拡張子ファイルだと確定。
            {
                debug_log_output("'%s' Unknown!!!", name );
                return ( -1 );
            }
            debug_log_output("'%s' is a directory!!!", name );
        }
    }
    // ==================================================================
    // 隠しディレクトリチェック
    // ==================================================================
    for ( i=0; i<SECRET_DIRECTORY_MAX; i++ )
    {
        if ( strcmp(name, secret_directory_list[i].dir_name ) == 0 )
        {
            debug_log_output("secret_directory_list[%d].'%s' HIT!!", i, secret_directory_list[i].dir_name);
            return ( -1 );
        }
    }
    return ( 0 );
}
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
// **************************************************************************
// * allplay 用のプレイリストを生成
// **************************************************************************
static void create_all_play_list(HTTP_RECV_INFO *http_recv_info_p, FILE_INFO_T *file_info_p, int file_num, unsigned char *send_filemenu_buf, int buf_size)
{
    int		i,j;
    int		stream_type;
    unsigned char	file_extension[32];
    unsigned char	file_name[255];
    unsigned char	file_uri_link[FILENAME_MAX];
    unsigned char	work_data[FILENAME_MAX * 2];
    SKIN_REPLASE_LINE_DATA_T	mp3_id3tag_data;	// MP3 ID3タグ読み込み用
    debug_log_output("create_all_play_list() start.");
    memset(send_filemenu_buf, '\0', buf_size);
    // =================================
    // ファイル表示用情報 生成開始
    // =================================
    for ( i=0; i<file_num ; i++ )
    {
        // 拡張子だけ生成
        filename_to_extension(file_info_p[i].org_name, file_extension, sizeof(file_extension));
        debug_log_output("file_extension='%s'\n", file_extension);
        // ---------------------------------------------------------
        // ファイルタイプ判定。STREAMファイル以外ならcontinue.
        // ---------------------------------------------------------
        // ディレクトリか？
        if ( S_ISDIR( file_info_p[i].type ) != 0 )
        {
            continue;
        }
        // 拡張子からストリームタイプを判断。
        stream_type = TYPE_NO_STREAM;
        for (j=0;;j++)
        {
            if (( mime_list[j].mime_name == NULL ) || ( strlen(file_extension) == 0 ) )
            break;
            if ( strcasecmp(mime_list[j].file_extension, file_extension) == 0 )
            {
                stream_type = mime_list[j].stream_type;
                break;
            }
        }
        // ------------------------------------------------------
        // ストリームファイル以外ならcontinue
        // ------------------------------------------------------
        if ( stream_type != TYPE_STREAM )
        {
            continue;
        }
        if ( mime_list[j].menu_file_type != TYPE_MOVIE &&
        mime_list[j].menu_file_type != TYPE_MUSIC )
        {
            continue;
        }
        // -----------------------------------------
        // 拡張子がmp3なら、ID3タグチェック。
        // -----------------------------------------
        mp3_id3tag_data.mp3_id3v1_flag = 0;
        if ( strcasecmp(file_extension, "mp3" ) == 0 )
        {
            // MP3ファイルのフルパス生成
            strncpy( work_data, http_recv_info_p->send_filename, sizeof(work_data));
            if ( work_data[strlen(work_data)-1] != '/' )
            {
                strncat(work_data, "/", sizeof(work_data) - strlen(work_data) );
            }
            strncat(work_data, file_info_p[i].org_name, sizeof(work_data) - strlen(work_data));
            debug_log_output("work_data(mp3) = %s", work_data);
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
            playlist_filename_adjustment(file_info_p[i].name, file_name, sizeof(file_name), global_param.server_language_code);
        }
        // ------------------------------------
        // Link用URI(エンコード済み) を生成
        // ------------------------------------
        if (file_info_p[i].org_name[0] == '/') {
            strncpy(work_data, file_info_p[i].org_name, sizeof(work_data));
        } else {
            strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) );
            cut_after_last_character( work_data, '/' );
            if ( work_data[strlen(work_data)-1] != '/' )
            {
                strncat( work_data, "/", sizeof(work_data) );
            }
            strncat(work_data, file_info_p[i].org_name, sizeof(work_data)- strlen(work_data) );
        }
        uri_encode(file_uri_link, sizeof(file_uri_link), work_data, strlen(work_data) );
        debug_log_output("file_uri_link='%s'\n", file_uri_link);
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
        strncat(send_filemenu_buf, work_data, buf_size - strlen(send_filemenu_buf) );
    }
    return;
}
// *************************************************************************
//  表示ファイルソート
// *************************************************************************
static void file_info_sort( FILE_INFO_T *p, int num, unsigned long type )
{
    int nDir, nFile, i, row;
    // ディレクトリ数とファイル数の分離
    for ( nDir = 0, i = 0; i < num; ++i )
    {
        if ( S_ISDIR( p[ i ].type ) )
        {
            ++nDir;
        }
    }
    nFile = num - nDir;
    // ソート処理 ******************************************************************
    // 準備
    row = 0;
    // ディレクトリソートがあれば行う
    if ( SORT_DIR_FLAG( type ) && nDir > 0 )
    {
        // とりあえず全部ソート
        qsort( p, num, sizeof( FILE_INFO_T ), (int (*)(const void*, const void*))dir_sort_api[ SORT_DIR_FLAG( type ) ] );
        // ディレクトリ名のソート
        qsort( &p[ ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? num - nDir : 0 ], nDir, sizeof( FILE_INFO_T ), (int (*)(const void*, const void*))file_sort_api[ SORT_NAME_UP ] );
        // ファイル位置確定
        row = ( SORT_DIR_FLAG( type ) == SORT_DIR_DOWN ) ? 0 : nDir;
    }
    else
    {
        // ファイルソート対象を全件にする
        nFile = num;
    }
    // ファイルソートを行う
    // ディレクトリが対象になっていなければ、全件対象にする
    if ( ( type & SORT_FILE_MASK ) && nFile > 0 )
    {
        qsort( &p[ row ], nFile, sizeof( FILE_INFO_T ), (int (*)(const void*, const void*))file_sort_api[ ( type & SORT_FILE_MASK ) ] );
    }
    return;
}
// *************************************************************************
// ディレクトリのソート
// *************************************************************************
static int _file_info_dir_sort( const void *in_a, const void *in_b, int order )
{
    FILE_INFO_T *a, *b;
    int n1, n2;
    a = (FILE_INFO_T *) in_a;
    b = (FILE_INFO_T *) in_b;
    n1 = ( S_ISDIR( a->type ) ? 1 : 0 );
    n2 = ( S_ISDIR( b->type ) ? 1 : 0 );
    return ( n1 == n2 ? 0 : ( order ? n1 - n2 : n2 - n1 ) );
}
static int _file_info_dir_sort_order_up( const void *in_a, const void *in_b )
{
    return _file_info_dir_sort( in_a, in_b, 0 );
}
static int _file_info_dir_sort_order_down( const void *in_a, const void *in_b )
{
    return _file_info_dir_sort( in_a, in_b, 1 );
}
// *************************************************************************
// 名前のソート
// *************************************************************************
static int _file_info_name_sort( const void *in_a, const void *in_b, int order )
{
    FILE_INFO_T *a, *b;
    a = (FILE_INFO_T *) in_a;
    b = (FILE_INFO_T *) in_b;
    return ( order ? strcmp( b->name, a->name ) : strcmp( a->name, b->name ) );
}
static int _file_info_name_sort_order_up( const void *in_a, const void *in_b )
{
    return _file_info_name_sort( in_a, in_b, 0 );
}
static int _file_info_name_sort_order_down( const void *in_a, const void *in_b )
{
    return _file_info_name_sort( in_a, in_b, 1 );
}
// *************************************************************************
// サイズのソート
// *************************************************************************
static int _file_info_size_sort( const void *in_a, const void *in_b, int order )
{
    FILE_INFO_T *a, *b;
    a = (FILE_INFO_T *) in_a;
    b = (FILE_INFO_T *) in_b;
    return (int)( order ? b->size - a->size : a->size - b->size );
}
static int _file_info_size_sort_order_up( const void *in_a, const void *in_b )
{
    return _file_info_size_sort( in_a, in_b, 0 );
}
static int _file_info_size_sort_order_down( const void *in_a, const void *in_b )
{
    return _file_info_size_sort( in_a, in_b, 1 );
}
// *************************************************************************
// 時間のソート
// *************************************************************************
static int _file_info_time_sort( const void *in_a, const void *in_b, int order )
{
    FILE_INFO_T *a, *b;
    a = (FILE_INFO_T *) in_a;
    b = (FILE_INFO_T *) in_b;
    return (int)( order ? b->time - a->time : a->time - b->time );
}
static int _file_info_time_sort_order_up( const void *in_a, const void *in_b )
{
    return _file_info_time_sort( in_a, in_b, 0 );
}
static int _file_info_time_sort_order_down( const void *in_a, const void *in_b )
{
    return _file_info_time_sort( in_a, in_b, 1 );
}
// *************************************************************************
// シャッフル
// *************************************************************************
static int _file_info_shuffle( const void *in_a, const void *in_b )
{
    FILE_INFO_T *a, *b;
    a = (FILE_INFO_T *) in_a;
    b = (FILE_INFO_T *) in_b;
    return (rand() & 0x800) ? 1 : -1;
}
#define  SKIN_IMAGE_VIEWER_HTML  "image_viewer.html" // ImageViewerのスキン
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
// * イメージビューアーを生成して返信
// **************************************************************************
void http_image_viewer(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    unsigned char	*image_viewer_skin_p;
    int				image_viewer_skin_malloc_size;
    unsigned int 	image_width, image_height;
    unsigned int 	image_viewer_width, image_viewer_height;
    unsigned char	file_extension[32];
    unsigned char	skin_filename[FILENAME_MAX];
    unsigned char	work_data[FILENAME_MAX];
    struct	stat	image_stat;
    int		result;
    int		now_page;
    int		flag_fit_mode = 0;
    SKIN_REPLASE_IMAGE_VIEWER_DATA_T	image_viewer_info;
    // ========================
    // 置換用データ生成
    // ========================
    // 一つ上のディレクトリパス(親パス)を生成。(URIエンコード)
    strncpy(work_data, http_recv_info_p->recv_uri, sizeof(work_data) - strlen(work_data) );
    cut_after_last_character(work_data, '/');
    strncat(work_data, "/", sizeof(work_data) - strlen(work_data) ); // 最後に'/'を追加。
    debug_log_output("parent_directory='%s'", work_data);
    uri_encode(image_viewer_info.parent_directory_link, sizeof(image_viewer_info.parent_directory_link), work_data, strlen(work_data));
    strncat(image_viewer_info.parent_directory_link, "?", sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
    // sort=が指示されていた場合、それを引き継ぐ。
    if ( strlen(http_recv_info_p->sort) > 0 )
    {
        snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
        strncat(image_viewer_info.parent_directory_link, work_data, sizeof(image_viewer_info.parent_directory_link) - strlen(image_viewer_info.parent_directory_link));
    }
    debug_log_output("parent_directory_link='%s'", image_viewer_info.parent_directory_link);
    // 現パス名 表示用 生成(文字コード変換)
    convert_language_code(	http_recv_info_p->recv_uri,
    image_viewer_info.current_uri_name,
    sizeof(image_viewer_info.current_uri_name),
    global_param.server_language_code,
    global_param.client_language_code);
    debug_log_output("image_viewer: current_uri = '%s'", image_viewer_info.current_uri_name );
    // 現パス名 Link用生成（URIエンコード）
    uri_encode(image_viewer_info.current_uri_link, sizeof(image_viewer_info.current_uri_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
    strncat(image_viewer_info.current_uri_link, "?", sizeof(image_viewer_info.current_uri_link) - strlen(image_viewer_info.current_uri_link)); // 最後に'?'を追加
    // sort=が指示されていた場合、それを引き継ぐ。
    if ( strlen(http_recv_info_p->sort) > 0 )
    {
        snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
        strncat(image_viewer_info.current_uri_link, work_data, sizeof(image_viewer_info.current_uri_link) - strlen(image_viewer_info.current_uri_link));
    }
    debug_log_output("image_viewer: current_uri_link='%s'", image_viewer_info.current_uri_link);
    if ( http_recv_info_p->page <= 1 )
    now_page = 1;
    else
    now_page = http_recv_info_p->page;
    // 	現在のページ 表示用
    snprintf(image_viewer_info.now_page_str, sizeof(image_viewer_info.now_page_str), "%d", now_page );
    // ファイルサイズ, タイムスタンプGET
    result = stat(http_recv_info_p->send_filename, &image_stat);
    if ( result != 0 )
    {
        debug_log_output("stat(%s) error.", http_recv_info_p->send_filename);
        return;
    }
    conv_num_to_unit_string(image_viewer_info.file_size_string, image_stat.st_size );
    conv_time_to_string(image_viewer_info.file_timestamp, image_stat.st_mtime );
    conv_time_to_date_string(image_viewer_info.file_timestamp_date, image_stat.st_mtime );
    conv_time_to_time_string(image_viewer_info.file_timestamp_time, image_stat.st_mtime );
    // 画像サイズGET
    image_width = 0;
    image_height = 0;
    // 拡張子取り出し
    filename_to_extension(http_recv_info_p->send_filename, file_extension, sizeof(file_extension) );
    // 拡張子で分岐
    if ( (strcasecmp( file_extension, "jpg" ) == 0 ) ||
    (strcasecmp( file_extension, "jpeg" ) == 0 ))
    {
        // JPEGファイルのサイズをGET
        jpeg_size( http_recv_info_p->send_filename, &image_width, &image_height );
    }
    else if (strcasecmp( file_extension, "gif" ) == 0 )
    {
        // GIFファイルのサイズをGET
        gif_size( http_recv_info_p->send_filename, &image_width, &image_height );
    }
    else if (strcasecmp( file_extension, "png" ) == 0 )
    {
        // PNGファイルのサイズをGET
        png_size( http_recv_info_p->send_filename, &image_width, &image_height );
    }
    debug_log_output("image_width=%d, image_height=%d", image_width, image_height);
    snprintf(image_viewer_info.image_width, sizeof(image_viewer_info.image_width), "%d", image_width);
    snprintf(image_viewer_info.image_height, sizeof(image_viewer_info.image_height), "%d", image_height);
    // 表示サイズ（仮）
    if ( strcasecmp(http_recv_info_p->option, "-2" ) == 0 )  // 0.5x
    {
        image_viewer_width = image_width / 2;
        image_viewer_height = image_height / 2;
        strncpy(image_viewer_info.image_viewer_mode, "0.5x", sizeof(image_viewer_info.image_viewer_mode) );
    }
    else if ( strcasecmp(http_recv_info_p->option, "2x" ) == 0 )  // 2x
    {
        image_viewer_width = image_width * 2;
        image_viewer_height = image_height * 2 ;
        strncpy(image_viewer_info.image_viewer_mode, "2x", sizeof(image_viewer_info.image_viewer_mode) );
    }
    else if ( strcasecmp(http_recv_info_p->option, "4x" ) == 0 )  // 4x
    {
        image_viewer_width = image_width * 4;
        image_viewer_height = image_height * 4 ;
        strncpy(image_viewer_info.image_viewer_mode, "4x", sizeof(image_viewer_info.image_viewer_mode) );
    }
    else if ( strcasecmp(http_recv_info_p->option, "fit" ) == 0 )  // FIT
    {
        // 縦に合わせてリサイズしてみる
        image_viewer_width = (image_width  * FIT_TERGET_HEIGHT) / image_height;
        image_viewer_height = FIT_TERGET_HEIGHT;
        if ( image_viewer_width > FIT_TERGET_WIDTH ) // 横幅超えていたら
        {
            // 横に合わせてリサイズする。
            image_viewer_width = FIT_TERGET_WIDTH;
            image_viewer_height = image_height * FIT_TERGET_WIDTH / image_width;
        }
        debug_log_output("fit:  (%d,%d) -> (%d,%d)", image_width, image_height, image_viewer_width, image_viewer_height);
        strncpy(image_viewer_info.image_viewer_mode, "FIT", sizeof(image_viewer_info.image_viewer_mode) );
        flag_fit_mode = 1;
    }
    else	// 1x
    {
        image_viewer_width = image_width;
        image_viewer_height = image_height;
        strncpy(image_viewer_info.image_viewer_mode, "1x", sizeof(image_viewer_info.image_viewer_mode) );
    }
    snprintf(image_viewer_info.image_viewer_width, sizeof(image_viewer_info.image_viewer_width), "%d", image_viewer_width );
    snprintf(image_viewer_info.image_viewer_height, sizeof(image_viewer_info.image_viewer_height), "%d", image_viewer_height );
    // ==============================
    // ImageViewer スキン読み込み
    // ==============================
    // ----------------------------------------------
    // スキンのフルパス生成
    // スキンのあるファイルパス生成
    // ----------------------------------------------
    strncpy(skin_filename, global_param.skin_root, sizeof(skin_filename) ); // スキン置き場
    if ( skin_filename[ strlen(skin_filename)-1 ] != '/' )// 最後が'/'じゃなかったら、'/'を追加
    {
        strncat(skin_filename, "/", sizeof(skin_filename) -strlen(skin_filename) );
    }
    strncat(skin_filename, global_param.skin_name, sizeof(skin_filename) - strlen(skin_filename) ); // スキン名（ディレクトリ）
    if ( skin_filename[ strlen(skin_filename)-1 ] != '/' ) // 最後が'/'じゃなかったら、'/'を追加
    {
        strncat(skin_filename, "/", sizeof(skin_filename) - strlen(skin_filename) );
    }
    // ImageView スキンファイル名生成
    strncat(skin_filename, SKIN_IMAGE_VIEWER_HTML, sizeof(skin_filename) - strlen(skin_filename) );
    debug_log_output("skin: skin_filename='%s'", skin_filename);
    // スキンデータ読み込み
    image_viewer_skin_p = skin_file_read( skin_filename, &image_viewer_skin_malloc_size);
    // ==============================
    // 置換実行
    // ==============================
    // SERVER_NAME
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_SERVER_NAME, SERVER_NAME);
    // CURRENT_PATH 現在のパス（表示用）
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_CURRENT_PATH, image_viewer_info.current_uri_name);
    // CURRENT_PATH_LINK	現在のパス（LINK用 URIエンコード）
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_CURRENT_PATH_LINK, image_viewer_info.current_uri_link);
    // SKIN_KEYWORKD_PARLENT_DIR_LINK 	親ディレクトリ(LINK用 URIエンコード)
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORKD_PARLENT_DIR_LINK, image_viewer_info.parent_directory_link );
    // SKIN_KEYWORD_CURRENT_PAGE	 	現在のページ
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_CURRENT_PAGE, image_viewer_info.now_page_str);
    // SKIN_KEYWORD_LINE_TIMESTAMP	  日時 表示用
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_LINE_TIMESTAMP, image_viewer_info.file_timestamp );
    // SKIN_KEYWORD_LINE_FILE_DATE	  日付 表示用
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_LINE_FILE_DATE, image_viewer_info.file_timestamp_date );
    // SKIN_KEYWORD_LINE_FILE_TIME	  時刻 表示用
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_LINE_FILE_TIME, image_viewer_info.file_timestamp_time );
    // SKIN_KEYWORD_LINE_FILE_SIZE	ファイルサイズ 表示用
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_LINE_FILE_SIZE,  image_viewer_info.file_size_string );
    // SKIN_KEYWORD_LINE_IMAGE_WIDTH	// 画像の横幅
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_LINE_IMAGE_WIDTH,  image_viewer_info.image_width );
    // SKIN_KEYWORD_LINE_IMAGE_HEIGHT	// 画像の高さ
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_LINE_IMAGE_HEIGHT,  image_viewer_info.image_height );
    // SKIN_KEYWORD_IMAGE_VIEWER_WIDTH		// 画像の表示横幅
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_IMAGE_VIEWER_WIDTH,  image_viewer_info.image_viewer_width );
    // SKIN_KEYWORD_IMAGE_VIEWER_HEIGHT		// 画像の表示高さ
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_IMAGE_VIEWER_HEIGHT,  image_viewer_info.image_viewer_height );
    // SKIN_KEYWORD_IMAGE_VIEWER_MODE		// 画像表示モード
    replace_character(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_IMAGE_VIEWER_MODE,  image_viewer_info.image_viewer_mode );
    // FITモードチェック
    if ( flag_fit_mode == 0 ) // FITモードではない
    {
        cut_enclose_words(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_DEL_IS_NO_FIT_MODE, SKIN_KEYWORD_DEL_IS_NO_FIT_MODE_E);
    }
    else // FITモード
    {
        cut_enclose_words(image_viewer_skin_p, image_viewer_skin_malloc_size, SKIN_KEYWORD_DEL_IS_FIT_MODE, SKIN_KEYWORD_DEL_IS_FIT_MODE_E);
    }
    debug_log_output("response: %s", image_viewer_skin_p);
    // =================
    // 返信実行
    // =================
    http_filemenu_send( accept_socket, image_viewer_skin_p );
    free(image_viewer_skin_p);
    return;
}
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
    cut_after_last_character( listfile_path, '/' );
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
        cut_character_at_linetail(buf, ' ');
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
#define  SKIN_OPTION_MENU_HTML  "option_menu.html" // OptionMenuのスキン
#define  SKIN_KEYWORD_CURRENT_PATH_LINK_NO_SORT "<!--WIZD_INSERT_CURRENT_PATH_LINK_NO_SORT-->" // 現PATH(Sort情報引き継ぎ無し)。LINK用。URIエンコード済み
// **************************************************************************
// * オプションメニューを生成して返信
// **************************************************************************
void http_option_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    unsigned char	*option_menu_skin_p;
    int				option_menu_skin_malloc_size;
    unsigned char	current_uri_link_no_sort[FILENAME_MAX];
    unsigned char	current_uri_link[FILENAME_MAX];
    unsigned char	work_data[FILENAME_MAX];
    int				now_page;
    unsigned char	now_page_str[16];
    unsigned char	skin_filename[FILENAME_MAX];
    // ========================
    // 置換用データ生成
    // ========================
    // --------------------------------------------------------------
    // 現パス名 Link用(Sort情報引き継ぎ無し)生成（URIエンコード）
    // --------------------------------------------------------------
    uri_encode(current_uri_link_no_sort, sizeof(current_uri_link_no_sort), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
    strncat(current_uri_link_no_sort, "?", sizeof(current_uri_link_no_sort) - strlen(current_uri_link_no_sort)); // '?'を追加
    debug_log_output("OptionMenu: current_uri_link_no_sort='%s'", current_uri_link_no_sort);
    // -----------------------------------------
    // 現パス名 Link用(Sort情報付き) 生成
    // -----------------------------------------
    uri_encode(current_uri_link, sizeof(current_uri_link), http_recv_info_p->recv_uri, strlen(http_recv_info_p->recv_uri));
    strncat(current_uri_link, "?", sizeof(current_uri_link) - strlen(current_uri_link)); // '?'を追加
    // sort=が指示されていた場合、それを引き継ぐ。
    if ( strlen(http_recv_info_p->sort) > 0 )
    {
        snprintf(work_data, sizeof(work_data), "sort=%s&", http_recv_info_p->sort);
        strncat(current_uri_link, work_data, sizeof(current_uri_link) - strlen(current_uri_link));
    }
    debug_log_output("OptionMenu: current_uri_link='%s'", current_uri_link);
    // ---------------------
    // 現ページ数 生成
    // ---------------------
    if ( http_recv_info_p->page <= 1 )
    now_page = 1;
    else
    now_page = http_recv_info_p->page;
    // 	現在のページ 表示用
    snprintf(now_page_str, sizeof(now_page_str), "%d", now_page );
    // ==============================
    // OptionMenu スキン読み込み
    // ==============================
    // ----------------------------------------------
    // スキンのフルパス生成
    // スキンのあるファイルパス生成
    // ----------------------------------------------
    strncpy(skin_filename, global_param.skin_root, sizeof(skin_filename) ); // スキン置き場
    if ( skin_filename[ strlen(skin_filename)-1 ] != '/' )// 最後が'/'じゃなかったら、'/'を追加
    {
        strncat(skin_filename, "/", sizeof(skin_filename) -strlen(skin_filename) );
    }
    strncat(skin_filename, global_param.skin_name, sizeof(skin_filename) - strlen(skin_filename) ); // スキン名（ディレクトリ）
    if ( skin_filename[ strlen(skin_filename)-1 ] != '/' ) // 最後が'/'じゃなかったら、'/'を追加
    {
        strncat(skin_filename, "/", sizeof(skin_filename) - strlen(skin_filename) );
    }
    // ImageView スキンファイル名生成
    strncat(skin_filename, SKIN_OPTION_MENU_HTML, sizeof(skin_filename) - strlen(skin_filename) );
    debug_log_output("skin: skin_filename='%s'", skin_filename);
    // スキンデータ読み込み
    option_menu_skin_p = skin_file_read( skin_filename, &option_menu_skin_malloc_size);
    // ==============================
    // 置換実行
    // ==============================
    // SERVER_NAME
    replace_character(option_menu_skin_p, option_menu_skin_malloc_size, SKIN_KEYWORD_SERVER_NAME, SERVER_NAME);
    //
    // CURRENT_PATH_LINK	現在のパス（LINK用 URIエンコード）
    replace_character(option_menu_skin_p, option_menu_skin_malloc_size, SKIN_KEYWORD_CURRENT_PATH_LINK, current_uri_link);
    // CURRENT_PATH_LINK	現在のパス（LINK用 URIエンコード）
    replace_character(option_menu_skin_p, option_menu_skin_malloc_size, SKIN_KEYWORD_CURRENT_PATH_LINK_NO_SORT, current_uri_link_no_sort);
    // SKIN_KEYWORD_CURRENT_PAGE	 	現在のページ
    replace_character(option_menu_skin_p, option_menu_skin_malloc_size, SKIN_KEYWORD_CURRENT_PAGE, now_page_str);
    // =================
    // 返信実行
    // =================
    http_filemenu_send( accept_socket, option_menu_skin_p );
    free(option_menu_skin_p);
    return;
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
    debug_log_output( "[%s][%s]",out,nkf_option);
    nkf((char*)in, (char*)out, len, (char*)nkf_option);
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
    off_t		length;
    memset(buf, 			'\0', sizeof(buf));
    fd = open(mp3_filename,  O_RDONLY);
    if ( fd < 0 )
    {
        return;
    }
    // 最後から128byteへSEEK
    length = lseek(fd, -128, SEEK_END);
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
    cut_character_at_linetail(buf, 0xFF);
    cut_character_at_linetail(buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_title,
    sizeof(skin_rep_data_line_p->mp3_id3v1_title),
    CODE_AUTO, global_param.client_language_code);
    // アーティスト
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    cut_character_at_linetail(buf, 0xFF);
    cut_character_at_linetail(buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_artist,
    sizeof(skin_rep_data_line_p->mp3_id3v1_artist),
    CODE_AUTO, global_param.client_language_code);
    // アルバム名
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 30);
    cut_character_at_linetail(buf, 0xFF);
    cut_character_at_linetail(buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_album,
    sizeof(skin_rep_data_line_p->mp3_id3v1_album),
    CODE_AUTO, global_param.client_language_code);
    // 制作年度
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 4);
    cut_character_at_linetail(buf, 0xFF);
    cut_character_at_linetail(buf, ' ');
    convert_language_code( 	buf,
    skin_rep_data_line_p->mp3_id3v1_year,
    sizeof(skin_rep_data_line_p->mp3_id3v1_year),
    CODE_AUTO, global_param.client_language_code);
    // コメント
    memset(buf, '\0', sizeof(buf));
    read(fd, buf, 28);
    cut_character_at_linetail(buf, 0xFF);
    cut_character_at_linetail(buf, ' ');
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
    off_t		len;
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
            cut_character_at_linetail(frame + 1, ' ');
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
        cut_after_last_character(dist_name, '.');
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
    replace_character(dist_name, dist_size, "|", "!");
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
    p += sprintf(p, "[playlist]\n");
    p += sprintf(p, "numberofentries=1\n");
    p += sprintf(p, "File1=%s\n", uri_string);
    p += sprintf(p, "Length1=-1\n");
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
        cut_after_last_character(pathname, '/');
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
static int read_avi_info(unsigned char *fname, SKIN_REPLASE_LINE_DATA_T *skin_rep_data_line_p)
{
    FILE *fp;
    MainAVIHeader avih;
    AVIStreamHeader avish;
    int len, listlen;
    time_t t;
    FOURCC fcc;
    fp = fopen((char*)fname, "rb");
    if (fp == NULL) return -1;
    listlen = read_avi_main_header(fp, &avih);
    if (listlen < 0) return -1;
    snprintf(skin_rep_data_line_p->image_width, sizeof(skin_rep_data_line_p->image_width), "%lu", avih.dwWidth );
    snprintf(skin_rep_data_line_p->image_height, sizeof(skin_rep_data_line_p->image_height), "%lu", avih.dwHeight );
    snprintf(skin_rep_data_line_p->avi_fps, sizeof(skin_rep_data_line_p->avi_fps), "%.3f", 1000000.0/avih.dwMicroSecPerFrame);
    t = (time_t)((double)avih.dwTotalFrames * avih.dwMicroSecPerFrame / 1000000);
    snprintf(skin_rep_data_line_p->avi_duration, sizeof(skin_rep_data_line_p->avi_duration), "%lu:%02lu:%02lu", t/3600, (t/60)%60, t%60);
    while ((len = read_next_chunk(fp, &fcc)) > 0) {
        //debug_log_output("--- %s\n", str_fourcc(fcc));
        if (fcc != SYM_LIST) break;
        if (read_avi_stream_header(fp, &avish, len) < 0) break;
        //debug_log_output("fccType: %s\n", str_fourcc(avish.fccType));
        //debug_log_output("fccHandler: %s (%s)\n", str_fourcc(avish.fccHandler), str_fourcc(avish.dwReserved1));
        //debug_log_output("rate: %d\n", avish.dwRate);
        if (avish.fccType == SYM_VIDS) {
            snprintf(skin_rep_data_line_p->avi_vcodec, sizeof(skin_rep_data_line_p->avi_hvcodec), "%s", str_fourcc(avish.fccHandler));
            snprintf(skin_rep_data_line_p->avi_hvcodec, sizeof(skin_rep_data_line_p->avi_vcodec), "%s", str_fourcc(avish.dwReserved1));
        } else if (avish.fccType == SYM_AUDS) {
            snprintf(skin_rep_data_line_p->avi_acodec, sizeof(skin_rep_data_line_p->avi_hacodec), "%s", str_acodec(avish.fccHandler));
            snprintf(skin_rep_data_line_p->avi_hacodec, sizeof(skin_rep_data_line_p->avi_acodec), "%s", str_acodec(avish.dwReserved1));
        }
    }
    // Interleave check :)
    do {
        int riff_type;
        if (fcc == SYM_LIST) {
            if ((riff_type = read_next_sym(fp)) == -1) {
                return -1;
            }
            len -= sizeof(riff_type);
            if (riff_type == SYM_MOVI) {
                DWORD old = 0;
                int i;
                for (i=0; i<100 && (len = read_next_chunk(fp, &fcc))>=0; i++) {
                    len = (len + 1)/ 2 * 2;
                    fseek(fp, len, SEEK_CUR);
                    /* mask BE value '01wb' into '01__', in LE. */
                    if (i != 0 && (fcc & 0x0000ffff) != old) break;
                    old = fcc & 0x0000ffff;
                }
                strncpy(skin_rep_data_line_p->avi_is_interleaved, (i<100) ? "I" : "NI", sizeof(skin_rep_data_line_p->avi_is_interleaved));
                break;
            }
        }
        fseek(fp, len, SEEK_CUR);
    } while ((len = read_next_chunk(fp, &fcc)) > 0);
    fclose(fp);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////
int http_index( int accept_socket , unsigned char* send_filename )
{
    unsigned char       document_path[MYFILENAME_MAX];
    unsigned char       read_filename[MYFILENAME_MAX];
    unsigned char       *index_html;
    //unsigned char       work[MYFILENAME_MAX];
    int                 index_html_malloc_size;
    strncpy((char*)document_path, (char*)send_filename, sizeof(document_path) );
    if ( document_path[ strlen((char*)document_path)-1 ] != DELIMITER[0] ){// 最後が'/'じゃなかったら、'/'を追加
        strncat((char*)document_path, DELIMITER, sizeof(document_path) -strlen((char*)document_path) );
    }
    // ----------------------------------------------
    // document_root/index.* のフルパス生成
    // ----------------------------------------------
    strncpy((char*)read_filename, (char*)document_path, sizeof( read_filename) );
    strncat((char*)read_filename, "index.html" , sizeof(read_filename) - strlen((char*)read_filename) );
    if( access( (char*)read_filename , 0 ) != 0 ){
        strncpy((char*)read_filename, (char*)document_path, sizeof( read_filename) );
        strncat((char*)read_filename, "index.htm" , sizeof(read_filename) - strlen((char*)read_filename) );
        if( access( (char*)read_filename , 0 ) != 0 ){
            strncpy((char*)read_filename, (char*)document_path, sizeof( read_filename) );
            strncat((char*)read_filename, "index.cgi" , sizeof(read_filename) - strlen((char*)read_filename) );
            if( access( (char*)read_filename , 0 ) != 0 ){
                return 0;
            }
        }
    }
    index_html = skin_file_read( read_filename, &index_html_malloc_size );
    if ( index_html == NULL ){
        debug_log_output("skin_file_read() error.");
        return 0;
    }
    // 204/12/15 Add start
    // index 内のwizdliveタグの置換処理
    //strcpy( (char*)work, wizdLive_url.c_str());
    //cut_after_last_character(work,'/');
    //strcat( (char*)work, "/");
    //replace_character(index_html , (BYTE*)SKIN_KEYWORD_WIZD_LIVE_SITE_URL , work );
    //replace_character(index_html , (BYTE*)SKIN_KEYWORD_WIZD_LIVE_SITE_URL2 , (unsigned char*)ROOTPAGE );
    // 204/12/15 Add end
    // =================
    // 返信実行
    // =================
    http_filemenu_send( accept_socket , index_html );
    free( index_html );
    return 1;
}
