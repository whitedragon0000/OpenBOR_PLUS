#ifndef BORPAK_SCANDIR_H
#define BORPAK_SCANDIR_H 1

#include <dirent.h>

#if defined(_WIN32) || defined(WIN32)

#ifdef __cplusplus
extern "C" {
#endif

/*
*  Scans a directory and returns an allocated list of matching entries.
*  The caller owns the returned array and each entry inside it.
*/
int scandir(const char *dir, struct dirent ***namelist,
						int (*select)(const struct dirent *),
						int (*compar)(const void *, const void *));

/*
*  Sort callback that compares directory entries by filename.
*/
int alphasort(const void *a, const void *b);

#ifdef __cplusplus
}
#endif

#endif /* defined(_WIN32) || defined(WIN32) */

#endif /* BORPAK_SCANDIR_H */
