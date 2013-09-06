//#define    replace_character(sentence,key,rep) replace_character((unsigned char*)(sentence),(unsigned char*)(key),(unsigned char*)(rep))
#define    cut_after_character(sentence,cut_char)  cut_after_character((unsigned char*)(sentence),(cut_char))
#define    cut_character_at_linetail(sentence,cut_char) cut_character_at_linetail((char *)(sentence), (char)(cut_char))
//#define    replace_character_first(sentence,key,rep) replace_character_first((unsigned char *)(sentence), (const unsigned char *)(key),(const unsigned char *)(rep))
#define    cut_enclose_words(sentence,sentence_buf_size,start_key,end_key) cut_enclose_words((unsigned char *)(sentence), (unsigned char *)(start_key), (unsigned char *)(end_key))
#define    replace_character(sentence,sentence_buf_size,key,rep) replace_character((unsigned char *)(sentence), (unsigned char *)(key), (unsigned char *)(rep))
#define    replace_character_first(sentence,sentence_buf_size,key,rep) replace_character_first((unsigned char *)(sentence), (unsigned char *)(key), (unsigned char *)(rep))
#define    uri_encode(dst,dst_len,src,src_len) uri_encode((unsigned char *)(dst),(unsigned int)(dst_len),(unsigned char *)(src),(unsigned int)(src_len))

