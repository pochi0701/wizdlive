//#define    replace_character(sentence,key,rep) replace_character((unsigned char*)(sentence),(unsigned char*)(key),(unsigned char*)(rep))
#define    cut_after_character(sentence,cut_char)  cut_after_character((unsigned char*)(sentence),(cut_char))
#define    cut_character_at_linetail(sentence,cut_char) cut_character_at_linetail((char *)(sentence), (char)(cut_char))
//#define    replace_character_first(sentence,key,rep) replace_character_first((unsigned char *)(sentence), (const unsigned char *)(key),(const unsigned char *)(rep))
#define    cut_enclose_words(sentence,sentence_buf_size,start_key,end_key) cut_enclose_words((unsigned char *)(sentence), (unsigned char *)(start_key), (unsigned char *)(end_key))
#define    replace_character(sentence,sentence_buf_size,key,rep) replace_character((unsigned char *)(sentence), (unsigned char *)(key), (unsigned char *)(rep))
#define    replace_character_first(sentence,sentence_buf_size,key,rep) replace_character_first((unsigned char *)(sentence), (unsigned char *)(key), (unsigned char *)(rep))
#define    uri_encode(dst,dst_len,src,src_len) uri_encode((unsigned char *)(dst),(unsigned int)(dst_len),(unsigned char *)(src),(unsigned int)(src_len))

//exnern int strlen(const unsigned char *str);
//extern char *strncpy(const unsigned char *dest, const          char *src, size_t n);
//extern char *strncpy(const unsigned char *dest, const unsigned char *src, size_t n);
//extern char *strncpy(const          char *dest, const unsigned char *src, size_t n);
//extern char *strncat(const unsigned char *dest, const unsigned char *src, size_t n);
//extern char *strncat(const unsigned char *dest, const          char *src, size_t n);
//extern char *strncat(               char *dest, const unsigned char *src, size_t n);
//extern char *strcpy(const unsigned char *dest, const          char *src);
//extern char *strcpy(const unsigned char *dest, const unsigned char *src);
//extern char *strcpy(const          char *dest, const unsigned char *src);
//extern char *strcat(unsigned char *dest, unsigned char *src);
//extern char *strcat(unsigned char *dest, const    char *src);
//extern char *strcat(         char *dest, unsigned char *src);
//extern char *strchr(const unsigned char *s, int c);
//extern char *strchr(const unsigned char *s, unsigned char c);
//extern char *strrchr(const unsigned char *s, int c);
//extern int strcasecmp(unsigned char *dest, unsigned char *src);
//extern int strcasecmp(unsigned char *dest, const    char *src);
//extern int strcasecmp(const    char *dest, unsigned char *src);
//extern int strncasecmp(unsigned char *dest, unsigned char *src, size_t n);
//extern int strncasecmp(unsigned char *dest, const    char *src, size_t n);
//extern int strncasecmp(const    char *dest, unsigned char *src, size_t n);
//extern int strcmp(unsigned char *dest, unsigned char *src);
//extern int strcmp(unsigned char *dest, const    char *src);
//extern int strcmp(const    char *dest, unsigned char *src);
//extern int strncmp(unsigned char *dest, unsigned char *src, size_t n);
//extern int strncmp(unsigned char *dest, const    char *src, size_t n);
//extern int strncmp(const    char *dest, unsigned char *src, size_t n);
//extern int atoi(unsigned  char *nptr);
//extern void  replace_character(unsigned char *sentence, int sentence_buf_size, const char *key, const char *rep);
//extern void  replace_character(unsigned char *sentence, int sentence_buf_size, const char *key, const unsigned char *rep);
//extern void  cut_character_at_linetail(unsigned char *sentence, char cut_char);
//extern void  cut_after_character(char *sentence,unsigned char cut_char);
//extern int stat(const unsigned char *path, struct stat *buf);
//extern char *strstr(const unsigned char *haystack, const unsigned char *needle);
//extern char *strstr(const unsigned char *haystack, const          char *needle);
//extern char *strstr(const          char *haystack, const unsigned char *needle);
//extern int sprintf(unsigned char *str, const char *format, ...); 
//extern int snprintf(unsigned char *str, size_t size, const char *format, ...);
//
//extern void     replace_character_first(unsigned char *sentence, int sentence_buf_size, const unsigned char *key, const unsigned char *rep);
