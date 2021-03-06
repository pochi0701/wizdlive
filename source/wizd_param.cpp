// ==========================================================================
//code=UTF8  tab=4
//
// wizd:    MediaWiz Server daemon.
//
//      wizd_param.c
//                                          $Revision: 1.15 $
//                                          $Date: 2004/07/19 04:37:32 $
//
//  すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef linux
#include <unistd.h>
#else
#include <io.h>
#include <dir.h>
#endif
#include "wizd.h"
#include "wizd_tools.h"
#include "wizd_String.h"
extern wString current_dir;
static int config_file_open(void);
static int config_file_read_line(int fd, char *line_buf, int line_buf_size);
static void line_buffer_clearance(char *line_buf);
// ********************************************
// MIME リスト
// とりあえず知ってる限り書いておく。
// ********************************************
MIME_LIST_T     mime_list[] = {
    //  {mime_name           ,file_extension  ,stream_type     ,   menu_file_type  },
    {(char*)"text/plain"     ,(char*)"txt"    ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/html"      ,(char*)"htm"    ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/html"      ,(char*)"html"   ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/css"       ,(char*)"css"    ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/html"      ,(char*)"exe"    ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/html"      ,(char*)"jss"    ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/html"      ,(char*)"php"    ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"text/html"      ,(char*)"pl"     ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   },
    {(char*)"image/gif"      ,(char*)"gif"    ,TYPE_NO_STREAM  ,   TYPE_IMAGE      },
    {(char*)"image/jpeg"     ,(char*)"jpeg"   ,TYPE_NO_STREAM  ,   TYPE_IMAGE      },
    {(char*)"image/jpeg"     ,(char*)"jpg"    ,TYPE_NO_STREAM  ,   TYPE_IMAGE      },
    {(char*)"image/png"      ,(char*)"png"    ,TYPE_NO_STREAM  ,   TYPE_IMAGE      },
    {(char*)"image/x-icon"   ,(char*)"ico"    ,TYPE_NO_STREAM  ,   TYPE_IMAGE      },
    {(char*)"video/mpeg"     ,(char*)"mpeg"   ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/mpeg"     ,(char*)"mpg"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/mpeg"     ,(char*)"m2p"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/mpeg"     ,(char*)"hnl"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/x-msvideo",(char*)"avi"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/mpeg"     ,(char*)"vob"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/mpeg"     ,(char*)"vro"    ,TYPE_STREAM     ,   TYPE_MOVIE      },  /* add for DVD-RAM */
    {(char*)"video/quicktime",(char*)"mov"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/x-ms-wmv" ,(char*)"wmv"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/x-ms-wmx" ,(char*)"asf"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"audio/mpeg"     ,(char*)"mp3"    ,TYPE_STREAM     ,   TYPE_MUSIC      },
    {(char*)"audio/x-ogg"    ,(char*)"ogg"    ,TYPE_STREAM     ,   TYPE_MUSIC      },
    {(char*)"video/mp4"      ,(char*)"mp4"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/divx"     ,(char*)"divx"   ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"video/flv"      ,(char*)"flv"    ,TYPE_STREAM     ,   TYPE_MOVIE      },
    {(char*)"audio/x-ms-wma" ,(char*)"wma"    ,TYPE_STREAM     ,   TYPE_MUSIC      },
    {(char*)"audio/x-wav"    ,(char*)"wav"    ,TYPE_STREAM     ,   TYPE_MUSIC      },
    {(char*)"audio/ac3"      ,(char*)"ac3"    ,TYPE_STREAM     ,   TYPE_MUSIC      },
    {(char*)"audio/x-m4a"    ,(char*)"m4a"    ,TYPE_STREAM     ,   TYPE_MUSIC      },
    {(char*)"text/plain"     ,(char*)"plw"    ,TYPE_STREAM     ,   TYPE_PLAYLIST   }, // Play List for wizd.
    {(char*)"text/plain"     ,(char*)"upl"    ,TYPE_STREAM     ,   TYPE_PLAYLIST   }, // Uzu Play List拡張子でもOK. ファイル自身の互換は無し。
    {(char*)"text/plain"     ,(char*)"m3u"    ,TYPE_STREAM     ,   TYPE_MUSICLIST  }, // m3u でもOK?
    {(char*)"text/javascript",(char*)"js"     ,TYPE_NO_STREAM  ,   TYPE_DOCUMENT   }, //JavaScript
    {NULL, NULL, (-1), (-1) }
};
// ********************************************
// 拡張子変換リスト
// ********************************************
// WARNING: hoge.m2p -> hoge.mpg 「ではない」
//          hoge.m2p -> hoge.m2p.mpg になる。
//          hoge.m2p.mpg -> hoge.m2p になる。
//          hoge.SVI.mpg -> hoge.SVI になる。
EXTENSION_CONVERT_LIST_T extension_convert_list[] = {
    //  {org_extension  ,   rename_extension    }
    {(char*)"m2p"           ,   (char*)"mpg"            },
    {(char*)"hnl"           ,   (char*)"mpg"            },
    { NULL, NULL }
};
// ********************************************
// 全体パラメータ構造体の実体
// ********************************************
GLOBAL_PARAM_T  global_param;
// IPアクセス許可リスト
ACCESS_CHECK_LIST_T     access_allow_list[ACCESS_ALLOW_LIST_MAX];
// User-Agent 許可リスト
ACCESS_USER_AGENT_LIST_T        allow_user_agent[ALLOW_USER_AGENT_LIST_MAX];
// 隠しディレクトリ リスト
SECRET_DIRECTORY_T secret_directory_list[SECRET_DIRECTORY_MAX];
//終了フラグ
int loop_flag=1;
//リアルタイム視聴フラグ
int view_flag=0;
//レディ？
int Ready_flag=0;
// ********************************************
// 全体パラメータ構造体の初期化。
// デフォルト値をセット
// ********************************************
void global_param_init(void)
{
    memset(&user_info_list, 0, sizeof(user_info_list));
    user_info_list.list_num = 0;
    // 構造体まとめて初期化
    memset(&global_param, 0, sizeof(global_param));
    memset(access_allow_list, 0, sizeof(access_allow_list));
    memset(allow_user_agent, 0, sizeof(allow_user_agent));
    memset(secret_directory_list, 0, sizeof(secret_directory_list));
    // デーモン化フラグ
    global_param.flag_daemon            = DEFAULT_FLAG_DAEMON;
    // 自動検出
    global_param.flag_auto_detect   = DEFAULT_FLAG_AUTO_DETECT;
    // デフォルトServer名。gethostname()する。
    gethostname(global_param.server_name, sizeof(global_param.server_name));
    // デフォルトHTTP 待ち受けPort.
    global_param.server_port            = DEFAULT_SERVER_PORT;
    // Document Root
    strncpy(global_param.document_root, DEFAULT_DOCUMENT_ROOT, sizeof(global_param.document_root));
    // DebugLog
    global_param.flag_debug_log_output = DEFAULT_FLAG_DEBUG_LOG_OUTPUT;
    strncpy(global_param.debug_log_filename, DEFAULT_DEBUG_LOG_FILENAME, sizeof(global_param.debug_log_filename));
    // Client(MediaWiz)の言語コード
    global_param.client_language_code = DEFAULT_CLIENT_LANGUAGE_CODE;
    // Server側の言語コード
    global_param.server_language_code = DEFAULT_SERVER_LANGUAGE_CODE;
    // スキン情報使用フラグ
    global_param.flag_use_skin      = DEFAULT_FLAG_USE_SKIN;
    // スキン置き場
    strncpy(global_param.skin_root, DEFAULT_SKINDATA_ROOT, sizeof(global_param.skin_root));
    // スキン名
    strncpy(global_param.skin_name, DEFAULT_SKINDATA_NAME, sizeof(global_param.skin_name));
    // ワーク
    strncpy(global_param.work_root, DEFAULT_WORKROOT_NAME, sizeof(global_param.work_root));
    // wizdが知らない拡張子のファイルを隠すか否か
    global_param.flag_unknown_extention_file_hide = DEFAULT_FLAG_UNKNOWN_EXTENSION_FLAG_HIDE;
    // 表示ファイル名から、()[]で囲まれた部分を削除するか否か。
    global_param.flag_filename_cut_parenthesis_area = DEFAULT_FLAG_FILENAME_CUT_PARENTHESIS_AREA;
    // 表示ファイル名で、親ディレクトリ名と同一文字列を削除するかフラグ
    global_param.flag_filename_cut_same_directory_name = DEFAULT_FLAG_FILENAME_CUT_SAME_DIRECTORY_NAME;
    // Allplayでの文字化け防止(ファイル名の全半角変換)するかフラグ
    global_param.flag_allplay_filelist_adjust = DEFAULT_FLAG_ALLPLAY_FILELIST_ADJUST;
    // Windows用にプレイリスト内のファイル名を調整するかフラグ
    global_param.flag_filename_adjustment_for_windows = DEFAULT_FLAG_FILENAME_ADJUSTMENT_FOR_WINDOWS;
    // CGIスクリプトの実行を許可するかフラグ
    global_param.flag_execute_cgi       = DEFAULT_FLAG_EXECUTE_CGI;
    // CGIスクリプトの標準エラー出力先
    strncpy(global_param.debug_cgi_output, DEFAULT_DEBUG_CGI_OUTPUT, sizeof(global_param.debug_cgi_output));
    // プロクシを許可するかフラグ
    global_param.flag_allow_proxy       = DEFAULT_FLAG_ALLOW_PROXY;
    return;
}
// ****************************************************
// wizd.conf 読む
// ****************************************************
void config_file_read(void)
{
    int    fd;
    char   line_buf[1024*4];
    int ret;
    int count_alias            = 0;
    int count_access_allow     = 0;
    int count_allow_user_agent = 0;
    int count_secret_directory = 0;
    int i;
    char   key[1024];
    char   value[1024];
    char   work1[32];
    char   work2[32];
    char   work3[32];
    char   work4[32];
    // =======================
    // ConfigファイルOPEN
    // =======================
    fd = config_file_open();
    if ( fd < 0 ){
    return;
    }
    // =====================
    // 内容読み込み
    // =====================
    while ( 1 ){
        // １行読む。
        ret = config_file_read_line(fd, line_buf, sizeof(line_buf));
        if ( ret < 0 ){
        	#ifdef linux
            printf("EOF Detect.\n");
            #endif
            break;
        }
        // 読んだ行を整理する。
        line_buffer_clearance(line_buf);
        if ( strlen(line_buf) > 0 ) // 値が入ってた。
        {
            // ' 'で、前後に分ける
            sentence_split(line_buf, ' ', key, value);
            //printf("key='%s', value='%s'\n", key, value);
            // ---------------------
            // 値の読みとり実行。
            // ---------------------
            if (( strlen(key) > 0 ) && (strlen(value) > 0 ))
            {

                // flag_auto_detect
                if ( strcasecmp("flag_auto_detect", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 ){
                        global_param.flag_auto_detect = TRUE;
                    }else if (strcasecmp(value ,"false") == 0 ){
                        global_param.flag_auto_detect = FALSE;
                    } 
                }
#ifdef linux
                // flag_daemon
                else if ( strcasecmp("flag_daemon", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_daemon = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_daemon = FALSE;
                }
                // flag_debug_log_output
                else if ( strcasecmp("flag_debug_log_output", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_debug_log_output = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_debug_log_output = FALSE;
                }
                // debug_log_filename
                else if ( strcasecmp("debug_log_filename", key) == 0 ){
                    strncpy(global_param.debug_log_filename, value, sizeof(global_param.debug_log_filename));
                }
                // exec_user
                else if ( strcasecmp("exec_user", key) == 0 ){
                    strncpy(global_param.exec_user, value, sizeof(global_param.exec_user));
                }
                // exec_group
                else if ( strcasecmp("exec_group", key) == 0 ){
                    strncpy(global_param.exec_group, value, sizeof(global_param.exec_group));
                }
                // auto_detect_bind_ip_address
                else if ( strcasecmp("auto_detect_bind_ip_address", key) == 0 ){
                    strncpy(global_param.auto_detect_bind_ip_address, value, sizeof(global_param.auto_detect_bind_ip_address));
                }
                // server_name
                else if ( strcasecmp("server_name", key) == 0 ){
                    strncpy(global_param.server_name, value, sizeof(global_param.server_name));
                }
#endif
                // server_port
                else if ( strcasecmp("server_port", key) == 0 ){
                    global_param.server_port = atoi(value);
                }
                // document_root
                else if ( strcasecmp("document_root", key) == 0 ){
                    //ALIASのために保存
                    strncpy(global_param.document_org, value, sizeof(global_param.document_org));
                    #ifdef linux
                    strncpy(global_param.document_root, value, sizeof(global_param.document_root));
                    //printf("%s\n", value );
                    #else
                    // ":"が含まれていなければ、相対パスとみなす
                    if (strchr(value, ':') == NULL){
                        //現在のディレクトリを取ってはいけない。現在のディレクトリは不定
                        snprintf(global_param.document_root, sizeof(global_param.document_root), "%s%s", current_dir.c_str(), value );
                        //}
                    }else{
                        strncpy(global_param.document_root, value, sizeof(global_param.document_root));
                    }
                    #endif
                }
                // alias
                else if ( strcasecmp("alias", key) == 0 && count_alias < MAX_COUNT_ALIAS){
                    // valueを' 'で分割
                    sentence_split(value, ' ', work1, work2);
                    strncpy(global_param.alias_key[count_alias], work1, sizeof(global_param.alias_key[0]));
                    strncpy(global_param.alias_rep[count_alias], work2, sizeof(global_param.alias_rep[0]));
                    count_alias++;
                }
                
                // client_language_code
                else if ( strcasecmp("client_language_code", key) == 0 ){
                    if ( strcasecmp(value ,"sjis") == 0 )
                    global_param.client_language_code = CODE_SJIS;
                    else if ( strcasecmp(value ,"euc") == 0 )
                    global_param.client_language_code = CODE_EUC;
                    else if ( strcasecmp(value, "utf8") == 0 )
                    global_param.client_language_code = CODE_UTF8;
                }
                // server_language_code
                else if ( strcasecmp("server_language_code", key) == 0 ){
                    if ( strcasecmp(value ,"auto") == 0 )
                    global_param.server_language_code = CODE_AUTO;
                    else if ( strcasecmp(value ,"sjis") == 0 )
                    global_param.server_language_code = CODE_SJIS;
                    else if ( strcasecmp(value ,"euc") == 0 )
                    global_param.server_language_code = CODE_EUC;
                    else if ( (strcasecmp(value ,"utf8") == 0) || (strcasecmp(value ,"utf-8") == 0) )
                    global_param.server_language_code = CODE_UTF8;
                    else if ( (strcasecmp(value ,"utf16") == 0) || (strcasecmp(value ,"utf-16") == 0) )
                    global_param.server_language_code = CODE_UTF16;
                }
                // flag_use_skin
                else if ( strcasecmp("flag_use_skin", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_use_skin = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_use_skin = FALSE;
                }
                // skin_root
                else if ( strcasecmp("skin_root", key) == 0 ){
                    #ifdef linux
                    strncpy(global_param.skin_root, value, sizeof(global_param.skin_root));
                    #else
                    // ":"が含まれていなければ、相対パスとみなす
                    if (strchr(value, ':') == NULL){
                        //現在のディレクトリを取ってはいけない
                        snprintf(global_param.skin_root, sizeof(global_param.skin_root), "%s%s", current_dir.c_str(), value);
                    }else{
                        strncpy(global_param.skin_root, value, sizeof(global_param.skin_root));
                    }
                    #endif

                    // 最後がDelimiter[0]じゃなかったら、Delimiter[0]を追加
                    if ( global_param.skin_root[strlen(global_param.skin_root)-1 ] != DELIMITER[0] ){
                        strncat(global_param.skin_root, DELIMITER, sizeof(global_param.skin_root));
                    }
                }
                // skin_name
                else if ( strcasecmp("skin_name", key) == 0 ){
                    strncpy(global_param.skin_name, value, sizeof(global_param.skin_name));
                    // 最後が'/'じゃなかったら、'/'を追加
                    if ( global_param.skin_name[strlen(global_param.skin_name)-1 ] != DELIMITER[0] ){
                        strncat(global_param.skin_name, DELIMITER, sizeof(global_param.skin_name));
                        //                                                printf("global_param.skin_name_B '%s' open.\n", global_param.skin_name);
                    }
                }
                //  flag_unknown_extention_file_hide
                else if ( strcasecmp("flag_unknown_extention_file_hide", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_unknown_extention_file_hide = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_unknown_extention_file_hide = FALSE;
                }
                // flag_filename_cut_parenthesis_area
                else if ( strcasecmp("flag_filename_cut_parenthesis_area", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_filename_cut_parenthesis_area = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_filename_cut_parenthesis_area = FALSE;
                }
                // flag_filename_cut_same_directory_name
                else if ( strcasecmp("flag_filename_cut_same_directory_name", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_filename_cut_same_directory_name = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_filename_cut_same_directory_name = FALSE;
                }
                // flag_allplay_filelist_adjust
                else if ( strcasecmp("flag_allplay_filelist_adjust", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_allplay_filelist_adjust = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_allplay_filelist_adjust = FALSE;
                }
                // buffer_size
                else if ( strcasecmp("buffer_size", key) == 0 ){
                    global_param.buffer_size = atoi(value);
                }
                // flag_buffer_send_asap
                else if ( strcasecmp("flag_buffer_send_asap", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_buffer_send_asap = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_buffer_send_asap = FALSE;
                }
                // user_agent_proxy_override
                else if ( strcasecmp("user_agent_proxy_override", key) == 0 ){
                    strncpy(global_param.user_agent_proxy_override, value
                    , sizeof(global_param.user_agent_proxy_override));
                }
                // max_child_count
                //else if ( strcasecmp("max_child_count", key) == 0 ){
                //    global_param.max_child_count = atoi(value);
                //}
                // flag_execute_cgi
                else if ( strcasecmp("flag_execute_cgi", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_execute_cgi = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_execute_cgi = FALSE;
                }
                // debug_cgi_output
                else if ( strcasecmp("debug_cgi_output", key) == 0 ){
                    strncpy(global_param.debug_cgi_output, value, sizeof(global_param.debug_cgi_output));
                }
                // flag_allow_proxy
                else if ( strcasecmp("flag_allow_proxy", key) == 0 ){
                    if (strcasecmp(value ,"true") == 0 )
                    global_param.flag_allow_proxy = TRUE;
                    else if (strcasecmp(value ,"false") == 0 )
                    global_param.flag_allow_proxy = FALSE;
                }
                // access_allow
                if ( strcasecmp("access_allow", key) == 0 )
                {
                    if (count_access_allow < ACCESS_ALLOW_LIST_MAX )
                    {
                        // valueを'/'で分割
                        sentence_split(value, '/', work1, work2);
                        access_allow_list[count_access_allow].flag = TRUE;
                        // adddress文字列を、'.'で分割し、それぞれをatoi()
                        strncat(work1, ".", sizeof(work1) - strlen(work1) ); // 分割処理のため、最後に"."を足しておく
                        for (i=0; i<4; i++ )
                        {
                            sentence_split(work1, '.', work3, work4);
                            access_allow_list[count_access_allow].address[i] = (char)atoi(work3);
                            strncpy(work1, work4, sizeof(work1));
                        }
                        // netmask文字列を、'.'で分割し、それぞれをatoi()
                        strncat(work2, ".", sizeof(work2) - strlen(work2) ); // 分割処理のため、最後に"."を足しておく
                        for (i=0; i<4; i++ )
                        {
                            sentence_split(work2, '.', work3, work4);
                            access_allow_list[count_access_allow].netmask[i] = (char)atoi(work3);
                            strncpy(work2, work4, sizeof(work1));
                        }
                        printf("[%d] address=[%d.%d.%d.%d/%d.%d.%d.%d]\n",count_access_allow,
                        access_allow_list[count_access_allow].address[0],
                        access_allow_list[count_access_allow].address[1],
                        access_allow_list[count_access_allow].address[2],
                        access_allow_list[count_access_allow].address[3],
                        access_allow_list[count_access_allow].netmask[0],
                        access_allow_list[count_access_allow].netmask[1],
                        access_allow_list[count_access_allow].netmask[2],
                        access_allow_list[count_access_allow].netmask[3]    );
                        // addressをnetmaskで and 演算しちゃう。
                        for ( i=0; i<4; i++ )
                        {
                            access_allow_list[count_access_allow].address[i] &= access_allow_list[count_access_allow].netmask[i];
                        }
                        count_access_allow++;
                    }
                }
                // allow_user_agent
                if ( strcasecmp("allow_user_agent", key) == 0 ){
                    if (count_allow_user_agent < ALLOW_USER_AGENT_LIST_MAX ){
                        strncpy(allow_user_agent[count_allow_user_agent].user_agent, value, sizeof(allow_user_agent[count_allow_user_agent].user_agent) );
                        printf("[%d] allow_user_agent='%s'\n", count_allow_user_agent, allow_user_agent[count_allow_user_agent].user_agent);
                        count_allow_user_agent++;
                    }
                }
                // secret_directory_list
                else if ( strcasecmp("secret_directory", key) == 0 ){
                    if (count_secret_directory < SECRET_DIRECTORY_MAX ){
                        // valueを' 'で分割
                        sentence_split(value, ' ', work1, work2);
                        strncpy(secret_directory_list[count_secret_directory].dir_name, work1, sizeof(secret_directory_list[count_secret_directory].dir_name) );
                        secret_directory_list[count_secret_directory].tvid = atoi(work2);
                        printf("[%d] secret_dir='%s', tvid=%d\n", count_secret_directory, secret_directory_list[count_secret_directory].dir_name, secret_directory_list[count_secret_directory].tvid);
                        count_secret_directory++;
                    }
                }
            }
        }
    }
    close( fd );
    return;
}
// *****************************************************
// wizd.conf から１行読み込む
// 読み込んだ文字数がreturnされる。
// 最後まで読んだら、-1が戻る。
// *****************************************************
static int config_file_read_line( int fd, char *line_buf, int line_buf_size)
{
    int read_len;
    int total_read_len;
    char   read_char;
    char *p;
    p = line_buf;
    total_read_len = 0;
    while ( 1 ){
        // 一文字read.
        read_len  = read(fd, &read_char, 1);
        if ( read_len <= 0 ){ // EOF検知
            // 2005/04/19 Maida EDIT START
            // ここでreturnを行うと最後の行が改行されていない場合読み込まれない。
            // そのためbreakを返す。
            return ( -1 );
        }else if ( read_char == '\r' ){
            continue;
        }else if ( read_char == '\n' ){
            break;
        }
        *p = read_char;
        p++;
        total_read_len++;
        if ( total_read_len >= line_buf_size ){
            break;
        }
    }
    *p = '\0';
    return total_read_len;
}
// ****************************************************
// wizd_conf を開く
// 開けなかったら -1
// ****************************************************
static int config_file_open(void)
{
    int     fd;
#ifdef linux
    fd = myopen(DEFAULT_CONF_FILENAME1, O_BINARY|O_RDONLY);
    if ( fd >= 0 ){
        printf("config '%s' open.\n", DEFAULT_CONF_FILENAME1);
        return ( fd );
    }
    fd = myopen(DEFAULT_CONF_FILENAME2, O_BINARY|O_RDONLY);
    if ( fd >= 0 ){
        printf("config '%s' open.\n", DEFAULT_CONF_FILENAME2);
        return ( fd );
    }
    fd = myopen(DEFAULT_CONF_FILENAME3, O_BINARY|O_RDONLY);
    if ( fd >= 0 ){
        printf("config '%s' open.\n", DEFAULT_CONF_FILENAME3);
        return ( fd );
    }
#else
    char work[FILENAME_MAX];
    snprintf( work, FILENAME_MAX, "%s%s%s", current_dir.c_str(), DELIMITER,DEFAULT_CONF_FILENAME1 );
    fd = myopen(work, O_BINARY|O_RDONLY);
    if ( fd >= 0 ){
        //              printf("config '%s' open.\n", DEFAULT_CONF_FILENAME1);
        return ( fd );
    }
#endif
    return ( -1 ) ;
}
// ****************************************************
// 読んだ行を整理する。
// ****************************************************
static void line_buffer_clearance(char *line_buf)
{
    // '#'より後ろを削除。
    cut_after_character(line_buf, '#');
    // '\t'を' 'に置換
    replace_character(line_buf, "\t", " ");
    // ' 'が重なっているところを削除
    duplex_character_to_unique(line_buf, ' ');
    // 頭に' 'がいたら削除。
    cut_first_character(line_buf, ' ');
    // 最後に ' 'がいたら削除。
    cut_character_at_linetail(line_buf, ' ');
    return;
}
//========================================================
// 拡張子を渡すと、Content-type と、file_typeを返す。
//========================================================
void check_file_extension_to_mime_type(const char *file_extension, char *mime_type, int mime_type_size )
{
    int     i;
    strncpy(mime_type, DEFAULT_MIME_TYPE, mime_type_size);
    debug_log_output("file_extension='%s'\n", file_extension);
    // -------------------------------------------
    // ファイルの拡張子比較。Content-type を決定
    // -------------------------------------------
    for (i=0;;i++){
        if ( mime_list[i].mime_name == NULL )
        break;
        if ( strcasecmp(mime_list[i].file_extension, file_extension) == 0 ){
            strncpy(mime_type, mime_list[i].mime_name, mime_type_size);
            break;
        }
    }
    debug_log_output("mime_type='%s'\n", mime_type);
    return;
}
void config_sanity_check()
{
    struct stat sb;
    char cwd[FILENAME_MAX];
    char buf[FILENAME_MAX];
    if (global_param.document_root[0] != '/') {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            debug_log_output("document_root: getcwd(): %s", strerror(errno));
            exit(-1);
        }
        snprintf(buf, sizeof(buf), "%s/%s", cwd, global_param.document_root);
        strncpy(global_param.document_root, buf, sizeof(global_param.document_root));
        debug_log_output("concatenated document_root: '%s'", global_param.document_root);
    }
    if (path_sanitize(global_param.document_root, sizeof(global_param.document_root)) == NULL) {
        debug_log_output("WARNING! weird path has been specified.");
        debug_log_output("falling back to the default document root.");
        strncpy(global_param.document_root, DEFAULT_DOCUMENT_ROOT
        , sizeof(global_param.document_root));
    }
    if (stat(global_param.document_root, &sb) != 0) {
        debug_log_output("document_root: %s: %s", global_param.document_root, strerror(errno));
        exit(-1);
    }
    if (!S_ISDIR(sb.st_mode)) {
        debug_log_output("document_root: %s: is not a directory.", global_param.document_root);
        exit(-1);
    }
    debug_log_output("document_root: '%s'", global_param.document_root);
}
