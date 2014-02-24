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
#include "TinyJS.h"
#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"
#include <assert.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <stdio.h>
#ifdef use_thread
#include <pthread.h>
void jss(int accpet_socket, HTTP_RECV_INFO *http_recv_info_p,unsigned char* script_filename,unsigned char* request_uri,char* query_string,char* script_exec_name,char* script_name);
void php(int accpet_socket, HTTP_RECV_INFO *http_recv_info_p,unsigned char* script_filename,unsigned char* request_uri,char* query_string,char* script_exec_name,char* script_name);
#endif

static int set_environment(const char *name, const char *value)
{
    //debug_log_output("set_environment: '%s' = '%s'", name, value);
    return setenv(name, value, 1);
}
//////////////////////////////////////////////////////////////////////////
int http_cgi_response(int accept_socket, HTTP_RECV_INFO *http_recv_info_p)
{
    debug_log_output( "CGI!!! %s", http_recv_info_p->request_uri);
    char *query_string;
    unsigned char *request_uri;
    unsigned char script_filename[FILENAME_MAX]={0};
    char script_name[FILENAME_MAX]={0};
    char *script_exec_name;
    char cwd[FILENAME_MAX]={0};
    char ext[4];
    //request_rui,script_name,query_string,script_exec_nameの生成
    request_uri = http_recv_info_p->request_uri;
    strncpy(script_name, request_uri, sizeof(script_name));
    if (http_recv_info_p->send_filename[0] != '/') {
        debug_log_output("WARNING: send_filename[0] != '/', send_filanem = '%s'", http_recv_info_p->send_filename);
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            close( accept_socket );
            debug_log_output("getcwd() failed. err = %s", strerror(errno));
            return NULL;

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
        return NULL;
    }
    //拡張子の取得
    filename_to_extension((unsigned char*)script_exec_name,(unsigned char*)ext,4);

#ifdef use_thread
    if( strcasecmp(ext,"jss") == 0 ){
        //スレッドIDについて出力先を保存
        jss(accept_socket,http_recv_info_p,script_filename,request_uri,query_string,script_exec_name,script_name);
    }else if ( strcasecmp(ext,"php") == 0 ){
        php(accept_socket,http_recv_info_p,script_filename,request_uri,query_string,script_exec_name,script_name);
    }else{
        //cgi((void*)http_recv_info_p,unsigned char* script_filename,unsigned char* request_uri,char* query_string,char* script_exec_name);
    }
    close(accept_socket);
    return 0;
}
/////////////////////////////////////////////////////////////////////////
void jss(int accept_socket, HTTP_RECV_INFO* http_recv_info_p,unsigned char* script_filename,unsigned char* request_uri,char* query_string,char* script_exec_name,char* script_name)
{
    //実行
    char server_port[100]={0};
    char remote_port[100]={0};
    struct sockaddr_in saddr;
    socklen_t socklen;
    unsigned char next_cwd[FILENAME_MAX]={0};

    //指定フォルダにcd
    strncpy(next_cwd, script_filename, sizeof(next_cwd));
    cut_after_last_character(next_cwd, '/');
    struct stat results;
    if (!stat ((char*)script_filename, &results) == 0)
    {
        debug_log_output ("Cannot stat file! '%s'\n", script_filename);
        return;
    }
    //指定フォルダにCD
    if (chdir((char*)next_cwd) != 0) {
        debug_log_output("chdir failed. err = %s", strerror(errno));
    }
    int size = results.st_size;
    FILE *file = fopen ((char*)script_filename, "rb");

    /* if we open as text, the number of bytes read may be > the size we read */
    if (!file)
    {
        debug_log_output("Unable to open file '%s'", script_filename );
        return;
    }
    char *buffer = new char[size + 3];
    long actualRead = fread (buffer+2, 1, size, file);
    buffer[actualRead+2] = 0;
    buffer[size+2] = 0;
    buffer[0] = '?';
    buffer[1] = '>';
    fclose (file);

    CTinyJS  *s = new CTinyJS(accept_socket);
    registerFunctions (&*s);
    registerMathFunctions (&*s);
    //insert Environment to js
    wString script1;
    wString script2;
    wString script3;
    wString script4;
    s->root->addChild ("_SERVER", new CScriptVar (TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT));
    s->root->addChild ("_GET", new CScriptVar (TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT));
    if( http_recv_info_p->isGet== 3 ){
        s->root->addChild ("_POST", new CScriptVar (TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT));
    }

    try
    {
        //環境変数をJSに展開
        //fastcgi_param  DOCUMENT_URI       $document_uri;
        if (http_recv_info_p->content_type[0]) {
            script1.sprintf( "var _SERVER.CONTENT_TYPE=\"%s\";", (char*)http_recv_info_p->content_type);	s->execute(script1);
        }
        if (http_recv_info_p->content_length[0]) {
            script1.sprintf( "var _SERVER.CONTENT_LENGTH=\"%s\";", (char*)http_recv_info_p->content_length);s->execute(script1);
        }
        if (http_recv_info_p->recv_host[0]) {
            script1.sprintf( "var _SERVER.HTTP_HOST=\"%s\";", (char*)http_recv_info_p->recv_host);		s->execute(script1);
        }
        if (http_recv_info_p->user_agent[0]) {
            script1.sprintf( "var _SERVER.HTTP_USER_AGENT=\"%s\";", (char*)http_recv_info_p->user_agent);	s->execute(script1);
        }
        //SERVER SIGNATURE
        script1.sprintf( "var _SERVER.PATH=\"%s\";", DEFAULT_PATH);					s->execute(script1);
        script1.sprintf( "var _SERVER.SERVER_SOFTWARE=\"%s\";", SERVER_NAME);			s->execute(script1);
        script1.sprintf( "var _SERVER.SERVER_NAME=\"%s\";", (char*)global_param.server_name);	s->execute(script1);
        //SERVER PORT
        socklen = sizeof(saddr);
        getsockname(accept_socket, (struct sockaddr*)&saddr, &socklen);
        script1.sprintf( "var _SERVER.SERVER_ADDR=\"%s\";", inet_ntoa(saddr.sin_addr));		s->execute(script1);
        snprintf(server_port, sizeof(server_port), "%u", ntohs(saddr.sin_port));
        script1.sprintf( "var _SERVER.SERVER_PORT=\"%s\";", server_port);				s->execute(script1);
        script1.sprintf( "var _SERVER.DOCUMENT_ROOT=\"%s\";", (char*)global_param.document_root);	s->execute(script1);
        //REMOTE ADDR
        socklen = sizeof(saddr);
        getpeername(accept_socket, (struct sockaddr*)&saddr, &socklen);
        script1.sprintf( "var _SERVER.REMOTE_ADDR=\"%s\";", inet_ntoa(saddr.sin_addr));		s->execute(script1);
        //REMOTE PORT
        snprintf(remote_port, sizeof(remote_port), "%u", ntohs(saddr.sin_port));
        script1.sprintf( "var _SERVER.REMOTE_PORT=\"%s\";", remote_port);				s->execute(script1);
        script1.sprintf( "var _SERVER.SCRIPT_FILENAME=\"%s\";",(char*)script_filename);		s->execute(script1);
        script1.sprintf( "var _SERVER.GATEWAY_INTERFACE=\"%s\";", "CGI/1.1");			s->execute(script1);
        script1.sprintf( "var _SERVER.SERVER_PROTOCOL=\"%s\";", "HTTP/1.0");			s->execute(script1);
        script1.sprintf( "var _SERVER.REQUEST_METHOD=\"%s\";", http_recv_info_p->isGet == 1 ? "GET" : ( http_recv_info_p->isGet == 2 ? "HEAD" : "POST" ) );
        s->execute(script1);

        script1.sprintf( "var _SERVER.QUERY_STRING=\"%s\";", query_string);				s->execute(script1);
        script1.sprintf( "var _SERVER.REQUEST_URI=\"%s\";", (char*)request_uri);			s->execute(script1);
        script1.sprintf( "var _SERVER.SCRIPT_NAME=\"%s\";", script_name);				s->execute(script1);
        //query stringをgetに展開
        script3 = query_string;
        while( script3.length() ){
            int pos = script3.Pos("&");
            if( pos > 1 ){
                script1 = script3.substr(0,pos);
                int pos2 = script1.Pos("=");
                script2 = "var _GET."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                script3 = script3.substr(pos+1);
            }else{
                script1 = script3;
                int pos2 = script1.Pos("=");
                script2 = "var _GET."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                script3 = "";
            }
            //debug_log_output( script2.c_str() );
            s->execute( script2 );
        }
        if( http_recv_info_p->isGet== 3 ){
            char buf[1024];
            int num;
            int contentsize = atoi(http_recv_info_p->content_length);
            int readsize;
            //指定されたサイズまで読む    
            while(contentsize){
                if( contentsize<1024){
                    readsize=contentsize;
                }else{
                    readsize=1024;
                }
                num = read( accept_socket, buf, readsize);
                wString* tmp;
                if( num <= 0 ){
                    break;
                }else{
                    contentsize -= num;
                }
                script4 += buf;
            }
            //script4.SaveToFile( "/tmp/aaa");
            //debug_log_output("POST %s", script4.c_str() );
            while( script4.length() ){
                int pos = script4.Pos("&");
                if( pos > 1 ){
                    script1 = script4.substr(0,pos);
                    int pos2 = script1.Pos("=");
                    script2 = "var _POST."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                    script4 = script4.substr(pos+1);
                }else{
                    script1 = script4;
                    int pos2 = script1.Pos("=");
                    script2 = "var _POST."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                    script4 = "";
                }
                //debug_log_output( script2.c_str() );
                s->execute( script2 );
            }
            //debug_log_output( "bbb");
        }
        s->execute (buffer);
    }
    catch (CScriptException * e)
    {
        //debug_log_output("SCRIPT ERROR: %s\n", e->text.c_str ());
        printf("SCRIPT ERROR: %s\n", e->text.c_str ());
        delete s;
        delete[] buffer;
        return;
    }
    delete s;
    delete[] buffer;
    debug_log_output("ServerSide JavaScript end");
    return;
}
void php(int accept_socket, HTTP_RECV_INFO* http_recv_info_p,unsigned char* script_filename,unsigned char* request_uri,char* query_string,char* script_exec_name,char* script_name)
{
    int pid;
    // fork実行 accept_socketクローズ必要
    if ((pid = fork()) < 0) {
        debug_log_output("fork() failed!!");
        return;
    }
    //子プロセス側
    if (pid == 0) { // child
        char server_port[100]={0};
        char remote_port[100]={0};
        struct sockaddr_in saddr;
        socklen_t socklen;
        unsigned char next_cwd[FILENAME_MAX]={0};
        clearenv();

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
        //SERVER SIGNATURE
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

        //SERVER ADMIN
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
        int socket = dup(accept_socket);
        //標準出力を書き込み側に設定
        close( STDOUT_FILENO );
        dup2(accept_socket, STDOUT_FILENO);
        close( STDIN_FILENO);
        dup2(accept_socket, STDIN_FILENO);
        close(accept_socket);

        //指定フォルダにcd
        strncpy(next_cwd, script_filename, sizeof(next_cwd));
        cut_after_last_character(next_cwd, '/');
        if (chdir((char*)next_cwd) != 0) {
            debug_log_output("chdir failed. err = %s", strerror(errno));
        }
        //if( strcasecmp(ext,"php") == 0 ){
            wString str;
            str.headerInit(0,0);
            str.headerPrint(STDOUT_FILENO,0);
            //wString::wStringEnd();
            if (execl("/usr/bin/php-cgi","php-cgi",(char*)script_filename, NULL) < 0) {
                debug_log_output("CGI EXEC ERROR. "
                "/usr/bin/php script = '%s', argv[0] = '%s', err = %s"
                , script_filename
                , script_exec_name
                , strerror(errno)
                );
                printf("\nPHP EXEC ERROR");
            }
        //}else{
        //    wString str;
        //    exit( 0 );
        close(socket);
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        exit(0);
    }
    return;
}
#else
    int pid;
    // fork実行 accept_socketクローズ必要
    if ((pid = fork()) < 0) {
        close( accept_socket );
        debug_log_output("fork() failed!!");
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
        //clear_environment();
        clearenv();
        set_nonblocking_mode(accept_socket,0);//BLOCKING MODE

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

//      //SERVER ADMIN
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
        int socket;
        socket = dup(accept_socket);
        //標準出力を書き込み側に設定
        close( STDOUT_FILENO );
        dup2(accept_socket, STDOUT_FILENO);
        close( STDIN_FILENO);
        dup2(accept_socket, STDIN_FILENO);
        close(accept_socket);

        //標準エラーをファイルに出力
        debug_log_output("DEBUG=%s",global_param.debug_cgi_output);
        int nullfd = open(global_param.debug_cgi_output, O_WRONLY);
        if (nullfd >= 0) {
            dup2(nullfd, STDERR_FILENO);
            close(nullfd);
        } else {
            close(STDERR_FILENO);
        }
        //ヘッダを出力
        //指定フォルダにcd
        strncpy(next_cwd, script_filename, sizeof(next_cwd));
        cut_after_last_character(next_cwd, '/');
        if (chdir((char*)next_cwd) != 0) {
            debug_log_output("chdir failed. err = %s", strerror(errno));
        }

        //実行
        if( strcasecmp(ext,"jss") == 0 ){
            struct stat results;
            if (!stat ((char*)script_filename, &results) == 0)
            {
                debug_log_output ("Cannot stat file! '%s'\n", script_filename);
                close(socket);
                exit(1);
            }
            int size = results.st_size;
            FILE *file = fopen ((char*)script_filename, "rb");

            /* if we open as text, the number of bytes read may be > the size we read */
            if (!file)
            {
                debug_log_output("Unable to open file '%s'", script_filename );
                close(socket);
                exit(1);
            }
            char *buffer = new char[size + 3];
            long actualRead = fread (buffer+2, 1, size, file);
            buffer[actualRead+2] = 0;
            buffer[size+2] = 0;
            buffer[0] = '?';
            buffer[1] = '>';
            fclose (file);

            CTinyJS  *s = new CTinyJS(STDOUT_FILENO);
            registerFunctions (&*s);
            registerMathFunctions (&*s);
            //insert Environment to js
            extern char** environ;
            wString script1;
            wString script2;
            wString script3;
            wString script4;
            s->root->addChild ("_SERVER", new CScriptVar (TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT));
            s->root->addChild ("_GET", new CScriptVar (TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT));
            if( http_recv_info_p->isGet== 3 ){
                s->root->addChild ("_POST", new CScriptVar (TINYJS_BLANK_DATA, SCRIPTVAR_OBJECT));
            }
            try
            {
                //環境変数をJSに展開
                for( int i=0 ; environ[i] != NULL ; i++ ){
                    script1 = environ[i];
                    int pos = script1.Pos("=");
                    if( (size_t)pos != wString::npos && pos > 1){
                         script2 = script1.substr(pos+1);
                         script1 = script1.substr(0,pos);
                         script1 = "var _SERVER."+script1+"=\""+script2+"\";";
                         //debug_log_output( "%s", script1.c_str() );
                         s->execute( script1 );
                    }
                }
                //query stringをgetに展開
                script3 = query_string;
                while( script3.length() ){
                    int pos = script3.Pos("&");
                    if( pos > 1 ){
                        script1 = script3.substr(0,pos);
                        int pos2 = script1.Pos("=");
                        script2 = "var _GET."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                        script3 = script3.substr(pos+1);
                    }else{
                        script1 = script3;
                        int pos2 = script1.Pos("=");
                        script2 = "var _GET."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                        script3 = "";
                    }
                    //debug_log_output( script2.c_str() );
                    s->execute( script2 );
                }
                if( http_recv_info_p->isGet== 3 ){
                    char buf[1024];
                    int num;
                    while(true){
                        num = read( STDIN_FILENO, buf, 1024);
                        if( num <= 0 ){
                            break;
                        }
                        script4 += buf;
                    }
                    //debug_log_output("POST %s", script4.c_str() );
                    while( script4.length() ){
                        int pos = script4.Pos("&");
                        if( pos > 1 ){
                            script1 = script4.substr(0,pos);
                            int pos2 = script1.Pos("=");
                            script2 = "var _POST."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                            script4 = script4.substr(pos+1);
                        }else{
                            script1 = script4;
                            int pos2 = script1.Pos("=");
                            script2 = "var _POST."+script1.substr(0,pos2)+"=\""+script1.substr(pos2+1).uri_decode(1)+"\";";
                            script4 = "";
                        }
                        //debug_log_output( script2.c_str() );
                        s->execute( script2 );
                    }
                }
                s->execute (buffer);
            }
            catch (CScriptException * e)
            {
                //debug_log_output("SCRIPT ERROR: %s\n", e->text.c_str ());
                printf("SCRIPT ERROR: %s\n", e->text.c_str ());
                delete s;
                delete[] buffer;
                close(socket);
                exit(1);
            }
            //fflush(stdout);
            //bool pass = s.root->getParameter ("result")->getBool ();
            delete s;
            delete[] buffer;
            wString::wStringEnd();
            debug_log_output("ServerSide JavaScript end");
        }else if( strcasecmp(ext,"php") == 0 ){
            wString str;
            str.headerInit(0,0);
            str.headerPrint(STDOUT_FILENO,0);
            wString::wStringEnd();
            if (execl("/usr/bin/php-cgi","php-cgi", (char*)script_filename, NULL) < 0) {
                debug_log_output("CGI EXEC ERROR. "
                "/usr/bin/php script = '%s', argv[0] = '%s', err = %s"
                , script_filename
                , script_exec_name
                , strerror(errno)
                );
                printf("\nPHP EXEC ERROR");
            }
        }else{
            wString str;
            str.headerInit(0,0);
            str.headerPrint(STDOUT_FILENO,0);
            wString::wStringEnd();
            if (execl((char*)script_filename, script_exec_name, NULL) < 0) {
                debug_log_output("CGI EXEC ERROR. "
                "script = '%s', argv[0] = '%s', err = %s"
                , script_filename
                , script_exec_name
                , strerror(errno)
                );
                printf("\nCGI EXEC ERROR");
            }
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(socket);
        exit( 0 );
    }
    close(accept_socket);
    return 0;
}

#endif
