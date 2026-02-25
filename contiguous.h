#include <stddef.h>

struct contiguous;

// We need to make these available solely for testing.
extern const int SIZEOF_CONTIGUOUS;
extern const int SIZEOF_CNODE;


// make_contiguous(size) Create a block including a buffer of size.
// This function does call malloc.
// requires: size is greater than SIZEOF_CONTIGUOUS
// effects: allocates memory.  Caller must call destroy_contiguous.
// time: O(n)
struct contiguous *make_contiguous(size_t size);

// destroy_contiguous(block) Cleans up block.
// effects: calls free.
// time: O(1)
void destroy_contiguous(struct contiguous *block);

// cmalloc(block, size) Inside block, make a region of size bytes, and
// return a pointer to it.  If there is not enough space,
// return NULL.
// requires: block is not NULL
//           size >= 0
// time: O(n), where n is the number of nodes within block
void *cmalloc(struct contiguous *block, int size);

// cfree(p) Remove the node for which p points to its data.
// time: O(1)
void cfree(void *p);


// print_debug(block) print a long message showing the content of block.
void print_debug(struct contiguous *block);
