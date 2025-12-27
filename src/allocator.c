/*
 * Memory allocation wrappers
 *
 * All internal allocations go through these functions.
 * Users can override via mco_set_allocators() for custom memory management.
 */

#include "internal/allocator.h"
#include "mcoptions.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Global Allocator State
 *============================================================================*/

static struct {
    mco_malloc_fn  f_malloc;
    mco_realloc_fn f_realloc;
    mco_free_fn    f_free;
} g_allocators = {
    .f_malloc  = malloc,
    .f_realloc = realloc,
    .f_free    = free
};

/*============================================================================
 * Public API
 *============================================================================*/

void mco_set_allocators(mco_malloc_fn f_malloc,
                        mco_realloc_fn f_realloc,
                        mco_free_fn f_free)
{
    g_allocators.f_malloc  = f_malloc  ? f_malloc  : malloc;
    g_allocators.f_realloc = f_realloc ? f_realloc : realloc;
    g_allocators.f_free    = f_free    ? f_free    : free;
}

/*============================================================================
 * Internal Allocation Functions
 *============================================================================*/

void *mco_malloc(size_t size)
{
    return g_allocators.f_malloc(size);
}

void *mco_realloc(void *ptr, size_t size)
{
    return g_allocators.f_realloc(ptr, size);
}

void *mco_calloc(size_t count, size_t size)
{
    size_t total = count * size;
    void *ptr = g_allocators.f_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void mco_free(void *ptr)
{
    if (ptr) {
        g_allocators.f_free(ptr);
    }
}

char *mco_strdup(const char *str)
{
    if (!str) return NULL;

    size_t len = strlen(str) + 1;
    char *copy = (char *)g_allocators.f_malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}
