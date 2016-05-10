#ifndef SIMPLE_HISTOGRAM
#define SIMPLE_HISTOGRAM

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// an implementation of a histogram of 64-bit int's
// power-of-2 size hashtable with linear probing
// todo: resizing and removal

typedef unsigned long long ui64;
typedef unsigned char uchar;
typedef ui64 hist_key;
typedef ui64 hist_count;

struct hist_entry {
  hist_key key;
  hist_count count;
};

struct hist_table {
  uchar exponent_size;
  ui64 entry_count;
  ui64 size;
  ui64 mask;
  struct hist_entry* entries;
};

struct hist_table*
hist_create(uchar initial_exponent_size) {
  int size = 1 << initial_exponent_size;
  int mask = size - 1;
  struct hist_table *ht = (struct hist_table*)calloc(1, sizeof(struct hist_table));

  ht->exponent_size = initial_exponent_size;
  ht->size = size;
  ht->mask = mask;
  ht->entry_count = 0;
  ht->entries = calloc(size, sizeof(struct hist_entry));

  return ht;
}

static inline
int
hist_hash_index(struct hist_table* ht, hist_key key) {
  return key & ht->mask;
}

const hist_count hist_not_found = 0;

hist_count hist_find(struct hist_table* ht, hist_key key) {
  int hash_index = hist_hash_index(ht, key);
  for(int i = 0;i < ht->size;i++){
    struct hist_entry entry = ht->entries[(i+hash_index)&ht->mask];
    if(!entry.count){
      return hist_not_found;
    }
    if(entry.key == key){
      return entry.count;
    }
  }
  return hist_not_found;
}

void hist_increment(struct hist_table* ht, hist_key key) {
  if(ht->entry_count == ht->size){
    printf("cannot add, table full\n" );
    exit(EXIT_FAILURE);
  }
  int hash_index = hist_hash_index(ht, key);
  for(int i = 0;i < ht->size;i++){
    struct hist_entry* entry = &ht->entries[(i+hash_index) & ht->mask];
    if(!entry->count){
      entry->key = key;
      entry->count = 1;

      ht->entry_count++;
      return;
    }else{
      if(entry->key == key){
        hist_count v = entry->count + 1;
        if(v == 0){
          printf("count wrapped\n");
          exit(EXIT_FAILURE);
        }
        entry->count = v;
        return;
      }
    }
  }
}

void hist_visit(struct hist_table* ht, void(*visitor)(hist_key, hist_count)) {
  int size = 1 << ht->exponent_size;
  for(int i = 0;i < ht->size;i++){
    struct hist_entry* entry = &ht->entries[i];

    if(entry->count){
      visitor(entry->key, entry->count);
    }
  }
}

void hist_destroy(struct hist_table* ht) {
  free(ht->entries);
  ht->entries = NULL;
  free(ht);
}

#endif // SIMPLE_HISTOGRAM
