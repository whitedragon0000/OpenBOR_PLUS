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
int scandir(const char *dir,
			struct dirent ***namelist,
			int (*filter)(const struct dirent *),
			int (*compar)(const struct dirent **,
						  const struct dirent **));

/*
*  Sort callback that compares directory entries by filename.
*/
int alphasort(const struct dirent **a, const struct dirent **b);

#ifdef __cplusplus
}
#endif

#endif /* defined(_WIN32) || defined(WIN32) */

#endif /* BORPAK_SCANDIR_H */