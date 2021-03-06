#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
//#include <unistd.h>
#include <sys/types.h>
#ifdef linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <io.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include "wizd.h"
#include "wizd_tools.h"
#include "const.h"
#define bzero(thing,sz) memset(thing,0,sz)
extern int line_receive(int accept_socket, char *line_buf_p, int line_max);

//extern int http_uri_to_scplaylist_create(int accept_socket, char *uri_string);
#define MAX_LINE 100 /* 記憶する、HTTPヘッダの最大行数 */
#define LINE_BUF_SIZE 4096 /* 行バッファ */

int http_proxy_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    const char *HTTP_RECV_CONTENT_TYPE = "Content-type: text/html";
    const char *HTTP_RECV_CONTENT_LENGTH = "Content-Length: ";
    const char *HTTP_RECV_LOCATION = "Location: ";
    char *p_target_host_name;
    char *p_uri_string;
    char send_http_header_buf[2048];
    char line_buf[MAX_LINE][LINE_BUF_SIZE + 5];
    char proxy_pre_string[2048];
    char base_url[2048];
    char *p_url;
    char *p;
    char *p_auth = NULL;
    int port = 80;
    int sock;
    off_t content_length = 0;
    int len = 0;
    int line = 0;
    int i;
    int content_is_html = 0;
    p_uri_string = http_recv_info_p->request_uri;
    if (!strncmp(p_uri_string, "/-.-playlist.pls?", 17)) {
        char buff[FILENAME_MAX];
        if (p_uri_string[17] == '/') {
            snprintf(buff, sizeof(buff), "http://%s%s"
            , http_recv_info_p->recv_host, p_uri_string+17);
        } else {
            strncpy(buff, p_uri_string+17, sizeof(buff));
        }
        replace_character((char*)buff, ".pls", "");
        return 0;//http_uri_to_scplaylist_create(accept_socket, (char*)buff);
    }
    if (strncmp(p_uri_string, "/-.-http://", 11)) {
        return -1;
    }
    if (!global_param.flag_allow_proxy) {
        return -1;
    }
    strncpy(base_url, p_uri_string, 2048);
    p = strrchr(base_url, '/');
    p[1] = '\0';
    p_target_host_name = p_uri_string + 11;
    p_url = strchr(p_target_host_name, '/');
    if (p_url == NULL) return -1;
    *p_url++ = '\0';
    strncpy(proxy_pre_string, p_uri_string, 2048);
    p = strchr(p_target_host_name, '@');
    if (p != NULL) {
        // there is a user name.
        p_auth = p_target_host_name;
        p_target_host_name = p + 1;
        *p = '\0';
    }
    p = strchr(p_target_host_name, ':');
    if (p != NULL) {
        port = atoi(p+1);
        *p = '\0';
    }
    debug_log_output("proxy:target_host_name: %s", p_target_host_name);
    debug_log_output("proxy:authenticate: %s", p_auth ? p_auth : "NULL");
    debug_log_output("proxy:url: %s", p_url);
    debug_log_output("proxy:prestring: %s", proxy_pre_string);
    debug_log_output("proxy:base_url: %s", base_url);
    debug_log_output("proxy:port: %d", port);
    p = send_http_header_buf;
    if( http_recv_info_p->isGet == 1 ){
        p += sprintf((char*)p, "GET /%s HTTP/1.0\r\n", p_url);
    }else if ( http_recv_info_p->isGet == 2 ){
        p += sprintf((char*)p, "HEAD /%s HTTP/1.0\r\n", p_url);
    }
    p += sprintf((char*)p, "Host: %s:%u\r\n", p_target_host_name, port);
    if (global_param.user_agent_proxy_override[0]) {
        p += sprintf((char*)p, "User-agent: %s\r\n", global_param.user_agent_proxy_override);
    } else {
        if (http_recv_info_p->user_agent[0]) {
            p += sprintf((char*)p, "User-agent: %s\r\n", http_recv_info_p->user_agent);
        } else {
            p += sprintf((char*)p, "User-agent: %s\r\n", SERVER_NAME);
        }
    }
    p += sprintf(p, "Accept: */*\r\n");
    p += sprintf(p, "Connection: close\r\n");
    if (http_recv_info_p->range_start_pos) {
        p += sprintf((char*)p, "Range: bytes=");
        p += sprintf((char*)p, "%zu-", http_recv_info_p->range_start_pos);
        if (http_recv_info_p->range_end_pos) {
            p += sprintf((char*)p, "%zu", http_recv_info_p->range_end_pos);
        }
        p += sprintf(p, "\r\n");
    }
    if (p_auth != NULL) {
        p += sprintf((char*)p, "Authorization: Basic %s\r\n", base64((unsigned char*)p_auth));
    }
    p += sprintf(p, "\r\n");
    sock = sock_connect((char*)p_target_host_name, port);
    if (sock < 0) {
        debug_log_output("error: %s", strerror(errno));
        debug_log_output("sock: %d", sock);
        return -1;
    }
    set_nonblocking_mode(sock, 0); /* blocking mode */
    set_nonblocking_mode(accept_socket, 0); /* blocking mode */
    write(sock, send_http_header_buf, strlen(send_http_header_buf));
    debug_log_output("================= send to proxy\n");
    debug_log_output("%s", send_http_header_buf);
    debug_log_output("=================\n");
    
    //TODO ここが４０４や４００の可能性あり
    for (line = 0; line < MAX_LINE; line ++) {
        char work_buf[LINE_BUF_SIZE + 10];
        memset(line_buf[line], 0, LINE_BUF_SIZE + 5);
        len = line_receive(sock, line_buf[line], LINE_BUF_SIZE + 1);
        if (len < 0) break;
        debug_log_output("recv html: '%s' len = %d", ((len)?(char*)line_buf[line]:""), len);
        line_buf[line][len++] = '\r';
        line_buf[line][len++] = '\n';
        line_buf[line][len] = '\0';
        if (!strncasecmp(line_buf[line], HTTP_RECV_CONTENT_TYPE, strlen(HTTP_RECV_CONTENT_TYPE))) {
            int n;
            content_is_html = 1;
            n = strlen(HTTP_RECV_CONTENT_TYPE);
            line_buf[line][n++] = '\r';
            line_buf[line][n++] = '\n';
            line_buf[line][n] = '\0';
        }else if (!strncasecmp(line_buf[line], HTTP_RECV_CONTENT_LENGTH, strlen(HTTP_RECV_CONTENT_LENGTH))) {
            content_length = strtoull((char*)(line_buf[line] + strlen(HTTP_RECV_CONTENT_LENGTH)), NULL, 0);
        }else if (!strncasecmp(line_buf[line], HTTP_RECV_LOCATION, strlen(HTTP_RECV_LOCATION))) {
            strcpy(work_buf, line_buf[line]);
            sprintf((char*)line_buf[line], "Location: /-.-%s", work_buf + strlen(HTTP_RECV_LOCATION));
        }else if ( strstr( line_buf[line],"charset") ){
            if ( strstr( line_buf[line], "EUC-JP" )){
                replace_character( (char*)line_buf[line], "EUC-JP", "UTF-8");
            }else if ( strstr ( line_buf[line], "Shift_JIS" )) {
                replace_character( (char*)line_buf[line], "Shift_JIS", "UTF-8");
            }
        }
        if (len <= 2) {
            line++;
            break;
        }
    }
    if (len < 0 ) {
        debug_log_output("CLOSED-C");
        close(sock);
        return -1;
    }
    if (content_is_html) {
        char *p, *q;
        char *new_p, *r;
        char work_buf[LINE_BUF_SIZE + 10];
        char work_buf2[LINE_BUF_SIZE * 2];
        char *link_pattern = (char*)"<A HREF=";
        int flag_conv_html_code = 1;
        for (i=0; i<line; i++) {
            //長さは不定なので送らない
            if (!strncasecmp(line_buf[i], HTTP_RECV_CONTENT_LENGTH, strlen(HTTP_RECV_CONTENT_LENGTH))) continue;
            //*q = '\0';
            p = work_buf;
            q = work_buf2;
            if (flag_conv_html_code) {
                convert_language_code(line_buf[i], work_buf, LINE_BUF_SIZE, CODE_AUTO, global_param.client_language_code);
            }
            //strcpy( work_buf, line_buf[i]);
            //            write(accept_socket, line_buf[i], strlen(line_buf[i]));
            //            debug_log_output("sent html: '%s' len = %d", line_buf[i], strlen(line_buf[i]));
            write(accept_socket, work_buf, strlen(line_buf[i]));
            debug_log_output("sent html: '%s' len = %d", work_buf, strlen(work_buf));
        }
        debug_log_output("sent header");
        //write(accept_socket, "debug:--\n", strlen("debug:--\n"));
        while (1) {
            char rep_str[1024];
            memset(work_buf, 0, LINE_BUF_SIZE);
            len = line_receive(sock, work_buf, LINE_BUF_SIZE);
            if (len < 0) break;
            debug_log_output("recv html: '%s' len = %d", work_buf, len);
            work_buf[len++] = '\r';
            work_buf[len++] = '\n';
            work_buf[len] = '\0';
            if (my_strcasestr(work_buf, "Content-Type") != NULL) {
                // Content-Type があったら
                // 漢字コードの変換をやめる
                //    flag_conv_html_code = 0;
                if ( strstr( work_buf, "EUC-JP" )){
                    replace_character( (char*)work_buf, "EUC-JP", "UTF-8");
                }else if ( strstr ( work_buf, "Shift_JIS" )) {
                    replace_character( (char*)work_buf, "Shift_JIS", "UTF-8");
                }
            }
            p = work_buf;
            q = work_buf2;
            memset(q, 0, sizeof(work_buf2));
            /*
            * HTML中に <A HREF="..."> があったら、プロクシを経由するように
            * たとえば <A HREF="/-.-http://www.yahoo.co.jp/"> のように変換する
            * もし、ファイルがストリーム可能そうであれば、VOD="0" も追加する。
            *
            * 問題点:
            *   タグの途中に改行があると失敗するだろう.
            *   面倒なのでたいした置換はしていない
            *   惰性で書いたので汚い。だれか修正して。
            */
            link_pattern = (char*)"<A HREF=";
            while ((new_p = my_strcasestr(p, link_pattern)) != NULL) {
                int l = new_p - p + strlen(link_pattern);
                char *tmp;
                MIME_LIST_T *mlt = NULL;
                strncpy(q, p, l);
                q += l;
                p += l; /* i.e., p = new_p + strlen(link_pattern); */
                r = strchr(p, '>');
                if (r == NULL) continue;
                *r = '\0';
                if (*p == '"') *q++ = *p++;
                if ((tmp = strchr(p, '"')) != NULL || (tmp = strchr(p, ' ')) != NULL) {
                    mlt = mime_list;
                    while (mlt->mime_name != NULL) {
                        if (*(tmp - strlen(mlt->file_extension) - 1) == '.'
                        && !strncasecmp(tmp - strlen(mlt->file_extension), mlt->file_extension, strlen(mlt->file_extension))) {
                            break;
                        }
                        mlt++;
                    }
                }
                //if (flag_pc && mlt != NULL && mlt->stream_type == TYPE_STREAM) {
                //    q += sprintf((char*)q, "/-.-playlist.pls?http://%s", http_recv_info_p->recv_host);
                //}
                if (*p == '/') {
                    q += sprintf(q, "%s%s", proxy_pre_string, p);
                } else if (!strncmp(p, "http://", 7)) {
                    q += sprintf(q, "/-.-%s", p);
                } else {
                    q += sprintf((char*)q, "%s%s", base_url, p);
                    //q += sprintf((char*)q, "%s", p);
                }
                if (mlt != NULL && mlt->stream_type == TYPE_STREAM) {
                    q += sprintf(q, " vod=\"0\"");
                }
                *q++ = '>';
                p = r + 1;
            }
            while (*p) *q++ = *p++;
            *q = '\0';
            /*
            * HTML中に SRC="..." があったら、プロクシを経由するように変換する
            *
            * 問題点:
            *   タグの途中に改行があると失敗するだろう.
            *   変数使いまわしたので、融通が効かない。
            *   だれか修正して。
            */
            p = work_buf2;
            q = work_buf;
            memset(q, 0, sizeof(work_buf));
            link_pattern = (char*)"SRC=";
            while ((new_p = my_strcasestr(p, link_pattern)) != NULL) {
                int l = new_p - p + strlen(link_pattern);
                strncpy(q, p, l);
                q += l;
                p += l; /* i.e., p = new_p + strlen(link_pattern); */
                if (*p == '"') *q++ = *p++;
                if (*p == '/') {
                    q += sprintf(q, "%s", proxy_pre_string);
                } else if (!strncmp(p, "http://", 7)) {
                    q += sprintf(q, "/-.-");
                } else {
                    q += sprintf(q, "%s", base_url);
                    //q += sprintf(q, "%s", p);
                }
            }
            while (*p) *q++ = *p++;
            *q = '\0';
            p = work_buf;
            q = work_buf2;
            if (flag_conv_html_code) {
                convert_language_code(p, q, LINE_BUF_SIZE, CODE_AUTO, global_param.client_language_code);
                //strcpy( q,p);
                p = q;
            }
            snprintf(rep_str, sizeof(rep_str), "|http://%s/-.-http://"
            , http_recv_info_p->recv_host);
            replace_character((char*)p, "|http://", rep_str);
            write(accept_socket, p, strlen(p));
            debug_log_output("sent html: %s", p);
        }
    } else {
        for (i=0; i<line; i++) {
            write(accept_socket, line_buf[i], strlen(line_buf[i]));
        }
        if( http_recv_info_p->isGet == 1 ){
            return ( copy_descriptors(sock, accept_socket, (size_t)content_length) );
            //NULL ,
            //(char*)http_recv_info_p->recv_uri,
           // http_recv_info_p->range_start_pos));
        }
    }
    sClose(sock);
    return 0;
}
