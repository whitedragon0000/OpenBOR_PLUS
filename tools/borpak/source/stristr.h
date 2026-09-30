/*
	Copyright (c) 2010 Bryan Cain

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

	http://www.gnu.org/licenses/gpl.txt
*/

#ifndef STRISTR_H
#define STRISTR_H

#if defined(__cplusplus) && __cplusplus
extern "C" {
#endif

/*
*  Searches haystack for needle without matching letter case.
*  Returns a pointer to the first match, or NULL if no match is found.
*/
char *stristr(const char *haystack, const char *needle);

#if defined(__cplusplus) && __cplusplus
}
#endif

#endif /* STRISTR_H */