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
int main()
{
        char script_filename[1000];
        char script_exec_name[1000];
        strcpy( script_filename,"/usr/bin/php-cgi /home/kanazawa/wizdlive20130809/test.php");
        strcpy( script_exec_name,"/home/kanazawa/wizdlive20130809/test.php");
        if (execl((char*)script_filename, script_exec_name, NULL) < 0) {
            printf("\nCGI EXEC ERROR");
        }
    return 0;
}
