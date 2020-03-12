#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#include "ind_cfg.h"
#include "ind_string.h"

#define PRINT_SPACE_SIZE_256	256
#define NAME_FULL_SIZE_64		64

#define NAME_SIZE_64			64
#define VALUE_SIZE_256			256

typedef struct Param *Param_t;
struct Param {
	Param_t next;/* hash table next */
	Param_t brother;

	int type;
	int visible;

	int len;//字符串长度

	union {
		int *addr_int;
		uint32_t *addr_uint;
		char *addr_string;
		Param_t child;
	} val;

	char name[4];/* name的长度是可变的 */
};

struct CfgTree {
	Param_t prm_hash[IND_HASH_SIZE_199];
};

//------------------------------------------------------------------------------
CfgTree_t ind_cfg_create(void)
{
	CfgTree_t tree;

	tree = (CfgTree_t)malloc(sizeof(struct CfgTree));
	if (tree == NULL)
		goto Err;
	memset(tree, 0, sizeof(struct CfgTree));

	return tree;
Err:
	return NULL;
}

//------------------------------------------------------------------------------
static Param_t param_find(CfgTree_t tree, char *paramname)
{
	Param_t param = NULL;
	uint32_t idx;

	if (paramname == NULL)
		goto Err;
	idx = ind_strhash((unsigned char *)paramname);
	param = tree->prm_hash[idx];

	while(param && strncmp(param->name, paramname, strlen(paramname)))
		param = param->next;

	return param;
Err:
	return NULL;
}

//------------------------------------------------------------------------------
static int param_inset(CfgTree_t tree, Param_t param)
{
	char *p;
	uint32_t idx;
	uint32_t len;
	char name[NAME_FULL_SIZE_64];

	if (param_find(tree, param->name))
		goto Err;
	strcpy(name, param->name);
	len = strlen(name);
	if (name[len - 1] == '.')
		name[len - 1] = 0;
	p = strrchr(name, '.');
	if (p) {
			Param_t parent;

			p[0] = 0;
			parent = param_find(tree, name);
			if (parent == NULL)
				goto Err;
			if (parent->val.child) {
				Param_t child = parent->val.child;
				while(child->brother)
					child = child->brother;
				child->brother = param;
			} else {
				parent->val.child = param;
			}
			param->brother = NULL;
	}

	idx = ind_strhash((unsigned char *)param->name);
	param->next = tree->prm_hash[idx];
	tree->prm_hash[idx] = param;

	return 0;

Err:
	return -1;
}

//------------------------------------------------------------------------------
static int param_print(Param_t param, char *buf, int len)
{
	int length, l, i;
	char *name, *p, ch;
	Param_t child;

	if (param->visible == 0)
		return 0;
	if (len < (NAME_SIZE_64 + VALUE_SIZE_256))
		goto Err;

	length = 0;

	if (CFG_TYPE_OBJECT == param->type) {
		child = param->val.child;
		while(child) {
			l = param_print(child, buf + length, len - length);
			if (l < 0) {
				printf("%s param_print = %d\n", param->name, l);
				goto Err;
			}
			length += l;
			child = child->brother;
		}
	} else {
		name = strchr(param->name, '.');
		if (name == NULL)
			goto Err;
		name ++;

	    switch(param->type) {
    	case CFG_TYPE_OBJECT:
    		break;
    	case CFG_TYPE_INT:
    		length = sprintf(buf, "%s=%d\n", name, *param->val.addr_int);
    		break;
    	case CFG_TYPE_STRING:
    		l = strlen(param->val.addr_string);
    		if (l >= param->len)
    			goto Err;
    		length = sprintf(buf, "%s=", name);
    		buf += length;
    		p = param->val.addr_string;
    		for (i = 0; i < l; i ++) {
    			ch = p[i];
    			if (ch == '\n' || ch == '\\') {
    				*buf ++ = '\\';
    				length ++;
    			}
    			*buf ++ = ch;
    			length ++;
    		}
    		*buf = '\n';
    		length ++;
    		break;
    	default:
    		goto Err;
    	}
	}

	return length;
Err:
	return -1;
}

//------------------------------------------------------------------------------
static Param_t param_cfg_inset(CfgTree_t tree, char *paramname)
{
	int len;
	Param_t param;

	if (paramname == NULL)
		goto Err;
	len = strlen(paramname);
	if (len == 0 || len >= NAME_FULL_SIZE_64) {
		printf("paramname = %s\n", paramname);
		goto Err;
	}
	param = param_find(tree, paramname);
	if (param)
		goto Err;

	len = (len + sizeof(struct Param)) & 0xfffc;
	param = (Param_t)malloc(len);
	if (param == NULL)
		goto Err;
	memset(param, 0, sizeof(struct Param));
	param->visible = 1;
	strcpy(param->name, paramname);
	if (param_inset(tree, param))
		goto Err;

	return param;
Err:
	return NULL;
}

