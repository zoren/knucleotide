/* The Computer Language Benchmarks Game
 * http://benchmarksgame.alioth.debian.org/

   Based on bit encoding idea of C++ contribution of Andrew Moon
   Copy task division idea from Java entry, contributed by James McIlree
   Contributed by Petr Prokhorenkov
   Modified by Stefano Guidoni
   Modified by Søren Sjørup
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "histogram.h"

#define HT_SIZE 21 // 2^21

typedef unsigned char uint8_t;
const uint8_t selector[] = { -1, 0,-1, 1, 3,-1,-1, 2 };
const char table[] = {'A', 'C', 'G', 'T'};

char *
read_stdin(int *stdin_size) {
    struct stat stat;
    fstat(0, &stat);

    char *result = malloc(stat.st_size);

    do {
        ;
    } while (fgets(result, stat.st_size, stdin) && strncmp(result, ">THREE", 6));

    int read = 0;
    while (fgets(result + read, stat.st_size - read, stdin)) {
        int len = strlen(result + read);
        if (len == 0 || result[read] == '>') {
            break;
        }
        read += len;
        if (result[read - 1] == '\n') {
            read--;
        }
    }

    result[read++] = '>';
    result = realloc(result, read);
    *stdin_size = read;

    return result;
}

static
inline char *
next_char(char *p) {
    do {
        ++p;
    } while (isspace(*p));

    return p;
}

inline uint64_t
push_char(uint64_t cur, uint8_t c) {
    return (cur << 2) + selector[(c & 7)];
}

uint64_t
pack_key(char *key, int len) {
    uint64_t code = 0;
    for (int i = 0; i < len; i++) {
        code = push_char(code, *key);
        key = next_char(key);
    }

    return code;
}

void
unpack_key(uint64_t key, int length, char *buffer) {
    int i;

    for (i = length - 1; i > -1; i--) {
        buffer[i] = table[key & 3];
        key >>= 2;
    }
    buffer[length] = 0;
}

void
generate_seqences(char *start, int length, int frame, struct hist_table *ht) {
    uint64_t code = 0;
    uint64_t mask = (1ull << 2*frame) - 1;
    char *p = start;
    char *end = start + length;

    // Pull first frame.
    for (int i = 0; i < frame; i++) {
        code = push_char(code, *p);
        ++p;
    }
    hist_increment(ht, code);

    while (p < end) {
        code = push_char(code, *p) & mask;
        hist_increment(ht, code);
        ++p;
        if (*p & 8) {
            if (*p & 1) {
                ++p;
            } else
                break;
        }
    }
}

int
key_count_cmp(const void *l, const void *r) {
    const struct hist_entry *lhs = l, *rhs = r;

    if (lhs->count != rhs->count) {
        return rhs->count - lhs->count;
    } else {
        // Overflow is possible here,
        // so use comparisons instead of subtraction.
        if (lhs->key < rhs->key) {
            return -1;
        } else if (lhs->key > rhs->key) {
            return 1;
        } else {
            return 0;
        }
    }
}

struct print_freqs_param {
    char *start;
    int length;
    int frame;
    char *output;
    int output_size;
};

struct hist_entry *
hist_values_as_vector(struct hist_table *ht) {
  struct hist_entry *v = malloc(ht->entry_count*sizeof(struct hist_entry));

  int j = 0;
  for(int i = 0;i < ht->size; i++){
    struct hist_entry* e = &ht->entries[i];
    if(e->count){
      v[j] = *e;
      j++;
    }
  }

  return v;
}

void
print_freqs(struct print_freqs_param *param) {
    char *start = param->start;
    int length = param->length;
    int frame = param->frame;
    char *output = param->output;
    int output_size = param->output_size;

    struct hist_table *ht = hist_create(5);
    char buffer[frame + 1];
    int output_pos = 0;

    generate_seqences(start, length, frame, ht);

    struct hist_entry *counts = hist_values_as_vector(ht);
    int size = ht->entry_count;

    qsort(counts, size, sizeof(struct hist_entry), &key_count_cmp);

    int total_count = 0;
    for (int i = 0; i < size; i++) {
        total_count += counts[i].count;
    }

    for (int i = 0; i < size; i++) {
        unpack_key(counts[i].key, frame, buffer);
        output_pos += snprintf(output + output_pos, output_size - output_pos,
                "%s %.3f\n", buffer, counts[i].count*100.0f/total_count);
    }

    free(counts);
    hist_destroy(ht);
}

struct print_occurences_param {
    char *start;
    int length;
    char *nuc_seq;
    char *output;
    int output_size;
};

void
print_occurences(struct print_occurences_param *param) {
    char *start = param->start;
    int length = param->length;
    char *nuc_seq = param->nuc_seq;
    char *output = param->output;
    int output_size = param->output_size;
    int nuc_seq_len = strlen(nuc_seq);
    struct hist_table *ht = hist_create(HT_SIZE);

    generate_seqences(start, length, nuc_seq_len, ht);

    uint64_t key = pack_key(nuc_seq, nuc_seq_len);
    int count = hist_find(ht, key);
    snprintf(output, output_size, "%d\t%s\n", count, nuc_seq);

    hist_destroy(ht);
}

#define MAX_OUTPUT 1024

int
main(void) {
    int stdin_size;
    char *stdin_mem = read_stdin(&stdin_size);

    char output_buffer[7][MAX_OUTPUT];

#   define DECLARE_PARAM(o, n) {\
    .start = stdin_mem, \
    .length = stdin_size, \
    .frame = n,\
    .output = output_buffer[o],\
    .output_size = MAX_OUTPUT }

    struct print_freqs_param freq_params[2] = {
        DECLARE_PARAM(0, 1),
        DECLARE_PARAM(1, 2)
    };

#   undef DECLARE_PARAM

#   define DECLARE_PARAM(o, s) {\
    .start = stdin_mem, \
    .length = stdin_size, \
    .nuc_seq = s,\
    .output = output_buffer[o],\
    .output_size = MAX_OUTPUT }

    struct print_occurences_param occurences_params[5] = {
        DECLARE_PARAM(2, "GGT"),
        DECLARE_PARAM(3, "GGTA"),
        DECLARE_PARAM(4, "GGTATT"),
        DECLARE_PARAM(5, "GGTATTTTAATT"),
        DECLARE_PARAM(6, "GGTATTTTAATTTATAGT")
    };

#   undef DECLARE_PARAM

    for (int i = 0 ; i < 2; i++) {
        print_freqs(&freq_params[i]);
    }
    for (int i = 0 ;i <  5; i++) {
        print_occurences(&occurences_params[i]);
    }

    for (int i = 0; i < 2; i++) {
        printf("%s\n", output_buffer[i]);
    }
    for (int i = 2; i < 7; i++) {
        printf("%s", output_buffer[i]);
    }

    free(stdin_mem);

    return 0;
}
