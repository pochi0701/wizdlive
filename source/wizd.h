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
#ifdef linux

typedef int HANDLE;
typedef int SOCKET;
typedef unsigned int DWORD;
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#define strncmpi strncasecmp
#define O_BINARY 0
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define SERROR(x) ((x) < 0 )
#else

typedef unsigned int pid_t;
typedef unsigned int ssize_t;
typedef int socklen_t;
#define STDOUT_FILENO 0
#define STDIN_FILENO 1
#define STDERR_FILENO 2
#define strcasecmp strcmpi
#define strncasecmp strnicmp
#define strtoull strtoul
#define SERROR(x) ((x) == INVALID_SOCKET)

#endif

#include <time.h>
#include "const.h"
#include "wizd_String.h"

FILE* myfopen(const char* filename,const char* mode);
int myopen(const char* filename,int amode,int option=0);
// ======================
// define いろいろ
// ======================
//WINSOCK2より
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#define SERVER_NAME     "wizdLive "
#define SERVER_DETAIL   "MediaWiz Server Daemon."
#ifndef TRUE
#define TRUE    (1)
#endif
#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef IGNNRE_PARAMETER
#define IGNORE_PARAMETER(n) ((void)n)
#endif
#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif
#ifndef QUERY_MAX
#define QUERY_MAX 1024
#endif


#define LISTEN_BACKLOG                  (32)
#define SEND_BUFFER_SIZE                (1024*128)
#define DEFAULT_SERVER_PORT             (8000)
#define DEFAULT_DOCUMENT_ROOT           "/"
#define DEFAULT_FLAG_DAEMON             TRUE
#define DEFAULT_FLAG_AUTO_DETECT        TRUE
#define DEFAULT_MIME_TYPE               "text/plain"

#ifdef linux
#define DEFAULT_CONF_FILENAME1          "./wizd.conf"
#else
#define DEFAULT_CONF_FILENAME1          "wizd.conf"
#endif
#define	DEFAULT_CONF_FILENAME2	        "/usr/local/wizd/wizd.conf"
#define	DEFAULT_CONF_FILENAME3	        "/etc/wizd.conf"

#define	DEFAULT_FLAG_DEBUG_LOG_OUTPUT	TRUE
#ifdef linux
#define	DEFAULT_DEBUG_LOG_FILENAME		"/tmp/wizd_debug.log"
#else
#define DEFAULT_DEBUG_LOG_FILENAME      "wizd_debug.log"
#endif
#define DEFAULT_FLAG_USE_SKIN           TRUE

#define DEFAULT_SKINDATA_ROOT           "./skin"
#define DEFAULT_SKINDATA_NAME           "default"
#define DEFAULT_WORKROOT_NAME           "./work"
#define MAX_EVENTS                      100
// execute path for CGI
#define	DEFAULT_PATH	"/usr/bin:/bin:/usr/sbin:/usr/bin"
#define	DEFAULT_FLAG_UNKNOWN_EXTENSION_FLAG_HIDE		TRUE
#define	DEFAULT_FLAG_FILENAME_CUT_PARENTHESIS_AREA		FALSE
#define	DEFAULT_FLAG_FILENAME_CUT_SAME_DIRECTORY_NAME	FALSE
#define	DEFAULT_FLAG_ALLPLAY_FILELIST_ADJUST			FALSE
#define	DEFAULT_FLAG_FILENAME_ADJUSTMENT_FOR_WINDOWS	FALSE
#define	DEFAULT_FLAG_EXECUTE_CGI	TRUE
#define	DEFAULT_FLAG_ALLOW_PROXY	TRUE
#define	DEFAULT_DEBUG_CGI_OUTPUT	"/dev/null"
#define	DEFAULT_FLAG_SHOW_FIRST_VOB_ONLY	TRUE
#define	DEFAULT_FLAG_SPECIFIC_DIR_SORT_TYPE_FIX TRUE
// wizd.conf 変更対応
#define DEFAULT_ALLOW_USER_AGENT        "Mozilla"
#define DEFAULT_PC_PLAYLIST_OUT         FALSE
#define NKF_CODE_SJIS                   "s"
#define NKF_CODE_EUC                    "e"
#define CODE_AUTO                      (0)
#define CODE_SJIS                      (1)
#define CODE_EUC                       (2)
#define CODE_UTF8                      (3)
#define CODE_UTF16                     (4)
#define DEFAULT_CLIENT_LANGUAGE_CODE    CODE_SJIS
#define DEFAULT_SERVER_LANGUAGE_CODE    CODE_AUTO
#define HTTP_USER_AGENT         "User-agent:"
#define HTTP_RANGE              "Range:"
#define HTTP_HOST               "Host:"
#define HTTP_CONTENT_LENGTH1	"Content-Length:"
#define HTTP_CONTENT_TYPE1      "Content-Type:"

