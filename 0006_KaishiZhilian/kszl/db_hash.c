#include <stdio.h>
#include <stdlib.h>

#include "db_hash.h"
#include "tm_reader.h"
#include "mid_mutex.h"

static mid_mutex_t g_mutex = NULL;
tag_database *seenTags = NULL;

void hash_init()
{
    g_mutex = mid_mutex_create();
    if(g_mutex == NULL) {
        printf("mid_mutex_create");
        return -1;
    }

   seenTags = init_tag_database(20);
}

char *my_strdup(const char *string)
{
  char *nstr;
  
  nstr = (char *) malloc(strlen(string) + 1);
  if (nstr)
  {
    strcpy(nstr, string);
  }
  return nstr;
}

tag_database *init_tag_database(uint32_t size)
{
  tag_database *db;
  uint32_t i;
    
  if (size < 1) 
  {
    return NULL;
  }
  
  if ((db = malloc(sizeof(tag_database))) == NULL)
  {
    return NULL;
  }
  
  if ((db->table = malloc(sizeof(tagdb_table *) * size)) == NULL)
  {
    return NULL;
  }
  
  for(i = 0; i < size; i++)
  {
    db->table[i] = NULL;
  }

  db->size = size;
  return db;
}

unsigned int hash(char *epc)
{
  if(!seenTags)
		return NULL;

  unsigned int tagHash = 0;
    
  for(; *epc != '\0'; epc++)
  {
    tagHash = *epc + (tagHash << 5) - tagHash;
  }
  return tagHash % seenTags->size;
}

void db_lookup_string(char *value, int count)
{
  if(!seenTags)
		return NULL;
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }
  int i, j = 1;
  tagdb_table *list, *temp;
  char str[64];
  
  for (i = 0; i < seenTags->size; i++)
  {
    list = seenTags->table[i];
    while (list != NULL)
    {
      temp = list;
      list = list->next;
      sprintf(str, "%s,%s,%d;",  temp->epc, temp->time, temp->count);
	strcpy(value, str);
	value += strlen(str);
	j++;
	if(j > count)
		break;
    }
  }
    mid_mutex_unlock(g_mutex);

}

tagdb_table *db_lookup(char *epc)
{
  if(!seenTags)
		return NULL;
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }

   tagdb_table *list;
  unsigned int tagHash = hash( epc);

  for(list = seenTags->table[tagHash]; list != NULL; list = list->next)
  {
    if (strcmp(epc, list->epc) == 0)
    {
    	  list->count++;
        mid_mutex_unlock(g_mutex);
      	  return list;
    }
  }

  mid_mutex_unlock(g_mutex);
  return NULL;
}

TMR_Status db_insert(char *epc, char *time)
{
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }
  tagdb_table *new_list;
  unsigned int tagHash = hash(epc);
  
  if ((new_list = malloc(sizeof(tagdb_table))) == NULL)
  {
    mid_mutex_unlock(g_mutex);
    return TMR_ERROR_OUT_OF_MEMORY;
  }
  
  /* Insert the record now */
  new_list->epc = my_strdup(epc);
  new_list->time = my_strdup(time);
  new_list->count = 1;
  new_list->next = seenTags->table[tagHash];
  seenTags->table[tagHash] = new_list;
  mid_mutex_unlock(g_mutex);
  return TMR_SUCCESS;
}

void db_free()
{
  int i;
  tagdb_table *list, *temp;
  if (seenTags == NULL)
  {
    return;
  }
   if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }
  /**
   * Free the memory for every item in tag_database
   */
  for (i = 0; i < seenTags->size; i++)
  {
    list = seenTags->table[i];
    while (list != NULL)
    {
      temp = list;
      list = list->next;
      free(temp->epc);
      free(temp->time);
      free(temp);
    }
  }
    for(i = 0; i < 20; i++)
  {
    seenTags->table[i] = NULL;
  }

  seenTags->size = 20;
  mid_mutex_unlock(g_mutex);
  /* Free the table itself */
  //free(db->table);
  //free(db);
}
