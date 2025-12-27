/*
 * Memory allocation wrappers
 *
 * All internal allocations go through these functions.
 * Users can override via mco_set_allocators() in the public API.
 *
 * Default: uses standard malloc/realloc/free
 */

#ifndef MCO_INTERNAL_ALLOCATOR_H
#define MCO_INTERNAL_ALLOCATOR_H

#include <stddef.h>

/* Internal allocation functions - use these everywhere internally */
void *mco_malloc(size_t size);
void *mco_realloc(void *ptr, size_t size);
void *mco_calloc(size_t count, size_t size);
void  mco_free(void *ptr);
char *mco_strdup(const char *str);

#endif /* MCO_INTERNAL_ALLOCATOR_H */
