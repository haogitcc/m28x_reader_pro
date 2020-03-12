
#ifndef __IND_STRING_H__
#define __IND_STRING_H__

#define IND_HASH_SIZE_199		199

#ifdef __cplusplus
extern "C" {
#endif


unsigned int ind_strhash(const unsigned char *s);
unsigned int ind_memhash(const unsigned char *s, int len);

int ind_memicmp(char *buf1, char *buf2, int count);
char *ind_stristr(char *String, char *Pattern);
char *ind_linestr(char *string, char *pattern);
int ind_linelen(char *string);
char *ind_memstr(char *buf, int len, char *str);
char *ind_memistr(char *buf, int len, char *str);
int ind_strline(char *string, char *buf, int len);
unsigned int ind_atoui(const char *str);
int ind_hextoi(const char *str, int len, unsigned int *value);
long long ind_ato64(const char *str);
void *ind_memmem(void *haystack, size_t haystacklen, void *needle, size_t needlelen);

int ind_lineparse(char* string, char* name, int namelen, char* value, int valuelen);

int ind_str8tohex(char *buf, unsigned int *hex);
int ind_str8frhex(unsigned int hex, char *buf);


int ind_realloc(void** pp, int size);

#ifdef __cplusplus
}
#endif

#endif//__IND_STRING_H__

