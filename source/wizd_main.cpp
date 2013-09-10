// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_main.c
//											$Revision: 1.6 $
//											$Date: 2004/03/07 14:14:05 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include <unistd.h>
#include  <signal.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include  <sys/wait.h>
#include "wizd.h"
static void print_help(void);
static void daemon_init(void);
static void set_user_id(unsigned char *user, unsigned char *group);
static void setup_SIGCHLD(void);
static void catch_SIGCHLD(int signo);
extern int child_count;
// **************************************************************************
// * Main Program
// **************************************************************************
int main(int argc, char *argv[])
{
    int i;
    //pid_t	pid;
    char flag_daemon = FALSE;
    printf("%s  start.\n", SERVER_NAME);
    // =============================================
    // 各種初期化
    // =============================================
    global_param_init();
    // =============================================
    // オプションチェック
    // =============================================
    for (i=1; i<argc; i++)
    {
        // ----------------------------------------------------
        // -h, --help, -v, --version:  ヘルプメッセージ
        // ----------------------------------------------------
        if ( (strcmp(argv[i], "-h") 		== 0) ||
        (strcmp(argv[i], "--help")		== 0) ||
        (strcmp(argv[i], "-v") 		== 0) ||
        (strcmp(argv[i], "--version")	== 0) 		)
        {
            print_help();
#ifdef linux
            printf("abort by main\n");
            abort();
#else
            ExitThread(TRUE);
#endif
            return 0;
        }
        if (strcmp(argv[i], "-d") 		== 0)
        {
            flag_daemon = TRUE;
        }
    }
    // =============================================
    // コンフィグファイル読む(wizd.conf)
    // =============================================
    config_file_read();
    // デーモンモードについては、コマンドラインパラメータを優先
    if (flag_daemon)
    {
        global_param.flag_daemon = TRUE;
    }
    // ======================
    // = SetUID 実行
    // ======================
    set_user_id(global_param.exec_user, global_param.exec_group);
    // =======================
    // Debug Log 出力開始
    // =======================
    if ( global_param.flag_debug_log_output == TRUE )
    {
        printf("debug log output start..\n");
        debug_log_initialize(global_param.debug_log_filename);
        debug_log_output("\n%s boot up.", SERVER_NAME );
        debug_log_output("debug log output start..\n");
    }
    config_sanity_check();
    // =================
    // daemon化する。
    // =================
#ifdef linux
    if ( global_param.flag_daemon == TRUE )
    {
        printf("Daemoning....\n");
        daemon_init();
    }else{
        signal(SIGPIPE, SIG_IGN);
   signal(SIGCHLD, SIG_IGN);
    }
#endif
    // ==========================================
    // 子プロセスシグナル受信準備。forkに必要
    // ==========================================
    setup_SIGCHLD();
    // ==================================
    // Server自動検出を使うばあい、
    // Server Detect部をForkして実行
    // ==================================
    //if ( global_param.flag_auto_detect == TRUE )
    //{
        //    pid = fork();
        //    if ( pid < 0 ) // fork失敗チェック
        //    {
            //        perror("fork");
            //        exit( 1 );
        //    }
        //    if (pid == 0)
        //    {
            //        // 以下子プロセス部
            //        server_detect();
            //        exit ( 0 );
        //    }
    //}
    // =======================
    // HTTP Server仕事開始
    // =======================
    server_listen();
    printf("%s  end.\n", SERVER_NAME);
    exit( 0 );
}
// **************************************************************************
// Helpメッセージ出力
// **************************************************************************
static void print_help(void)
{
    printf("%s -- %s\n\n", SERVER_NAME, SERVER_DETAIL);
    printf("Usase: wizd [options]\n");
    printf("Options:\n");
    printf(" -h, --help\tprint this message.\n");
    printf("\n");
}
#ifdef linux
// **************************************************************************
// デーモン化する。
// **************************************************************************
static void daemon_init(void)
{
    int       pid;
    // 標準入出力／標準エラー出力を閉じる
    //fclose(stdin);
    //fclose(stdout);
    //fclose(stderr);
    pid = fork() ;
    if ( pid != 0 ){
        exit ( 0 );
    }
    setsid();
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    pid = fork();
    if ( pid != 0 ){
        exit ( 0 );
    }
    return;
}
#endif
// **************************************************************************
// 指定されたUID/GIDに変更する。root起動されたときのみ有効
// **************************************************************************
static void set_user_id(unsigned char *user, unsigned char *group)
{
    struct passwd 	*user_passwd;
    struct group 	*user_group;
    // rootかチェック
    if ( getuid() != 0 )
    return;
    // userが指定されているかチェック
    if ( strlen(user) <= 0 )
    return;
    // userIDをGet.
    user_passwd = getpwnam( (const char*)user );
    if ( user_passwd == NULL )
    {
        return;
    }
    // setuid実行
    setuid ( user_passwd->pw_uid );
    // groupはオプション。指定があれば設定する。
    if ( strlen(group) > 0 )
    {
        user_group = getgrnam( (const char*)group );
        if ( user_group == NULL )
        return;
        setgid( user_group->gr_gid );
    }
    return;
}
// **************************************************************************
// 子プロセスが終了したときにシグナルを受け取る設定
// **************************************************************************
static void setup_SIGCHLD(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    // SIGCHLD発生時にcatch_SIGCHLD()を実行
    act.sa_handler = catch_SIGCHLD;
    // catch_SIGCHLD()中の追加シグナルマスクなし
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);
    return;
}
// **************************************************************************
// シグナル駆動部。SIGCHLDを受け取ると呼ばれる。
// **************************************************************************
static void catch_SIGCHLD(int signo)
{
    pid_t child_pid = 0;
    int child_ret;
    debug_log_output("catch SIGCHLD!!(signo=%d)\n", signo);
    // すべての終了している子プロセスに対してwaitpid()を呼ぶ
    while ( 1 )
    {
        child_pid = waitpid(-1, &child_ret, WNOHANG);
        if (child_pid <= 0 )	// 全部waitpidが終わると、-1が戻る
        {
            break;
        }
        if (child_count > 0) child_count--;
        debug_log_output("catch_SIGCHILD waitpid()=%d, child_count = %d\n"
        , child_pid, child_count);
    }
    debug_log_output("catch SIGCHLD end.\n");
    return;
}
