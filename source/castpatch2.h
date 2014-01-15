#define         NO_RESPONSE_TIMEOUT     (10)
#define O_BINARY 0
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define SERROR(x) ((x) == INVALID_SOCKET)
typedef int SOCKET;
typedef unsigned long DWORD;

#define    strlen(str)              strlen((char*)str)
#define    strncpy(dst,src,n)       strncpy((char*)(dst),(char*)(src),(n))
#define    strcpy(dst,src)          strcpy((char*)(dst),(char*)(src))
#define    strncat(dst,src,n)       strncat((char*)(dst),(char*)(src),(n))
#define    strcat(dst,src)          strcat((char*)(dst),(char*)(src))
#define    strchr(src,c)            strchr((char*)(src),(c))
#define    strrchr(src,c)           strrchr((char*)src,(c))
#define    strcasecmp(dst,src)      strcasecmp((char*)(dst), (char*)(src))
#define    strncasecmp(dst,src,n)   strncasecmp((char*)(dst),(char*)(src),(n))
#define    strcmp(dst,src)          strcmp((char*)(dst),(char*)(src))
#define    strncmp(dst,src,n)       strncmp((char*)(dst),(char*)(src),(n))
#define    atoi(nptr)               atoi((char*)(nptr))
#define    strstr(haystack,needle)  strstr((char*)(haystack),(char*)(needle))
//#define    stat(path,buf)           stat((char*)path,(buf))
//#define    sprintf(str,...)         sprintf((char*)(str), __VA_ARGS__)
#define    snprintf(str,size,...)   snprintf((char*)(str),(size_t)(size),__VA_ARGS__)
extern     int open(unsigned char *pathname,int flags,mode_t  mode);
extern     int open(unsigned char *pathname,int flags);
