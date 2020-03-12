
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#include "ind_string.h"

uint32_t ind_strhash(const u_char *s)
{
	uint32_t h = 0;
	while (*s)
		h = 65599 * h + *s ++;
  return h % IND_HASH_SIZE_199;
}

uint32_t ind_memhash(const u_char *s, int len)
{
	int i;
	uint32_t h = 0;

	for (i = 0; i < len; i ++)
		h = 65599 * h + *s ++;
  return h % IND_HASH_SIZE_199;
}

int ind_memicmp(char *buf1, char *buf2, int count)
{
	int i;
	if (buf1 == NULL || buf2 == NULL || count <= 0)
		return -1;
	for (i = 0; i < count; i ++) {
		if (toupper(buf1[i]) != toupper(buf2[i]))
			return -1;
	}
	return 0;
}

char *ind_memstr(char *buf, int len, char *str)
{
	int l;
	if (buf == NULL || str == NULL)
		return NULL;
	l = strlen(str);

	for (; l <= len; buf ++, len --) {
		if (*buf != *str)
			continue;
		if (memcmp(buf, str, l) == 0)
			return buf;
	}
	return NULL;
}

char *ind_memistr(char *buf, int len, char *str)
{
	int l;
	if (buf == NULL || str == NULL)
		return NULL;
	l = strlen(str);

	for (; l <= len; buf ++, len --) {
		if (toupper(buf[0]) != toupper(str[0]))
			continue;
		if (ind_memicmp(buf, str, l) == 0)
			return buf;
	}
	return NULL;
}

char *ind_stristr(char *string, char *pattern)
{
	if (string == NULL || pattern == NULL)
		return NULL;
	return ind_memistr(string, strlen(string), pattern);
}

char *ind_linestr(char *string, char *pattern)
{
	int l, len;
	if (string == NULL || pattern == NULL)
		return NULL;
	len = strlen(string);
	l = strlen(pattern);

	for (; l <= len; string ++, len --) {
		if (string[0] == '\r' || string[0] == '\n')
			break;
		if (string[0] != pattern[0])
			continue;
		if (memcmp(string, pattern, l) == 0)
			return string;
	}
	return NULL;
}

int ind_linelen(char *string)
{
	char ch;
	int len;

	if (string == NULL)
		return 0;

	len = 0;
	for (;;) {
		ch = string[len];
		if (ch == '\r' || ch == '\n' || ch == 0)
			break;
		len ++;
	}

	return len;
}

int ind_lineparse(char* string, char* name, int namelen, char* value, int valuelen)
{
	int i, j, l;
	char ch;

	if (string == NULL || name == NULL || namelen <= 0 || value == NULL || valuelen <= 0) {
		printf("string = %p, name = %p, namelen = %d, value = %p, valuelen = %d\n", string, name, namelen, value, valuelen);
		goto Err;
	}
	i = 0;
	for (;;i ++) {
		ch = string[i];
		if (ch != '\r' && ch != '\n')
			break;
	}
	if (ch == '\0')
		return 0;

	j = i;
	for (;;i ++) {
		ch = string[i];
		if (ch == '\r' || ch == '\n' || ch == '\0' || ch == '=')
			break;
	}
	if (ch != '=')
		goto Err;
	l = i - j;
	if (l <= 0)
		goto Err;
	if (l >= namelen)
		goto Err;
	memcpy(name, string + j, l);
	name[l] = 0;

	i ++;
	j = i;
	for (;;i ++) {
		ch = string[i];
		if (ch == '\r' || ch == '\n' || ch == '\0')
			break;
	}
	l = i - j;
	if (l >= valuelen) {
		printf("l = %d, valuelen = %d\n", l, valuelen);
		goto Err;
	}
	memcpy(value, string + j, l);
	value[l] = 0;

	return i;
Err:
	return -1;
}

