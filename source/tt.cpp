#include "wizd_String.h"


int main( int argc, char* argv[] )
{
     wString::wStringInit();
     wString str;
     str.LoadFromFile( "/tmp/aaa");
     while( str.length() ){
          int p = str.Pos("\r\n" );
          if( p == wString::npos ){
              break;
          }
          wString b = str.substr(0,p);
          str = str.substr(p+2);
          printf( "%d", p );
          printf( "[%d][%s]\n", str.length(),b.c_str() );
     }
     wString::wStringEnd();
}
