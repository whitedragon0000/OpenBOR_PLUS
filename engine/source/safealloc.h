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
#define MAX_DMALLOCS        (SIZE_MAX / 100000)
#define MAX_DMALLOC_LOGS            100
#define MAX_STR_BUF_LEN     128
typedef size_t DMADDR;
struct dmalloc_info
{
    char func[MAX_STR_BUF_LEN];
    char file[MAX_STR_BUF_LEN];
    int line;
    short active;
    DMADDR addr;
};
struct dmalloc_info __um[MAX_DMALLOCS];     // unfreed mallocs
struct dmalloc_info __um_ol[MAX_DMALLOCS];  // overflow list for unfreed mallocs
struct dmalloc_info __om[MAX_DMALLOCS];     // overwritten mallocs
long __um_count;
long __om_count;
int __is_um_full;
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
static int __dmalloc_add(void *ptr, const char *func, const char *file, int line);
static int __dmalloc_remove(void *ptr, const char *func, const char *file, int line);
#endif

static inline void *safeRealloc(void *ptr, size_t size, const char *func, const char *file, int line)
{
    void *rptr = realloc(ptr, size);
#ifdef DMALLOC_MODE
    __dmalloc_remove(ptr, func, file, line);
    __dmalloc_add(rptr, func, file, line);
#endif
    return checkAlloc(rptr, size, func, file, line);
}

static inline void *safeMalloc(size_t size, const char *func, const char *file, int line)
{
    void *ptr = malloc(size);
#ifdef DMALLOC_MODE
    __dmalloc_add(ptr, func, file, line);
#endif
    return checkAlloc(ptr, size, func, file, line);
}

static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line)
{
    void *ptr = calloc(nmemb, size);
#ifdef DMALLOC_MODE
    __dmalloc_add(ptr, func, file, line);
#endif
    return checkAlloc(ptr, size, func, file, line);
}

static inline void safeFree(void* ptr, const char *func, const char *file, int line)
{
#ifdef DMALLOC_MODE
    __dmalloc_remove(ptr, func, file, line);
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
    __dmalloc_add(ptr, func, file, line);
#endif

    return newString;
}

#define realloc(ptr, size) safeRealloc(ptr, size, __func__, __FILE__, __LINE__)
#define malloc(size) safeMalloc(size, __func__, __FILE__, __LINE__)
#define calloc(nmemb, size) safeCalloc(nmemb, size, __func__, __FILE__, __LINE__)
#define free(ptr) safeFree(ptr, __func__, __FILE__, __LINE__)
#define strdup(str) safeStrdup(str, __func__, __FILE__, __LINE__)

#ifdef DMALLOC_MODE
static DMADDR __dmalloc_hash(void *ptr)
{
    DMADDR hash = ((DMADDR)ptr) % MAX_DMALLOCS;
    return hash;
}

static int __dmalloc_add_overwritten_malloc(void *ptr, const char *func, const char *file, int line)
{
    DMADDR pos;

    if (__om_count >= MAX_DMALLOCS) return 0;
    pos = __om_count % MAX_DMALLOCS;
    __om[pos].line = line;
    __om[pos].addr = (DMADDR)ptr;
    strcpy(__om[pos].func, func);
    strcpy(__om[pos].file, file);
    ++__om_count;

    return 1;
}

static int __dmalloc_remove(void *ptr, const char *func, const char *file, int line)
{
    DMADDR hash;
    int i, ok = 0;


    hash = __dmalloc_hash(ptr);
    if(__um[hash].active && __um[hash].addr == (DMADDR)ptr)
    {
        if (__is_um_full) __is_um_full = 0;
        __um[hash].active = 0;
        if (ptr != NULL)
        {
            if (--__um_count < 0) __um_count = 0;
        }
    }
    else if (__um[hash].active) // use overflow list
    {
        for(i = 0; i < MAX_DMALLOCS; i++)
        {
            if(__um_ol[i].active && __um[i].addr == (DMADDR)ptr)
            {
                if (__is_um_full) __is_um_full = 0;
                __um_ol[i].active = 0;
                if (ptr != NULL)
                {
                    if (--__um_count < 0) __um_count = 0;
                }
                ok = 1;
                break;
            }
        }
        if(!ok) return 0;
    }
    else
    {
        return 0;
    }

    return 1;
}