int ind_strline(char *string, char *buf, int len)
{
	int l;

	if (string == NULL || buf == NULL) {
		//printf("string = %p, buf = %p\n", string, buf);
		return -1;
	}

	l = 0;
	for (;;) {
		char ch = string[l];
		if (ch == '\r' || ch == '\n' || ch == '\0')
			break;
		l ++;
	}

	if (l >= len) {
		//printf("l = %d, len = %d\n", l, len);
		return -2;
	}
	if (l > 0)
		memcpy(buf, string, l);
	buf[l] = '\0';

	return l;
}

void *ind_memmem(void *haystack, size_t haystacklen,
		     void *needle, size_t needlelen)
{
	char *ph, *pn;
	char *plast;
	size_t n;

	if (needlelen == 0)
		return (void *) haystack;

	if (haystacklen < needlelen)
		return NULL;

	ph = (char *) haystack;
	pn = (char *) needle;
	plast = ph + (haystacklen - needlelen);

	do {
		n = 0;
		while (ph[n] == pn[n]) {
			if (++n == needlelen) {
				return (void *) ph;
			}
		}
	} while (++ph <= plast);

	return NULL;
}

uint32_t ind_atoui(const char *str)
{
	int i;
	char ch;
	uint32_t n = 0;

	for (i = 0; i < 10; i ++) {
		ch = str[i];
		if (ch < '0' || ch > '9')
			return n;
		n = n * 10 + (ch - '0');
	}
	return n;
}

long long ind_ato64(const char *str)
{
	int i;
	char ch;
	long long n = 0;

	for (i = 0; i < 20; i ++) {
		ch = str[i];
		if (ch < '0' || ch > '9')
			return n;
		n = n * 10 + (ch - '0');
	}
	return n;
}

int ind_hextoi(const char *str, int len, uint32_t *value)
{
	uint32_t v, i, l;
	char ch;

	v = 0;

	if (len >= 8)
		l = 8;
	else
		l = len;
	for (i = 0; i < l; i ++) {
		ch = str[i];
		if (ch >= 'a')
			ch -= 'a' - 'A';
		if ((ch < '0' || ch > '9') && (ch < 'A' || ch > 'F'))
			break;
		v <<= 4;
		if (ch >= '0' && ch <= '9') {
			v += ch - '0';
			continue;
		}
		if (ch >= 'A' && ch <= 'F') {
			v += ch - 'A' + 10;
			continue;
		}
		return -2;
	}

	if (i == 0)
		return -1;

	if (i == 8) {
		ch = str[8];
		if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F'))
			return -3;
	}

	*value = v;

	return 0;
}

int ind_str8tohex(char *buf, unsigned int *hex)
{
	int i;
	char ch;
	unsigned int h;

	if (buf == NULL || hex == NULL) {
		printf("buf = %p, hex = %p\n", buf, hex);
		goto Err;
	}
	h = 0;
	for (i = 0; i < 8; i ++) {
		ch = buf[i];
		if (ch >= '0' && ch <= '9')
			h = (h << 4) + (ch - '0');
		else if (ch >= 'a' && ch <= 'f')
			h = (h << 4) + 10 + (ch - 'a');
		else {
			printf("buf[%d] = %x\n", i, ch);
			goto Err;
		}
	}
	*hex = h;

	return 0;
Err:
	return -1;
}

int ind_str8frhex(unsigned int hex, char *buf)
{
	int i;
	unsigned int h;

	if (buf == NULL)
		goto Err;

	for (i = 7; i >= 0; i --, hex >>= 4) {
		h = hex & 0xF;
		if (h <= 9)
			buf[i] = '0' + h;
		else
			buf[i] = 'a' + (h - 10);
	}
	buf[8] = 0;

	return 0;
Err:
	return -1;
}

int ind_realloc(void** pp, int size)
{
	void *p;

	if (pp == NULL || size <= 0)
		goto Err;

	if (*pp)
		free(*pp);
	*pp = NULL;

	p = malloc(size);
	if (p == NULL)
		goto Err;

	*pp = p;

	return 0;
Err:
	return -1;
}
