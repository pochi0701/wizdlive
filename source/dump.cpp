#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


void dp( unsigned char* buf, int len )
{
    int i;
    for( i = 0 ; i < 16 ; i++ ){
        if( i <= len ){
            printf( "%02X ", buf[i] );
        }else{
            printf( "   ");
        }
    }
    printf( " " );
    for( i = 0 ; i < 16 ; i++ ){
        if( i <= len ){
            if( isprint(buf[i]) ){
                 printf( "%c", buf[i] );
            }else{
                 printf( "." );
            }
        }else{
            printf( " " );
        }
     }
     printf( "\n" );
}
//---------------------------------------------------------------------------
// ファイル読み込み
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    char FileName[1024];
    unsigned char buf[256];
    if( argc < 2 ){
         printf( "Usage odd filename\n");
         return 0;
    }
    strcpy( FileName, argv[1] );
    
    long flen;
    int rlen,len;
    int  handle;
    handle  = open(FileName,O_RDONLY | S_IREAD );
    flen = lseek(handle,0,SEEK_END);
           lseek(handle,0,SEEK_SET);
    while( flen )
    {
        memset( (char*)buf, 0, 256);
        if( flen >= 16 ){
           rlen = 16;
        }else{
           rlen = flen;
        }
        len = read( handle, (char*)buf, rlen);
        flen -= rlen;
        dp( buf, rlen );
    }
    close(handle);
}
