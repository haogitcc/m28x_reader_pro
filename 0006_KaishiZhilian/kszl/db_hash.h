#ifndef _DB_HASH_H
#define _DB_HASH_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tm_reader.h"
#ifdef  __cplusplus
extern "C" {
#endif



typedef struct tagdb_table
{
  char *epc;
  char *time;
  int count;
  struct tagdb_table *next;
} tagdb_table;

typedef struct tag_database
{
  int size;            /* the size of the database */
  tagdb_table **table;
} tag_database;

char *my_strdup(const char *string);
tag_database *init_tag_database(uint32_t size);
unsigned int hash(char *epc);
void db_lookup_string(char *value, int count);
tagdb_table *db_lookup(char *epc);
TMR_Status db_insert( char *epc, char *time);
void db_free();
void hash_init();


#ifdef __cplusplus
}
#endif

#endif
