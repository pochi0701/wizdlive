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

#include "castpatch2.h"

#define			CR		(0x0D)
#define			LF		(0x0A)

#define		OUTPUT_LOG_ROTATE_SIZE		(1024*1024*1900)	/* 1.9Gbyte */
#define		OUTPUT_LOG_ROTATE_MAX		5					/* 何回のrotateを許すか */

// uriエンコード／デコード
extern int 		uri_encode(unsigned char *dst,  unsigned int dst_len, const unsigned char *src, unsigned int src_len);
extern unsigned char*   myuri_encode(unsigned char *src);
extern int 		uri_decode(unsigned char *dst, unsigned int dst_len, const unsigned char *src, unsigned int src_len);

// テキスト処理イロイロ。
extern char*    cut_after_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_before_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_first_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_before_last_character(unsigned char *sentence, unsigned char cut_char);
extern void 	cut_after_last_character(unsigned char *sentence, unsigned char cut_char);
extern int 		sentence_split(unsigned char *sentence, unsigned char cut_char, unsigned char *split1, unsigned char *split2);
extern void 	duplex_character_to_unique(unsigned char *sentence, unsigned char unique_char);
extern void     replace_character(unsigned char *sentence, const unsigned char *key, const unsigned char *rep);
extern void     replace_character_first(unsigned char *sentence, const unsigned char *key, const unsigned char *rep);
extern void 	make_datetime_string(unsigned char *sentence);
extern void 	conv_time_to_string(unsigned char *sentence, time_t conv_time);
extern void 	conv_time_to_date_string(unsigned char *sentence, time_t conv_time);
extern void 	conv_time_to_time_string(unsigned char *sentence, time_t conv_time);
extern void 	cat_before_n_length(unsigned char *sentence,  unsigned int n);
extern void 	cat_after_n_length(unsigned char *sentence,  unsigned int n);
extern void 	cut_character_at_linetail(char *sentence, char cut_char);
extern void     filename_to_extension(unsigned char *filename, unsigned char *extension_buf, unsigned int extension_buf_size);
extern void 	han2euczen(unsigned char *src, unsigned char *dist, int dist_size);
extern void 	euc_string_cut_n_length(unsigned char *euc_sentence,  unsigned int n);
extern void     cut_enclose_words(unsigned char *sentence, unsigned char *start_key, unsigned char *end_key);
extern void 	decode_samba_hex_and_cap_coding( unsigned char *sentence );
extern void 	sjis_code_thrust_replace(unsigned char *sentence, const unsigned char code );

extern unsigned char 	*buffer_distill_line(unsigned char *text_buf_p, unsigned char *line_buf_p, unsigned int line_buf_size );

extern void extension_add_rename(unsigned char *rename_filename_p, size_t rename_filename_size);
extern void extension_del_rename(unsigned char *rename_filename_p, size_t rename_filename_size);

extern char *my_strcasestr(const char *p1, const char *p2);

// path から ./ と ../ を取り除く。dir に結果が格納される。
extern unsigned char *path_sanitize(unsigned char *dir, size_t dir_size);

// DebugLog 出力

extern void debug_log_initialize(const unsigned char *set_debug_log_filename);
extern void debug_log_output(const char *fmt, ...);

// 画像系
extern void jpeg_size(unsigned char *jpeg_filename, unsigned int *x, unsigned int *y);
extern void gif_size(unsigned char *gif_filename, 	unsigned int *x, unsigned int *y);
extern void png_size(unsigned char *png_filename, 	unsigned int *x, unsigned int *y);

//システム処理
extern bool   FileExists(unsigned char* str);
extern size_t  FileSizeByName(unsigned char* str);
extern void   Sleep(unsigned int milliseconds);
extern SOCKET sock_connect(char *host, int port);
extern void   sock_close(SOCKET sock);
extern char* ExtractFileExtension( unsigned char* filename );
//extern char* uri_encode(char* src);
extern void set_blocking_mode(int fd, int flag);
#endif
