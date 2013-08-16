// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd.h
//											$Revision: 1.26 $
//											$Date: 2004/07/19 04:37:32 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#ifndef	_WIZD_H
#define	_WIZD_H


#include	"wizd_tools.h"
#include        "const.h"

// ======================
// define いろいろ
// ======================

#define		SERVER_NAME		"wizd 0.12g"
#define		SERVER_DETAIL	"MediaWiz Server Daemon."


#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif

#define		LISTEN_BACKLOG	(32)

#define		SEND_BUFFER_SIZE	(1024*128)


#define DEFAULT_SERVER_PORT (8000)
#define	DEFAULT_DOCUMENT_ROOT	"/"


#define	DEFAULT_FLAG_DAEMON			TRUE
#define	DEFAULT_FLAG_AUTO_DETECT	TRUE




#define	DEFAULT_MIME_TYPE		"text/plain"

#define	DEFAULT_INDEX_FILENAME	"index.html"

#define	DEFAULT_CONF_FILENAME1	"./wizd.conf"
#define	DEFAULT_CONF_FILENAME2	"/usr/local/wizd/wizd.conf"
#define	DEFAULT_CONF_FILENAME3	"/etc/wizd.conf"

#define	DEFAULT_FLAG_DEBUG_LOG_OUTPUT	FALSE
#define	DEFAULT_DEBUG_LOG_FILENAME		"/tmp/wizd_debug.log"


#define	DEFAULT_FLAG_USE_SKIN		FALSE
#define	DEFAULT_SKINDATA_ROOT		"./skin"
#define	DEFAULT_SKINDATA_NAME		"default"

// execute path for CGI
#define	DEFAULT_PATH	"/usr/bin:/bin:/usr/sbin:/usr/bin"

#define	DEFAULT_MENU_FILENAME_LENGTH_MAX		FILENAME_MAX

#define	DEFAULT_FLAG_UNKNOWN_EXTENSION_FLAG_HIDE	TRUE

#define	DEFAULT_FLAG_FILENAME_CUT_PARENTHESIS_AREA	FALSE

#define	DEFAULT_FLAG_DECODE_SAMBA_HEX_AND_CAP			FALSE
#define	DEFAULT_FLAG_FILENAME_CUT_SAME_DIRECTORY_NAME	FALSE

#define	DEFAULT_FLAG_ALLPLAY_FILELIST_ADJUST			FALSE

#define	DEFAULT_FLAG_FILENAME_ADJUSTMENT_FOR_WINDOWS	FALSE

#define	DEFAULT_FLAG_EXECUTE_CGI	TRUE
#define	DEFAULT_FLAG_ALLOW_PROXY	TRUE

#define	DEFAULT_DEBUG_CGI_OUTPUT		"/dev/null"

#define	DEFAULT_USER_AGENT_PC		""

#define	DEFAULT_FLAG_SHOW_FIRST_VOB_ONLY	TRUE

#define	DEFAULT_FLAG_SPECIFIC_DIR_SORT_TYPE_FIX TRUE

#define	SORT_NONE				(0)
#define	SORT_NAME_UP			(1)
#define	SORT_NAME_DOWN			(2)
#define	SORT_TIME_UP			(3)
#define	SORT_TIME_DOWN			(4)
#define	SORT_SIZE_UP			(5)
#define	SORT_SIZE_DOWN			(6)
#define	SORT_SHUFFLE			(7)

#define	DEFAULT_SORT_RULE		SORT_NONE
#define	DEFAULT_PAGE_LINE_MAX	(14)


#define	NKF_CODE_SJIS			"s"
#define	NKF_CODE_EUC			"e"


#define CODE_AUTO		(0)
#define CODE_SJIS		(1)
#define CODE_EUC		(2)
#define CODE_UTF8		(3)
#define CODE_UTF16		(4)

#define	DEFAULT_CLIENT_LANGUAGE_CODE	CODE_SJIS
#define	DEFAULT_SERVER_LANGUAGE_CODE	CODE_AUTO



#define	HTTP_USER_AGENT		"User-agent:"
#define	HTTP_RANGE			"Range:"
#define	HTTP_HOST			"Host:"

#define	HTTP_OK 			"HTTP/1.0 200 OK\r\n"
#define	HTTP_NOT_FOUND 		"HTTP/1.0 404 File Not Found\r\n"
#define HTTP_NOT_FOUND1         "HTTP/1.x 404 Not Found"
#define HTTP_CONTENT_LENGTH	"Content-Length: %lu\r\n"
#define	HTTP_ACCEPT_RANGES	"Accept-Ranges: bytes\r\n"
#define HTTP_CONTENT_TYPE 	"Content-Type: %s\r\n"
#define	HTTP_SERVER_NAME	"Server: %s\r\n"
#define	HTTP_CONNECTION		"Connection: Close\r\n"
#define HTTP_END			"\r\n"


// アクセスコントロール 登録可能数
#define		ACCESS_ALLOW_LIST_MAX	(32)


#define		ALLOW_USER_AGENT_LIST_MAX	(32)