static int __dmalloc_add(void *ptr, const char *func, const char *file, int line)
{
    DMADDR hash;
    int i, ok = 0;

    if (__is_um_full) return 0;

    hash = __dmalloc_hash(ptr);
    if(__um[hash].active && __um[hash].addr == (DMADDR)ptr) // use overflow list
    {
        if (__um[hash].addr == (DMADDR)ptr)
        {
            __dmalloc_add_overwritten_malloc(ptr, func, file, line);
        }

        for(i = 0; i < MAX_DMALLOCS; i++)
        {
            if(!__um_ol[i].active)
            {
                __um_ol[i].line = line;
                __um_ol[i].addr = (DMADDR)ptr;
                strcpy(__um_ol[i].func, func);
                strcpy(__um_ol[i].file, file);
                __um_ol[i].active = 1;
                ok = 1;
                break;
            }
        }
        if(!ok)
        {
            __is_um_full = 1;
            return 0;
        }

    }
    else if(!__um[hash].active)
    {
        __um[hash].line = line;
        __um[hash].addr = (DMADDR)ptr;
        strcpy(__um[hash].func, func);
        strcpy(__um[hash].file, file);
        __um[hash].active = 1;

        ++__um_count;
    }
    else
    {
        return 0;
    }

    return 1;
}

static inline void print_dmalloc_info()
{
    int i;
    size_t count = 0;
    writeToLogFile("\n\n");
    writeToLogFile("////////////////////////////////////////////\n");
    writeToLogFile("///////////     DMALLOC INFO    ////////////\n");
    writeToLogFile("////////////////////////////////////////////\n");
    writeToLogFile("\n");
    writeToLogFile("Total Unfreed Mallocs: %ld\n", __um_count);
    count = 0;
    if (__um_count > 0) writeToLogFile("First %d unfreed mallocs...\n", MAX_DMALLOC_LOGS);
    for (i = 0; i < MAX_DMALLOCS; i++)
    {
        if (__um[i].active)
        {
            writeToLogFile("Unfreed Malloc from %s:%d into function: %s at address: 0x%X\n",__um[i].file,__um[i].line,__um[i].func,__um[i].addr);
            if(++count > MAX_DMALLOC_LOGS) break;
        }
    }
    if(count < MAX_DMALLOC_LOGS)
    {
        for (i = 0; i < MAX_DMALLOCS; i++)
        {
            if (__um_ol[i].active)
            {
                writeToLogFile("Unfreed Malloc from %s:%d into function: %s at address: 0x%X\n",__um_ol[i].file,__um_ol[i].line,__um_ol[i].func,__um_ol[i].addr);
                if(++count > MAX_DMALLOC_LOGS) break;
            }
        }
    }
    #ifdef DMALLOC_OVERWRITTEN
    writeToLogFile("\n\n");
    writeToLogFile("Total Overwritten Mallocs: %ld\n", __om_count);
    count = 0;
    if (__om_count > 0) writeToLogFile("First %d overwritten mallocs...\n", MAX_DMALLOC_LOGS);
    for (i = 0; i < MAX_DMALLOCS; i++)
    {
        writeToLogFile("Overwritten Malloc from %s:%d into function: %s at address: 0x%X\n",__om[i].file,__om[i].line,__om[i].func,__om[i].addr);
        if(++count > MAX_DMALLOC_LOGS) break;
    }
    #endif
}
#define PRINT_DMALLOC_INFO print_dmalloc_info()
#endif

#endif // !defined SAFEALLOC_H
