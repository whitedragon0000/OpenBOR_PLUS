/*
** Designation:  StriStr
**
** Call syntax:  char *stristr(const char *haystack, const char *needle)
**
** Description:  This function is an ANSI version of strstr() with
**               case insensitivity.
**
** Return item:  char *pointer to the first match if needle is found
**               in haystack, else NULL
**
** Rev History:  16/07/97  Greg Thayer  Optimized
**               07/04/95  Bob Stout    ANSI-fy
**               02/03/94  Fred Cole    Original
**               09/01/03  Bob Stout    Bug fix (lines 40-41) per Fred Bulback
**               2026-06-10  Damon V. Caskey  Matched prototypes, fixed empty-pattern handling,
**                                       made ctype use safe, and documented internals.
**
** Hereby donated to public domain.
*/

#include "stristr.h"

#include <ctype.h>
#include <stddef.h>

#ifdef TEST
#include <stdio.h>
#endif

#define NUL '\0'

/*
*  Converts a character to uppercase safely for ctype functions.
*  The cast avoids undefined behavior when char is signed.
*/
static int stristr_toupper(char c)
{
	return toupper((unsigned char)c);
}

/*
*  Searches haystack for needle without matching letter case.
*  Mirrors strstr() behavior, including returning haystack for an empty needle.
*/
char *stristr(const char *haystack, const char *needle)
{
	const char *start;
	const char *haystack_cursor;
	const char *needle_cursor;

	/*
	*  Match strstr() behavior for an empty search pattern.
	*/
	if (*needle == NUL)
	{
		return (char *)haystack;
	}

	for (start = haystack; *start != NUL; start++)
	{
		/*
		*  Move to the next possible first-character match.
		*/
		if (stristr_toupper(*start) != stristr_toupper(*needle))
		{
			continue;
		}

		haystack_cursor = start;
		needle_cursor = needle;

		/*
		*  Compare both strings from the candidate position.
		*/
		while (*haystack_cursor != NUL &&
			   *needle_cursor != NUL &&
			   stristr_toupper(*haystack_cursor) == stristr_toupper(*needle_cursor))
		{
			haystack_cursor++;
			needle_cursor++;
		}

		/*
		*  Reaching the end of needle means the full pattern matched.
		*/
		if (*needle_cursor == NUL)
		{
			return (char *)start;
		}
	}

	return NULL;
}

#ifdef TEST

int main(void)
{
	int i;
	const char *buffer[2] = {"heLLo, HELLO, hello, hELLo, HellO", "Hell"};
	const char *sptr;

	for (i = 0; i < 2; ++i)	{
		printf("\nTest string=\"%s\"\n", sptr = buffer[i]);

		while ((sptr = stristr(sptr, "hello")) != NULL)	{
			printf("Testing %s:\n", sptr);
			printf("Found %5.5s!\n", sptr++);
		}
	}

	return 0;
}

#endif /* TEST */