#define	HTTP_OK 	        "HTTP/1.0 200 OK\r\n"
#define	HTTP_NOT_FOUND 		"HTTP/1.0 404 File Not Found\r\n"
#define HTTP_NOT_FOUND1     "HTTP/1.x 404 Not Found\r\n"
#ifdef linux
#define HTTP_CONTENT_LENGTH	"Content-Length: %zu\r\n"
#else
#define HTTP_CONTENT_LENGTH     "Content-Length: %llu\r\n"
#endif
#define	HTTP_ACCEPT_RANGES	"Accept-Ranges: bytes\r\n"
#define HTTP_CONTENT_TYPE 	"Content-Type: %s\r\n"
#define	HTTP_SERVER_NAME	"Server: %s\r\n"
#define	HTTP_CONNECTION		"Connection: Close\r\n"
#define HTTP_ACCEPTRANGE	"Accept-Ranges: bytes\r\n"
#define HTTP_DATE		"Date: %s\r\n"
#define HTTP_END		"\r\n"


// アクセスコントロール 登録可能数
#define		ACCESS_ALLOW_LIST_MAX	        (32)
#define		ALLOW_USER_AGENT_LIST_MAX	(32)
// 隠しディレクトリ登録可能数
#define		SECRET_DIRECTORY_MAX            (4)
// MIME_LIST_T.stream_type 用
#define		TYPE_STREAM			(0)
#define		TYPE_NO_STREAM	          	(1)
// MIME_LIST.menu_file_type用
#define		TYPE_UNKNOWN			(0)
#define		TYPE_DIRECTORY			(1)
#define		TYPE_MOVIE		        (2)
#define		TYPE_MUSIC			(3)
#define		TYPE_IMAGE			(4)
#define		TYPE_DOCUMENT			(5)
#define         TYPE_PLAYLIST                   (7)
#define         TYPE_MUSICLIST                  (9)
#define         TYPE_JPEG                       (10)
#define         TYPE_URL                        (11)
#define         NO_RESPONSE_TIMEOUT             (12)

