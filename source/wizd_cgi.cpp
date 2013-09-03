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
    newenv = (char**)calloc(20, sizeof(char*));
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
int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    char *query_string;
    unsigned char *request_uri;
    unsigned char script_filename[FILENAME_MAX];
    char script_name[FILENAME_MAX];
    char *script_exec_name;
    char cwd[FILENAME_MAX];
    int pfd[2];
    int pid;
    int nullfd;
    request_uri = http_recv_info_p->request_uri;
    strncpy(script_name, request_uri, sizeof(script_name));
    if (http_recv_info_p->send_filename[0] != '/') {
        debug_log_output("WARNING: send_filename[0] != '/', send_filanem = '%s'", http_recv_info_p->send_filename);
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            debug_log_output("getcwd() failed. err = %s", strerror(errno));
            return -1;
        }
        snprintf(script_filename, sizeof(script_filename), "%s/%s"
        , cwd, http_recv_info_p->send_filename);
        path_sanitize(script_filename, sizeof(script_filename));
        ////
        strcpy(script_filename,"/usr/bin/php");
    } else {
        strncpy(script_filename, http_recv_info_p->send_filename, sizeof(script_filename));
    }
    query_string = strchr(script_name, '?');
    if (query_string == NULL) {
        query_string = (char*)"";
    } else {
        *query_string++ = '\0';
    }
    script_exec_name = strrchr(script_name, '/');
    if (script_exec_name == NULL) {
        script_exec_name = script_name;
    } else {
        script_exec_name++;
    }
    if (script_exec_name == NULL) {
        debug_log_output("script_exec_name and script_name == NULL");
        return -1;
    }
    if (pipe(pfd) < 0) {
        debug_log_output("pipe() failed!!");
        return -1;
    }
    if ((pid = fork()) < 0) {
        debug_log_output("fork() failed!!");
        close(pfd[0]);
        close(pfd[1]);
        return -1;
    }
    if (pid == 0) { // child
        char server_port[100];
        char remote_port[100];
        struct sockaddr_in saddr;
        socklen_t socklen;
        unsigned char next_cwd[FILENAME_MAX];
        close(pfd[0]);
        clear_environment();
        set_environment("PATH", DEFAULT_PATH);
        set_environment("GATEWAY_INTERFACE", "CGI/1.1");
        set_environment("SERVER_PROTOCOL", "HTTP/1.0");
        socklen = sizeof(saddr);
        getsockname(accept_socket, (struct sockaddr*)&saddr, &socklen);
        set_environment("SERVER_ADDR", inet_ntoa(saddr.sin_addr));
        snprintf(server_port, sizeof(server_port), "%u"
        , ntohs(saddr.sin_port));
        set_environment("SERVER_PORT", server_port);
        set_environment("SERVER_NAME", (char*)global_param.server_name);
        set_environment("SERVER_SOFTWARE", SERVER_NAME);
        set_environment("DOCUMENT_ROOT", (char*)global_param.document_root);
        socklen = sizeof(saddr);
        getpeername(accept_socket, (struct sockaddr*)&saddr, &socklen);
        set_environment("REMOTE_ADDR", inet_ntoa(saddr.sin_addr));
        snprintf(remote_port, sizeof(remote_port), "%u"
        , ntohs(saddr.sin_port));
        set_environment("REMOTE_PORT", remote_port);
        set_environment("REQUEST_METHOD", "GET");
        set_environment("REQUEST_URI", (char*)request_uri);
        set_environment("SCRIPT_FILENAME", (char*)script_filename);
        set_environment("SCRIPT_NAME", script_name);
        set_environment("QUERY_STRING", query_string);
        if (http_recv_info_p->recv_host[0]) {
            set_environment("HTTP_HOST", (char*)http_recv_info_p->recv_host);
        }
        if (http_recv_info_p->user_agent[0]) {
            set_environment("HTTP_USER_AGENT", (char*)http_recv_info_p->user_agent);
        }
        if (http_recv_info_p->recv_range[0]) {
            set_environment("HTTP_RANGE", (char*)http_recv_info_p->recv_range);
        }
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        nullfd = open("/dev/null", O_RDONLY);
        if (nullfd >= 0) {
            dup2(nullfd, STDIN_FILENO);
            close(nullfd);
        } else {
            close(STDIN_FILENO);
        }
        nullfd = open(global_param.debug_cgi_output, O_WRONLY);
        if (nullfd >= 0) {
            dup2(nullfd, STDERR_FILENO);
            close(nullfd);
        } else {
            close(STDERR_FILENO);
        }
        printf("%s", HTTP_OK);
        printf("%s", HTTP_CONNECTION);
        printf(HTTP_SERVER_NAME, SERVER_NAME);
        strncpy(next_cwd, script_filename, sizeof(next_cwd));
        cut_after_last_character(next_cwd, '/');
        if (chdir((char*)next_cwd) != 0) {
            debug_log_output("chdir failed. err = %s", strerror(errno));
        }
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
    // parent
    close(pfd[1]);
    copy_descriptors(pfd[0], accept_socket, 0);
    return 0;
}