//------------------------------------------------------------------------------
int ind_cfg_inset_object(CfgTree_t tree, char *paramname)
{
	Param_t param;

	if (paramname == NULL)
		goto Err;

	param = param_cfg_inset(tree, paramname);
	if (param == NULL)
		goto Err;

	param->type = CFG_TYPE_OBJECT;

	return 0;
Err:
	return -1;
}

//------------------------------------------------------------------------------
int ind_cfg_inset_int(CfgTree_t tree, char *paramname, int *addr)
{
	Param_t param;

	if (paramname == NULL || addr == NULL)
		goto Err;

	param = param_cfg_inset(tree, paramname);
	if (param == NULL)
		goto Err;
	param->type = CFG_TYPE_INT;
	param->val.addr_int = addr;

	return 0;
Err:
	return -1;
}

//------------------------------------------------------------------------------
int ind_cfg_inset_string(CfgTree_t tree, char *paramname, char *addr, int len)
{
	Param_t param;

	if (paramname == NULL || addr == NULL || len <= 0)
		goto Err;
	param = param_cfg_inset(tree, paramname);
	if (param == NULL)
		goto Err;

	param->type = CFG_TYPE_STRING;
	param->len = len;
	param->val.addr_string = addr;

	return 0;
Err:
	return -1;
}

//------------------------------------------------------------------------------
static int config_elem(const char *buf, int len, int line, char *name, int nsize, char *value, int vsize)
{
	char ch;
	int i;

	for (i = 0; (isalnum(buf[i]) || buf[i] == '.' || buf[i] == '_') && i < len; i ++) ;
	if (i == 0 || i >= len)
		goto Err;

	if (i >= nsize)
		goto Err;
	memcpy(name, buf, i);
	name[i] = 0;
	buf += i;
	len -= i;

	if (buf[0] != '=') {
		printf("line %d name = %s, op = 0x%02x\n", line, name, (uint32_t)((uint8_t)buf[0]));
		goto Err;
	}
	buf ++;
	len --;

	i = 0;
	ch = *buf ++;
	while(ch != '\n' && ch != 0x00 && len > 0) {
		if (ch == '\\') {
			ch = *buf ++;
			len --;
			if (len < 0)
				goto Err;
			value[i ++] = ch;
		} else {
			value[i ++] = ch;
		}
		if (i >= vsize)
			goto Err;
		ch = *buf ++;
		len --;
	}
	value[i] = 0;

	return 0;
Err:
	return -1;
}

//------------------------------------------------------------------------------
int ind_cfg_input(CfgTree_t tree, char *rootname, char *buf, int len)
{
	int l, ret, line, rlen;
	char *p;
	Param_t param;
	char name[NAME_SIZE_64], value[VALUE_SIZE_256];

	if (rootname == NULL || buf == NULL || len < 0)
		goto Err;
	if (buf[len] != 0)
		goto Err;

	line = 1;
	rlen = sprintf(name, "%s.", rootname);
	if (rlen >= NAME_SIZE_64)
		goto Err;
	while (1) {
		while (isspace(buf[0]) && len > 0) {
			if (buf[0] == '\n')
				line ++;
			buf ++;
			len --;
		}
		if (len <= 0)
			break;

		p = buf;
		l = 0;
		while (*p != '\n' && l < len) {
			p ++;
			l ++;
		}

		ret = config_elem(buf, l, line, name + rlen, NAME_SIZE_64 - rlen, value, VALUE_SIZE_256);
		buf += l;
		len -= l;
		if (ret)
			continue;

		if (strncmp(rootname, name, strlen(rootname))) {
			printf("%s not child of %s at line %d\n", name, rootname, line);
			continue;
		}
		param = param_find(tree, name);
		if (param) {
			switch(param->type) {
			case CFG_TYPE_OBJECT:
				printf("%s is object at line %d\n", name, line);
				break;
			case CFG_TYPE_INT:
				if (value[0] == 0) {
					printf("value of %s error at line %d\n", name, line);
					break;
				}
				*param->val.addr_int = atoi(value);
				break;
			case CFG_TYPE_STRING:
				l = strlen(value);
				if (l >= param->len) {
					printf("value of %s too large at line %d\n", name, line);
					break;
				}
				strcpy(param->val.addr_string, value);
				break;
			default:
				break;
			}
		}
	}

	return 0;
Err:
	return -1;
}

//------------------------------------------------------------------------------
int ind_cfg_output(CfgTree_t tree, char *rootname, char *buf, int len)
{
	Param_t param;

	if (rootname == NULL || buf == NULL || len < PRINT_SPACE_SIZE_256)
		goto Err;

	param = param_find(tree, rootname);
	if (param == NULL)
		goto Err;
	buf[0] = 0;

	len = param_print(param, buf, len);
	if (len < 0)
		goto Err;

	return len;
Err:
	return -1;
}


//------------------------------------------------------------------------------
int ind_cfg_set_visible(CfgTree_t tree, char *paramname, int visible)
{
	Param_t param;

	if (paramname == NULL)
		goto Err;
	param = param_find(tree, paramname);
	if (param == NULL)
		goto Err;

	param->visible = visible;

	return 0;
Err:
	return -1;
}

