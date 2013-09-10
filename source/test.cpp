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
//////////////////////////////////////////////////////////////////////////
int main()
{
            if (execl("/usr/bin/php-cgi", "php-cgi", (char*)"/var/www/html/test.php", NULL) < 0) {
                printf("\nCGI EXEC ERROR");
            }
    return 0;
}
