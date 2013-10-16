#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "wizd.h"
static int clear_environment()
{
    extern char **environ;
    char **newenv;
    newenv = (char**)calloc(40, sizeof(char*));
    if (newenv == NULL) {
        debug_log_output("calloc failed. (err = %s)", strerror(errno));
        return -1;
    }
    newenv[0] = NULL;
    environ = newenv;
    return 0;
}
static int set_environment(const char *name, const char *value)
{
    debug_log_output("set_environment: '%s' = '%s'", name, value);
    return setenv(name, value, 1);
}
//////////////////////////////////////////////////////////////////////////
int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    char *query_string;
    unsigned char *request_uri;
    unsigned char script_filename[FILENAME_MAX]={0};
    char script_name[FILENAME_MAX]={0};
    char *script_exec_name;
    char cwd[FILENAME_MAX]={0};
    char ext[4];
    int pfd[2]={0};
    int pid;
    int nullfd;
    request_uri = http_recv_info_p->request_uri;
    strncpy(script_name, request_uri, sizeof(script_name));
    if (http_recv_info_p->send_filename[0] != '/') {
        debug_log_output("WARNING: send_filename[0] != '/', send_filanem = '%s'", http_recv_info_p->send_filename);
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            close( accept_socket );
            debug_log_output("getcwd() failed. err = %s", strerror(errno));
            return -1;
        }
        snprintf(script_filename, sizeof(script_filename), "%s/%s", cwd, http_recv_info_p->send_filename);
        path_sanitize(script_filename, sizeof(script_filename));
    } else {
        strncpy(script_filename, http_recv_info_p->send_filename, sizeof(script_filename));
    }
    //QueryString取得
    query_string = strchr(script_name, '?');
    if (query_string == NULL) {
        query_string = (char*)"";
    } else {
        *query_string++ = '\0';
    }
    //実行CGI名のみを取得
    script_exec_name = strrchr(script_name, '/');
    if (script_exec_name == NULL) {
        script_exec_name = script_name;
    } else {
        script_exec_name++;
    }
    if (script_exec_name == NULL) {
        close( accept_socket );
        debug_log_output("script_exec_name and script_name == NULL");
        return -1;
    }
    //拡張子の取得
    filename_to_extension((unsigned char*)script_exec_name,(unsigned char*)ext,4);
    // パイプの作成 accept_sockクローズ必要
    if (pipe(pfd) < 0) {
        close( accept_socket );
        debug_log_output("pipe() failed!!");
        return -1;
    }
    // fork実行 accept_socketクローズ必要
    if ((pid = fork()) < 0) {
        close( accept_socket );
        debug_log_output("fork() failed!!");
        close(pfd[0]);
        close(pfd[1]);
        return -1;
    }

    //子プロセス側
    if (pid == 0) { // child
        char server_port[100]={0};
        char remote_port[100]={0};
        struct sockaddr_in saddr;
        socklen_t socklen;
        unsigned char next_cwd[FILENAME_MAX]={0};
        //close(pfd[0]);//read 側をクローズ
        clear_environment();

//fastcgi_param  DOCUMENT_URI       $document_uri;
        if (http_recv_info_p->content_type[0]) {
            set_environment("CONTENT_TYPE", (char*)http_recv_info_p->content_type);
        }
        if (http_recv_info_p->content_length[0]) {
            set_environment("CONTENT_LENGTH", (char*)http_recv_info_p->content_length);
        }
        if (http_recv_info_p->recv_host[0]) {
            set_environment("HTTP_HOST", (char*)http_recv_info_p->recv_host);
        }
        if (http_recv_info_p->user_agent[0]) {
            set_environment("HTTP_USER_AGENT", (char*)http_recv_info_p->user_agent);
        }
//        //SERVER SIGNATURE
        set_environment("PATH", DEFAULT_PATH);
        set_environment("SERVER_SOFTWARE", SERVER_NAME);
        set_environment("SERVER_NAME", (char*)global_param.server_name);
        socklen = sizeof(saddr);
        getsockname(accept_socket, (struct sockaddr*)&saddr, &socklen);
        set_environment("SERVER_ADDR", inet_ntoa(saddr.sin_addr));
        snprintf(server_port, sizeof(server_port), "%u", ntohs(saddr.sin_port));
        set_environment("SERVER_PORT", server_port);
        set_environment("DOCUMENT_ROOT", (char*)global_param.document_root);

        socklen = sizeof(saddr);
        getpeername(accept_socket, (struct sockaddr*)&saddr, &socklen);
        set_environment("REMOTE_ADDR", inet_ntoa(saddr.sin_addr));
        snprintf(remote_port, sizeof(remote_port), "%u", ntohs(saddr.sin_port));
        set_environment("REMOTE_PORT", remote_port);

//	//SERVER ADMIN
        set_environment("SCRIPT_FILENAME", (char*)script_filename);
        set_environment("GATEWAY_INTERFACE", "CGI/1.1");
        set_environment("SERVER_PROTOCOL", "HTTP/1.0");
        set_environment("REQUEST_METHOD", http_recv_info_p->isGet == 1 ? "GET" : ( http_recv_info_p->isGet == 2 ? "HEAD" : "POST" ) );
        set_environment("QUERY_STRING", query_string);
        set_environment("REQUEST_URI", (char*)request_uri);
        set_environment("SCRIPT_NAME", script_name);
        //PHP-CGIのセキュリティを回避する環境変数。
	set_environment("REDIRECT_STATUS","1");
        set_environment("HTTP_REDIRECT_STATUS", "1" );
        //標準出力を書き込み側に設定
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);

        //読み込み側を/dev/nullに
        //nullfd = open("/dev/null", O_RDONLY);
        //if (nullfd >= 0) {
        //    dup2(nullfd, STDIN_FILENO);
        //    close(nullfd);
        //} else {
        //    close(STDIN_FILENO);
        //}
        dup2(accept_socket, STDIN_FILENO);
        close(accept_socket);
        close(pfd[0]);

        //標準エラーをファイルに出力
        //debug_log_output("DEBUG=%s",global_param.debug_cgi_output);
        nullfd = open(global_param.debug_cgi_output, O_WRONLY);
        //if (nullfd >= 0) {
        //    dup2(nullfd, STDERR_FILENO);
        //    close(nullfd);
        //} else {
        //    close(STDERR_FILENO);
        //}
        //ヘッダを出力
        printf("%s", HTTP_OK);
        printf("%s", HTTP_CONNECTION);
        printf(HTTP_SERVER_NAME, SERVER_NAME);
        //指定フォルダにcd
        strncpy(next_cwd, script_filename, sizeof(next_cwd));
        cut_after_last_character(next_cwd, '/');
        if (chdir((char*)next_cwd) != 0) {
            debug_log_output("chdir failed. err = %s", strerror(errno));
        }
        //実行
        if( strcasecmp(ext,"php") == 0 ){
            if (execl("/usr/bin/php-cgi","php-cgi", (char*)script_filename, NULL) < 0) {
                debug_log_output("CGI EXEC ERROR. "
                "/usr/bin/php script = '%s', argv[0] = '%s', err = %s"
                , script_filename
                , script_exec_name
                , strerror(errno)
                );
                printf("\nPHP EXEC ERROR");
            }
            exit(0);
        }else{
            if (execl((char*)script_filename, script_exec_name, NULL) < 0) {
                debug_log_output("CGI EXEC ERROR. "
                "script = '%s', argv[0] = '%s', err = %s"
                , script_filename
                , script_exec_name
                , strerror(errno)
                );
                printf("\nCGI EXEC ERROR");
            }
            exit(0);
        }
    }
    // parent
    close(pfd[1]);//書き込み側をクローズ
    //ヘッダ出力
    set_blocking_mode(accept_socket, 0); /* non_blocking */
    copy_descriptors(pfd[0], accept_socket, 0, NULL,0);
    return 0;
}
