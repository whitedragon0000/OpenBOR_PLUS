// URL: http://www.koders.com/c/fid67F09A5A42BB1F6B563434398602BF78C930F0C0.aspx?s=TV+Raman
// Released under the GPL as part of the Viewmol project

/*******************************************************************************
*                                                                              *
*                                   Viewmol                                    *
*                                                                              *
*                              S C A N D I R . C                               *
*                                                                              *
*                 Copyright (c) Joerg-R. Hill, October 2003                    *
*                                                                              *
********************************************************************************
*
* $Id: scandir.c,v 1.3 2003/11/07 11:15:53 jrh Exp $
*
* Caskey, Damon V.
* 2026-06-10
*
* Reworked to catch integer overflow, add error handling,
* and document internal behavior. Also added a simple insertion
* sort to avoid casting the caller's comparison callback
* to qsort's different function pointer type.
*/

#if defined(_WIN32) || defined(WIN32)

#include "scandir.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
*  Frees a partially or fully allocated scandir result list.
*  Used on error paths so previously copied entries do not leak.
*/
static void scandir_free_entries(struct dirent **namelist, size_t count)
{
	size_t i;

	if (namelist == NULL)
	{
		return;
	}

	for (i = 0; i < count; i++)
	{
		free(namelist[i]);
	}

	free(namelist);
}

/*
* Calculates the number of bytes needed to copy a 
* directory entry. The Windows dirent shim uses a 
* fixed-size structure, so copy the whole entry.
*
* Caskey, Damon V.
* 2026-06-10
*
* Simplified to return the fixed size of struct dirent since
* the Windows shim does not support variable-length entries.
* Whoever wrote the original is a lot smarter than I am, but 
* it assumes d_name is the final field in every dirent layout.
* For a packing tool, I don't think the optimization is worth
* the risk of breaking on some platform's dirent implementation.
*/
static size_t scandir_entry_size(const struct dirent *entry) {
	(void)entry;

	return sizeof(struct dirent);
}

/*
*  Sorts the collected entries using the scandir-compatible comparison callback.
*  This avoids casting the callback to qsort's different function pointer type.
*/
static void scandir_sort_entries(struct dirent **entries,
								 size_t count,
								 int (*compar)(const struct dirent **,
											   const struct dirent **))
{
	size_t i;

	if (entries == NULL || compar == NULL || count < 2)	{
		return;
	}

	for (i = 1; i < count; i++)	{
		struct dirent *current = entries[i];
		size_t j = i;

		while (j > 0) {
			
			const struct dirent *left = entries[j - 1];
			const struct dirent *right = current;

			if (compar(&left, &right) <= 0)
			{
				break;
			}

			entries[j] = entries[j - 1];
			j--;
		}

		entries[j] = current;
	}
}

/*
*  Windows compatibility implementation of POSIX scandir().
*  Reads a directory, optionally filters entries, copies matches, and sorts them.
*/
int scandir(const char *dir,
			struct dirent ***namelist,
			int (*filter)(const struct dirent *),
			int (*compar)(const struct dirent **,
						  const struct dirent **))
{
	DIR *directory;
	struct dirent *entry;
	struct dirent **entries;
	struct dirent **resized_entries;
	size_t count;
	size_t entry_size;
	int saved_errno;

	/*
	*  Caller must provide a valid output pointer.
	*  The output is initialized early so callers never see stale data.
	*/
	if (namelist == NULL) {
		errno = EINVAL;
		return -1;
	}

	*namelist = NULL;

	directory = opendir(dir);
	if (directory == NULL) {
		return -1;
	}

	entries = NULL;
	count = 0;

	for (;;){

		/*
		*  readdir() returns NULL both at end-of-directory and on error.
		*  Clearing errno lets us distinguish those two cases.
		*/
		errno = 0;
		entry = readdir(directory);

		if (entry == NULL)	{
			if (errno != 0)	{
				goto error;
			}

			break;
		}

		/*
		*  Skip entries rejected by the caller's filter function.
		*/
		if (filter != NULL && !filter(entry)) {
			continue;
		}

		/*
		*  Prevent integer overflow before growing the result array.
		*  scandir() returns int, so count also needs to stay in range.
		*/
		if (count >= (size_t)INT_MAX ||
			count + 1 > ((size_t)-1) / sizeof(*entries)) {
			errno = ENOMEM;
			goto error;
		}

		resized_entries = (struct dirent **)realloc(entries,
													(count + 1) * sizeof(*entries));

		if (resized_entries == NULL) {
			errno = ENOMEM;
			goto error;
		}

		entries = resized_entries;
		entry_size = scandir_entry_size(entry);

		/*
		*  readdir() reuses its internal buffer, so every accepted entry
		*  must be copied before the next call to readdir().
		*/
		entries[count] = (struct dirent *)malloc(entry_size);

		if (entries[count] == NULL)	{
			errno = ENOMEM;
			goto error;
		}

		memcpy(entries[count], entry, entry_size);
		count++;
	}

	/*
	*  closedir() can fail, so keep the regular cleanup path available.
	*/
	if (closedir(directory) != 0) {
		directory = NULL;

		if (errno == 0) {
			errno = EIO;
		}

		goto error;
	}

	directory = NULL;

	scandir_sort_entries(entries, count, compar);

	/*
	*  A directory with no selected entries is a successful scan.
	*  The caller receives NULL and a return value of 0.
	*/
	*namelist = entries;

	return (int)count;

error:
	saved_errno = errno;

	if (directory != NULL)	{
		closedir(directory);
		directory = NULL;
	}

	scandir_free_entries(entries, count);
	*namelist = NULL;

	errno = saved_errno != 0 ? saved_errno : ENOMEM;

	return -1;
}

/*
*  Compares two directory entries by filename.
*  Used as the default alphabetical sort callback for this scandir shim.
*/
int alphasort(const struct dirent **a, const struct dirent **b) {
	return strcmp((*a)->d_name, (*b)->d_name);
}

#endif /* defined(_WIN32) || defined(WIN32) */