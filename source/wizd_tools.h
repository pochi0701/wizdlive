// ==========================================================================
//code=UTF8	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_tools.h
//											$Revision: 1.7 $
//											$Date: 2004/03/10 04:47:55 $
//
//	すべて自己責任でおながいしまつ。
//  このソフトについてVertexLinkに問い合わせないでください。
// ==========================================================================
#ifndef	_WIZD_TOOLS_H
#define	_WIZD_TOOLS_H
#ifdef linux
#define O_BINARY                0
#define INVALID_SOCKET          (-1)
#define SOCKET_ERROR            (-1)
typedef int SOCKET;
#else
#include <winsock.h>
#include <time.h>
#include <sys/types.h>
#endif
#define		CR		(0x0D)
#define		LF		(0x0A)
#define		OUTPUT_LOG_ROTATE_SIZE		(1024*1024*1900)	/* 1.9Gbyte */
#define		OUTPUT_LOG_ROTATE_MAX		5					/* 何回のrotateを許すか */

// uriエンコード／デコード
extern int      uri_encode(char *dst, unsigned int dst_len, const char *src, unsigned int src_len);
extern int      uri_decode(char *dst, unsigned int dst_len, const char *src, unsigned int src_len);

// テキスト処理イロイロ。
extern char*    cut_after_character(char *sentence, char cut_char);
extern void     cut_before_character(char *sentence, char cut_char);
extern void     cut_character(char *sentence, char cut_char);
extern void     cut_first_character(char *sentence, char cut_char);
extern void     cut_before_last_character(char *sentence, char cut_char);
extern void     cut_after_last_character(char *sentence, char cut_char);
extern int      sentence_split(char *sentence, char cut_char, char *split1, char *split2);
extern void     duplex_character_to_unique(char *sentence, char unique_char);
extern void     replace_character(char *sentence, const char *key, const char *rep);
extern void     replace_character_first(char *sentence, const char *key, const char *rep);
extern void 	make_datetime_string(char *sentence);
extern void 	cut_character_at_linetail(char *sentence, char cut_char);
extern void     filename_to_extension(char *filename, char *extension_buf, unsigned int extension_buf_size);
extern char*    buffer_distill_line(char *text_buf_p, char *line_buf_p, unsigned int line_buf_size );
extern void     extension_add_rename(char *rename_filename_p, size_t rename_filename_size);
extern void     extension_del_rename(char *rename_filename_p);
extern char*    my_strcasestr(const char *p1, const char *p2);

extern int      HTTPDownload(char* src, char* dst, off_t offset);
//extern char* HTTPGet(char* src, off_t offset);
// path から ./ と ../ を取り除く。dir に結果が格納される。
extern char*    path_sanitize(char *dir, size_t dir_size);
// DebugLog 出力
extern void     debug_log_initialize(const char *set_debug_log_filename);
extern void     debug_log_output(const char *fmt, ...);
// 画像系
extern void     jpeg_size(char *jpeg_filename, unsigned int *x, unsigned int *y);
extern void     gif_size(char *gif_filename,   unsigned int *x, unsigned int *y);
extern void     png_size(char *png_filename,   unsigned int *x, unsigned int *y);
extern char*    ExtractFileExtension( char* filename );
extern void     filename_to_extension(char *filename, char *extension_buf, unsigned int extension_buf_size);

//システム処理
extern bool   FileExists(char* str);
extern size_t  FileSizeByName(char* str);
extern SOCKET sock_connect(char *host, int port);
extern char* ExtractFileExtension( char* filename );
extern void set_nonblocking_mode(int fd, int flag);
extern int sClose(SOCKET socket);
extern int readLine(int fd, char *line_buf_p, int line_max);
#ifdef linux
//extern int      send(int fd,char* buffer, unsigned int length, int mode);
//extern int      recv(int fd,char* buffer, unsigned int length, int mode);
extern int      getTargetFile( const char *LinkFile, char *TargetFile );
extern void     Sleep(unsigned int milliseconds);
extern void     ExitThread(DWORD dwExitCode);
#endif
char*           mymalloc(size_t size);
char*           mycalloc(size_t size1, int num);
void            myfree(char* ptr);
#endif