// 隠しディレクトリ登録可能数
#define		SECRET_DIRECTORY_MAX	(4)


// MIME_LIST_T.stream_type 用
#define		TYPE_STREAM			(0)
#define		TYPE_NO_STREAM		(1)

// MIME_LIST.menu_file_type用
#define		TYPE_UNKNOWN			(0)
#define		TYPE_DIRECTORY			(1)
#define		TYPE_MOVIE				(2)
#define		TYPE_MUSIC				(3)
#define		TYPE_IMAGE				(4)
#define		TYPE_DOCUMENT			(5)
#define		TYPE_PLAYLIST			(7)
#define		TYPE_PSEUDO_DIR			(8)
#define		TYPE_MUSICLIST			(9)

// SVI_INFO

#define SVI_FILENAME_OFFSET             (4368)
#define SVI_FILENAME_LENGTH             (255)

// ==========================================================================
// MIMEリスト保存用構造体
// ==========================================================================
typedef struct {
	unsigned char	*mime_name;
	unsigned char	*file_extension;
	int				stream_type;
	int				menu_file_type;
} MIME_LIST_T;


// ==========================================================================
// 拡張子変換テーブル
// ==========================================================================
typedef struct {
	unsigned char	*org_extension;
	unsigned char	*rename_extension;
} EXTENSION_CONVERT_LIST_T;





// ==========================================================================
// HTTP Request情報保存用構造体
// ==========================================================================
typedef struct {
	unsigned char 	recv_uri[FILENAME_MAX];		// 受信したURI(decoded)
	unsigned char 	user_agent[256];			// 受信したUser-Agent
	unsigned char	recv_host[256];				// 受信したホスト名

	unsigned char	recv_range[256];			// 受信した Range
	off_t	range_start_pos;			// Rangeデータ 開始位置
	off_t	range_end_pos;				// Rangeデータ 終了位置

	unsigned char	mime_type[128];		//
	unsigned char	send_filename[FILENAME_MAX];	// フルパス


	unsigned char	action[128];	// ?action=  の内容
	int				page;			// ?page=	で指定された表示ページ

	unsigned char 	option[32];		// ?option= の内容
	unsigned char	sort[32];		// ?sort= の内容

	unsigned char 	focus[32];		// ?focus= の内容

	unsigned char 	request_uri[FILENAME_MAX];		// 受信した生のURI

	int				flag_pc;		// クライアントが PC かどうか. 非0 = PC
    int				isGet;			// GETなら1HEADなら2
} HTTP_RECV_INFO;



// ==========================================================================
// 全体パラメータ保存用構造体
// ==========================================================================
typedef struct {


	// -----------------
	// システム系
	// -----------------

	// デーモン化する/しない
	char			flag_daemon;

	// デバッグログ
	char			flag_debug_log_output;
	unsigned char	debug_log_filename[FILENAME_MAX];


	// 動作ユーザー名
	unsigned char	exec_user[32];
	unsigned char	exec_group[32];

	// -----------------
	// 自動検出系
	// -----------------

	// サーバホスト名
	unsigned char	server_name[32];

	char			flag_auto_detect;
	unsigned char	auto_detect_bind_ip_address[32];

	// --------------------
	// HTTP Server系
	// --------------------

	// HTTP Server Port
	int		server_port;

	// Document Root
	unsigned char 	document_root[FILENAME_MAX];



	// ----------------------
	// 表示系
	// ----------------------

	// MediaWiz の言語コード
	int	client_language_code;

	// Serverの言語コード
	int	server_language_code;


	// スキンを使用する／しない
	char			flag_use_skin;

	// スキン置き場
	unsigned char	skin_root[FILENAME_MAX];

	// スキン名
	unsigned char	skin_name[32];

	// ファイルソートのルール
	int		sort_rule;

	//ファイルリストの１ページの最大表示数
	int		page_line_max;

	// ファイル名表示の最大長
	int		menu_filename_length_max;

	//sambaのCAP/HEXエンコード使用するかフラグ
	char	flag_decode_samba_hex_and_cap;

	// wizd が知らないファイル名を隠すかフラグ
	char	flag_unknown_extention_file_hide;

	// 表示ファイル名から、()[]で囲まれた部分を削除するかフラグ
	char	flag_filename_cut_parenthesis_area;

	// 表示ファイル名で、親ディレクトリ名と同一文字列を削除するかフラグ
	char	flag_filename_cut_same_directory_name;

	// Allplayでの文字化け防止(ファイル名の全半角変換)するかフラグ
	char	flag_allplay_filelist_adjust;

	// Windows用にプレイリスト内のファイル名を調整するかフラグ
	char	flag_filename_adjustment_for_windows;

	// ----------------------
	// 拡張系
	// ----------------------

	// キャッシュバッファのサイズ(個数)
	size_t	buffer_size;

	// キャッシュバッファをすぐ送るかフラグ
	char	flag_buffer_send_asap;

	// プロクシで User-Agent を上書きするならその文字列
	char	user_agent_proxy_override[128];

	// 子プロセスの制限
	int		max_child_count;

	// CGIスクリプトの実行を許可するかフラグ
	int		flag_execute_cgi;

	// CGIスクリプトの標準エラー出力先
	unsigned char	debug_cgi_output[FILENAME_MAX];

	// プロクシを許可するかフラグ
	int		flag_allow_proxy;

	// PCと判断する User-Agent
	unsigned char 	user_agent_pc[64];

	int				menu_font_metric;
	unsigned char	menu_font_metric_string[128];

	// 0.12f3 先頭vobのみ
	char	flag_show_first_vob_only;

	// 0.12f4 特定のディレクトリのソート方法を固定する。
	char	flag_specific_dir_sort_type_fix;

} GLOBAL_PARAM_T;