#define MULTI_ACCESS_MAX                (5)
#define TIMEOUT_SECOND                  (300)
#define MAX_COUNT_ALIAS                 (3)
// ==========================================================================
// MIMEリスト保存用構造体
// ==========================================================================
typedef struct {
    char   *mime_name;
    char   *file_extension;
    int	   stream_type;
    int	   menu_file_type;
} MIME_LIST_T;
// ==========================================================================
// 拡張子変換テーブル
// ==========================================================================
typedef struct {
    char   *org_extension;
    char   *rename_extension;
} EXTENSION_CONVERT_LIST_T;
// ==========================================================================
// HTTP Request情報保存用構造体
// ==========================================================================
typedef struct {
    char   recv_uri[QUERY_MAX];      // 受信したURI(decoded)
    char   user_agent[256];          // 受信したUser-Agent
    char   recv_host[256];           // 受信したホスト名
    char   recv_range[256];          // 受信した Range
    off_t  range_start_pos;          // Rangeデータ 開始位置
    off_t  range_end_pos;            // Rangeデータ 終了位置
    char   content_length[32];	     // Content-Length
    char   content_type[128];	     // PUTのためのContent_type
    char   mime_type[128];           //
    char   send_filename[QUERY_MAX]; // フルパス
    char   action[128];              // ?action=  の内容
    char   request_uri[QUERY_MAX];   // 受信した生のURI
    int    isGet;		     // GETなら1HEADなら2POSTなら3
} HTTP_RECV_INFO;
// ==========================================================================
// 全体パラメータ保存用構造体
// ==========================================================================
typedef struct {
    // -----------------
    // システム系
    // -----------------
    // デーモン化する/しない
    char        flag_daemon;
    // デバッグログ
    char        flag_debug_log_output;
    char	debug_log_filename[FILENAME_MAX];
    // 動作ユーザー名
    char        exec_user[32];
    char        exec_group[32];
    // -----------------
    // 自動検出系
    // -----------------
    // サーバホスト名
    char        server_name[32];
    char        flag_auto_detect;
    char        auto_detect_bind_ip_address[32];
    // --------------------
    // HTTP Server系
    // --------------------
    // HTTP Server Port
    int         server_port;
    // Document Root
    char        document_root[FILENAME_MAX];
    // Alias
    char        document_org[FILENAME_MAX];
    char        alias_key[MAX_COUNT_ALIAS][FILENAME_MAX];
    char        alias_rep[MAX_COUNT_ALIAS][FILENAME_MAX];
    // ----------------------
    // 表示系
    // ----------------------
    // MediaWiz の言語コード
    int         client_language_code;
    // Serverの言語コード
    int         server_language_code;
    // スキンを使用する／しない
    char        flag_use_skin;
    // スキン置き場
    char        skin_root[FILENAME_MAX];
    // スキン名
    char        skin_name[32];
    // ワーク作成場所
    char        work_root[FILENAME_MAX];
    // wizd が知らないファイル名を隠すかフラグ
    char	flag_unknown_extention_file_hide;
    // 表示ファイル名から、()[]で囲まれた部分を削除するかフラグ
    char	flag_filename_cut_parenthesis_area;
    // 表示ファイル名で、親ディレクトリ名と同一文字列を削除するかフラグ
    char        flag_filename_cut_same_directory_name;
    // Allplayでの文字化け防止(ファイル名の全半角変換)するかフラグ
    char	flag_allplay_filelist_adjust;
    // Windows用にプレイリスト内のファイル名を調整するかフラグ
    char        flag_filename_adjustment_for_windows;
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
//    int         max_child_count;
    // CGIスクリプトの実行を許可するかフラグ
    int         flag_execute_cgi;
    // CGIスクリプトの標準エラー出力先
    char        debug_cgi_output[FILENAME_MAX];
    // プロクシを許可するかフラグ
    int         flag_allow_proxy;
} GLOBAL_PARAM_T;
// IPアドレスアクセスコントロール用
typedef struct {
    int				flag;			// 値が入ってるか
    char 	address[4];		// アドレス
    char 	netmask[4];		// ネットマスク
} ACCESS_CHECK_LIST_T;
// User-Agent アクセスコントロール
typedef struct {
    char	user_agent[64];
} ACCESS_USER_AGENT_LIST_T;
// 隠しディレクトリ
typedef struct {
    char	dir_name[64];	// 隠しディレクトリ名
    int				tvid;			// アクセスTVID
} SECRET_DIRECTORY_T;
#define		JOINT_MAX	(255)
// ********************************
// JOINTする個々のファイル情報
// ********************************
typedef struct {
    char   name[FILENAME_MAX];
    off_t   size;
} _FILE_INFO_T;
// ****************************************
// JOINTファイル情報 (VOB解析情報)
// ****************************************
typedef struct {
    unsigned int		file_num;		// 全ファイル数
    size_t			total_size;		// 全ファイル総byte数
    _FILE_INFO_T		file[JOINT_MAX];	// JOINTファイル情報
    unsigned int	current_file_num;			// とりあえずVOB専用
} JOINT_FILE_INFO_T;
// 2004/08/02 Add test
typedef struct {
    SOCKET                      accept_socket;         // SOCKET
    char*                       access_host;           // アクセスしてきたIP
    struct  sockaddr_in         caddr;
} ACCESS_INFO;
// 2004/08/02 Add test
// 2004/08/11 Add test
typedef struct {
    char           access_ip[32];           // アクセスしてきたIP
    char           *user_name;           // ユーザー名
    char           *user_pass;           // ユーザーパスワード
    int                     login_flag;           // ログインフラグ
    time_t                  login_time;           // ログイン時刻
    int                     file_send_flag;         // ファイル転送中フラグ(ファイル転送中はタイムアウトさせないため)
    // 2004/09/24 Add start
    char           recv_uri_log[FILENAME_MAX];           // 受信したURI(decoded)のログ
    // 2004/09/24 Add end
    // 2004/10/04 Add start
    char           recv_uri_last[FILENAME_MAX];          // 受信したURI(decoded)のログ
    // 2004/10/04 Add end
} ACCESS_USER_INFO;
typedef struct {
    ACCESS_USER_INFO    user_info[MULTI_ACCESS_MAX];
    int                 list_num;
} ACCESS_USER_INFO_LIST;
// ======================
// extern いろいろ
// ======================
// ------------------
// 各種リスト
// ------------------
extern GLOBAL_PARAM_T   global_param;
extern MIME_LIST_T      mime_list[];
extern EXTENSION_CONVERT_LIST_T extension_convert_list[];
// アクセス許可リスト
extern ACCESS_CHECK_LIST_T      access_allow_list[ACCESS_ALLOW_LIST_MAX];
// User-Agent 許可リスト
extern ACCESS_USER_AGENT_LIST_T allow_user_agent[ALLOW_USER_AGENT_LIST_MAX];
// 隠しディレクトリ リスト
extern SECRET_DIRECTORY_T secret_directory_list[SECRET_DIRECTORY_MAX];
extern ACCESS_USER_INFO_LIST    user_info_list;
extern int loop_flag;
extern int view_flag;
extern int Ready_flag;
extern SOCKET           listen_socket;  // 待ち受けSocket
// ------------------
// グローバル関数
// ------------------
// wizd 初期化
extern void global_param_init(void);
extern int Initialize(void);

