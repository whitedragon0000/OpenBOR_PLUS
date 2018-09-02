/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef SAFEALLOC_H
#define SAFEALLOC_H

#include <stdlib.h>
#include <string.h> // for strlen
#include "utils.h" // for checkAlloc

#undef strdup

#define MALLOCLIKE __attribute__((__malloc__))
#define FREELIKE __attribute__((__cleanup__(free)))
//#define ATTR_UNUSED __attribute__((__unused__))

#ifdef DMALLOC_MODE
#define MAX_DMALLOCS 2000000
#define MAX_STR_BUF_LEN 128
struct dmalloc_info
{
    char func[MAX_STR_BUF_LEN];
    char file[MAX_STR_BUF_LEN];
    int line;
    short active;
};
struct dmalloc_info __mi[MAX_DMALLOCS];
size_t dmalloc_count;
#undef MAX_STR_BUF_LEN
#endif

// attributes can only be declared on function declarations, so declare these before defining them
static inline void *safeRealloc(void *ptr, size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void *safeMalloc(size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void safeFree(void* ptr, const char *func, const char *file, int line);
static inline void *safeStrdup(const char *str, const char *func, const char *file, int line) MALLOCLIKE;
#ifdef DMALLOC_MODE
static inline void print_dmalloc_info();
#endif

static inline void *safeRealloc(void *ptr, size_t size, const char *func, const char *file, int line)
{
    void *rptr = realloc(ptr, size);
#ifdef DMALLOC_MODE
    size_t hash = ((size_t)ptr) % MAX_DMALLOCS;
    __mi[hash].active = 0;
    hash = ((size_t)rptr) % MAX_DMALLOCS;
    __mi[hash].line = line;
    strcpy(__mi[hash].func, func);
    strcpy(__mi[hash].file, file);
    __mi[hash].active = 1;
#endif
    return checkAlloc(rptr, size, func, file, line);
}

static inline void *safeMalloc(size_t size, const char *func, const char *file, int line)
{
    void *ptr = malloc(size);
#ifdef DMALLOC_MODE
    size_t hash = ((size_t)ptr) % MAX_DMALLOCS;
    __mi[hash].line = line;
    strcpy(__mi[hash].func, func);
    strcpy(__mi[hash].file, file);
    __mi[hash].active = 1;
    ++dmalloc_count;
#endif
    return checkAlloc(ptr, size, func, file, line);
}

static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line)
{
    void *ptr = calloc(nmemb, size);
#ifdef DMALLOC_MODE
    size_t hash = ((size_t)ptr) % MAX_DMALLOCS;
    __mi[hash].line = line;
    strcpy(__mi[hash].func, func);
    strcpy(__mi[hash].file, file);
    __mi[hash].active = 1;
    ++dmalloc_count;
#endif
    return checkAlloc(ptr, size, func, file, line);
}

static inline void safeFree(void* ptr, const char *func, const char *file, int line)
{
#ifdef DMALLOC_MODE
    size_t hash = ((size_t)ptr) % MAX_DMALLOCS;
    __mi[hash].active = 0;
    --dmalloc_count;
#endif
    free(ptr);
}

static inline void *safeStrdup(const char *str, const char *func, const char *file, int line)
{
    // reimplement strdup to avoid doing strlen(str) twice
    int size = strlen(str) + 1;
    void *ptr = malloc(size);
    char *newString = (char *) checkAlloc(ptr, size, func, file, line);
    memcpy(newString, str, size);
#ifdef DMALLOC_MODE
    size_t hash = ((size_t)ptr) % MAX_DMALLOCS;
    __mi[hash].line = line;
    strcpy(__mi[hash].func, func);
    strcpy(__mi[hash].file, file);
    __mi[hash].active = 1;
    ++dmalloc_count;
#endif

    return newString;
}

#define realloc(ptr, size) safeRealloc(ptr, size, __func__, __FILE__, __LINE__)
#define malloc(size) safeMalloc(size, __func__, __FILE__, __LINE__)
#define calloc(nmemb, size) safeCalloc(nmemb, size, __func__, __FILE__, __LINE__)
#define free(ptr) safeFree(ptr, __func__, __FILE__, __LINE__)
#define strdup(str) safeStrdup(str, __func__, __FILE__, __LINE__)

#ifdef DMALLOC_MODE
static inline void print_dmalloc_info()
{
    int i;
    writeToLogFile("\n\n");
    writeToLogFile("////////////////////////////////////////////\n");
    writeToLogFile("///////////     DMALLOC INFO    ////////////\n");
    writeToLogFile("////////////////////////////////////////////\n");
    writeToLogFile("\n");
    writeToLogFile("total unfreed mallocs: %lu\n", dmalloc_count);
    for (i = 0; i < MAX_DMALLOCS; i++)
    {
        if (__mi[i].active)
        {
            writeToLogFile("unfreed malloc from %s:%d in function: %s\n",__mi[i].file,__mi[i].line,__mi[i].func);
        }
    }
}
#define PRINT_DMALLOC_INFO print_dmalloc_info()
#endif

#endif // !defined SAFEALLOC_H