// IPアドレスアクセスコントロール用
typedef struct {
	int				flag;			// 値が入ってるか
	unsigned char 	address[4];		// アドレス
	unsigned char 	netmask[4];		// ネットマスク

} ACCESS_CHECK_LIST_T;


// User-Agent アクセスコントロール
typedef struct {
	unsigned char	user_agent[64];
} ACCESS_USER_AGENT_LIST_T;


// 隠しディレクトリ
typedef struct {
	unsigned char	dir_name[64];	// 隠しディレクトリ名
	int				tvid;			// アクセスTVID
} SECRET_DIRECTORY_T;




#define		JOINT_MAX	(255)

// ********************************
// JOINTする個々のファイル情報
// ********************************
typedef struct {
	unsigned char	name[FILENAME_MAX];
	//u_int64_t		size;
        off_t			size;
} _FILE_INFO_T;


// ****************************************
// JOINTファイル情報 (VOB解析情報)
// ****************************************
typedef struct {
	unsigned int		file_num;		// 全ファイル数
	off_t			total_size;		// 全ファイル総byte数

	_FILE_INFO_T		file[JOINT_MAX];	// JOINTファイル情報

	unsigned int	current_file_num;			// とりあえずVOB専用
} JOINT_FILE_INFO_T;



// ======================
// extern いろいろ
// ======================

// ------------------
// 各種リスト
// ------------------
extern GLOBAL_PARAM_T	global_param;
extern MIME_LIST_T	mime_list[];
extern EXTENSION_CONVERT_LIST_T extension_convert_list[];


// アクセス許可リスト
extern ACCESS_CHECK_LIST_T	access_allow_list[ACCESS_ALLOW_LIST_MAX];

// User-Agent 許可リスト
extern ACCESS_USER_AGENT_LIST_T	allow_user_agent[ALLOW_USER_AGENT_LIST_MAX];

// 隠しディレクトリ リスト
extern SECRET_DIRECTORY_T secret_directory_list[SECRET_DIRECTORY_MAX];

// ------------------
// グローバル関数
// ------------------


// wizd 初期化
extern void global_param_init(void);

// config_file(wizd.conf) 読み込み部
extern void config_file_read(void);

extern void config_sanity_check(void);

extern void skin_config_file_read(unsigned char *skin_conf_filename);


// MediaWiz 自動登録部
extern void	server_detect(void);


// HTTP 待ち受け部
extern void server_listen(void);

// HTTP処理部
extern void server_http_process(int accept_socket);



// バッファリングしながら in_fd から out_fd へ データを転送
extern int copy_descriptors(int in_fd,int out_fd,off_t content_length,JOINT_FILE_INFO_T *joint_file_info_p,char* infilename,off_t range_start_pos);
extern int copy_descriptors2(int, int, off_t, JOINT_FILE_INFO_T*, char*, off_t);
//extern int copy_descriptors(int in_fd, int out_fd, off_t content_length, JOINT_FILE_INFO_T *joint_file_info_p);

extern int analyze_vob_file(unsigned char *vob_filename, JOINT_FILE_INFO_T *joint_file_info_p );


// Proxy解析＆返信
extern int http_proxy_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// VOBファイル解析＆返信
extern int http_vob_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// CGI解析＆返信
extern int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );

// ファイル実体返信
extern int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);


// メニュー部
extern void http_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info, int flag_pseudo);


// Image Viewer（画像表示部)
extern void http_image_viewer(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// Option Menu(Sortモード変更)
extern void http_option_menu(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// Music Single Play.(音楽ファイル １曲分の演奏データ送信)
extern void http_music_single_play(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// wizd play list(*.plw)ファイル処理。
extern void http_listfile_to_playlist_create(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);

// MIME
extern void check_file_extension_to_mime_type(const unsigned char *file_extension, unsigned char *mime_type, int mime_type_size );


// 日本語文字コード変換(NKFラッパー）
extern void convert_language_code(const unsigned char *in, unsigned char *out, size_t len, int in_flag, int out_flag);


// ========================================================
// 文字コード変換。
// libnkfをそのまま使用。作者様に感謝ヽ(´ー｀)ノ
// http://www.mr.hum.titech.ac.jp/~morimoto/libnkf/
// ========================================================
int nkf(const char *, char *, size_t, const char *);


#ifdef __CYGWIN__
char *strcasestr(const char *p1, const char *p2);
#endif

#endif