// config_file(wizd.conf) 読み込み部
extern void config_file_read(void);
extern void config_sanity_check(void);
// MediaWiz 自動登録部
extern void*    server_detect(void* ptr);
extern void     detect_finalize(void);
// HTTP 待ち受け部
extern void server_listen(void);
// HTTP処理部
extern void server_http_process(int accept_socket);

// ヘッダ出力
extern int http_header_response(int accept_socket, HTTP_RECV_INFO* http_recv_info_p,size_t content_length);

// バッファリングしながら in_fd から out_fd へ データを転送
extern int copy_descriptors(int in_fd,int out_fd,size_t content_length);

// Proxy解析＆返信
extern int http_proxy_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );
// CGI解析＆返信
extern int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p );
// ファイル実体返信
extern int http_file_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p);
// MIME
extern void check_file_extension_to_mime_type(const char *file_extension, char *mime_type, int mime_type_size );
// 日本語文字コード変換(NKFラッパー）
extern void convert_language_code(const char *in, char *out, size_t len, int in_flag, int out_flag);
int copy_body(int in_fd, int _out_fd, size_t content_length);
// 文字コード変換。
// libnkfをそのまま使用。作者様に感謝ヽ(´ー｀)ノ
// http://www.mr.hum.titech.ac.jp/~morimoto/libnkf/
// ========================================================
int nkf(const char *in,char *out,size_t len,const char *options);
extern unsigned char *base64(unsigned char *str);
extern unsigned char *unbase64(unsigned char *str);
#endif
