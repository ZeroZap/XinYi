#include <stddef.h>
#include <stdlib.h>

void *xy_malloc(size_t size)
{
    return malloc(size);
}

void *xy_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void *xy_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void xy_free(void *ptr)
{
    free(ptr);
}

void xy_safe_free(void **ptr)
{
    if (ptr && *ptr) {
        xy_free(*ptr);
        *ptr = NULL;
    }
}
