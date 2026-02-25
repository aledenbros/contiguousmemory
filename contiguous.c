#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "contiguous.h"

struct contiguous {
  struct cnode *first;
  void *upper_limit;
};

struct cnode {
  size_t nsize;
  struct cnode *prev;
  struct cnode *next;
  struct contiguous *block;
};

const int SIZEOF_CONTIGUOUS = sizeof(struct contiguous);
const int SIZEOF_CNODE = sizeof(struct cnode);



static const char STAR_STR[] = "*";
static const char NULL_STR[] = "NULL";

// maybe_null(void *p) return a pointer to "NULL" or "*",
//   indicating if p is NULL or not.
static const char *maybe_null(void *p) {
  return p ? STAR_STR : NULL_STR;
}

// gapsize(n0, n1) determine the size (in bytes) of the gap between n0 and n1.
static size_t gapsize(struct cnode *n0, struct cnode *n1) {
  assert(n0);
  assert(n1);
  void *v0 = n0;
  void *v1 = n1;
  return (v1 - v0) - n0->nsize - sizeof(struct cnode);
}

// print_gapsize(n0, n1) print the size of the gap between n0 and n1,
//     if it's non-zero.
static void print_gapsize(struct cnode *n0, struct cnode *n1) {
  assert(n0);
  assert(n1);
  size_t gap = gapsize(n0, n1);
  
  if (gap != 0) { 
    printf("%zd byte gap\n", gap);
  }
}


// pretty_print_block(chs, size) Print size bytes, starting at chs,
//    in a human-readable format: printable characters other than backslash
//    are printed directly; other characters are escaped as \xXX
static void pretty_print_block(unsigned char *chs, int size) {
  assert(chs);
  for (int i = 0; i < size; i++) {
    printf(0x20 <= chs[i] && chs[i] < 0x80 && chs[i] != '\\'
           ? "%c" : "\\x%02X", chs[i]);
  }
  printf("\n");
}

// print_node(node) Print the contents of node and all nodes that
//    follow it.  Return a pointer to the last node.
static struct cnode *print_node(struct cnode *node) {
  while (node != NULL) {
    void *raw = node + 1;     // point at raw data that follows.
    printf("struct cnode\n");
    printf("    nsize: %ld\n", node->nsize);
    printf("    prev: %s\n", maybe_null(node->prev));
    printf("    next: %s\n",  maybe_null(node->next));

    printf("%zd byte chunk: ", node->nsize);
    
    pretty_print_block(raw, node->nsize);
    
    if (node->next == NULL) {
      return node;
    } else {
      print_gapsize(node, node->next);
      node = node->next;
    }
  }
  return NULL;
}



static void print_hr(void) {
    printf("----------------------------------------------------------------\n");
}

// print_debug(block) print a long message showing the content of block.
void print_debug(struct contiguous *block) {
  assert(block);
  void *raw = block;

  print_hr();
  printf("struct contiguous\n");
  printf("    first: %s\n", maybe_null(block->first));

  if (block->first == NULL) {
    size_t gap = block->upper_limit - raw - sizeof(struct contiguous);
    printf("%zd byte gap\n", gap);           
  } else {
    void *block_first = block->first;
    size_t gap = block_first - raw - sizeof(struct contiguous);
    if (gap) {
      printf("%zd byte gap\n", gap);
    }
  }
 
  struct cnode *lastnode = print_node(block->first);
  
  if (lastnode != NULL) {
    print_gapsize(lastnode, block->upper_limit);
  }

  print_hr();
}



struct contiguous *make_contiguous(size_t size) {
  assert(size >= sizeof(struct contiguous));

  void *block = malloc(size);

  struct contiguous *contig = block;
  contig->first = NULL;
  contig->upper_limit = block + size;

  for (int i = sizeof(struct contiguous); i < size; ++i) {
    char *entry = block + i;
    *entry = '$';
  }

  return contig;
}


void destroy_contiguous(struct contiguous *block) {
  if (block == NULL) {
    return;
  }
  if (block->first != NULL) {
    printf("Destroying non-empty block!\n");
  }
  free(block);
}


void cfree(void *p) {
  if (p == NULL) {
    return;
  }
  
  struct cnode *node = p - sizeof(struct cnode);

  if (node->prev == NULL && node->next == NULL) {
    node->block->first = NULL;
  } else if (node->prev == NULL) {
    node->block->first = node->next;
    node->next->prev = NULL;
  } else if (node->next == NULL) {
    node->prev->next = NULL;
  } else {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
}


void *cmalloc(struct contiguous *block, int size) {
  assert(block);
  assert(size >= 0);

  const int required = sizeof(struct cnode) + size;
  struct cnode *start = block->first;

  void * const b_pos = block;

  if (start == NULL && ((block->upper_limit - b_pos) - 
      sizeof(struct contiguous) >= required)) {
    void *new = b_pos + sizeof(struct contiguous);
    struct cnode *node = new;

    node->nsize = size;
    node->prev = NULL;
    node->next = NULL;
    node->block = block;
    block->first = node;

    return new + sizeof(struct cnode);
  } else if (start == NULL) {
    return NULL;
  }

  void *n_pos = start;

  if ((n_pos - b_pos) - sizeof(struct contiguous) >= required) {
    void *new = b_pos + sizeof(struct contiguous);
    struct cnode *node = new;

    node->nsize = size;
    node->prev = NULL;
    node->next = start;
    node->block = block;
    block->first = node;
    start->prev = node;

    return new + sizeof(struct cnode);
  }

  while (start->next != NULL) {
    if (gapsize(start, start->next) >= required) {
      void *new = n_pos + sizeof(struct cnode) + start->nsize;
      struct cnode *node = new;

      node->nsize = size;
      node->prev = start;
      node->next = start->next;
      node->block = block;

      node->next->prev = node;
      start->next = node;

      return new + sizeof(struct cnode);
    }
    start = start->next;
    n_pos = start;
  }

  if ((block->upper_limit - n_pos) - sizeof(struct cnode) - start->nsize >= 
      required) {
    void *new = n_pos + sizeof(struct cnode) + start->nsize;
    struct cnode *node = new;
    
    node->nsize = size;
    node->prev = start;
    node->next = NULL;
    node->block = block;

    start->next = node;
    return new + sizeof(struct cnode);
  }

  return NULL;
}


