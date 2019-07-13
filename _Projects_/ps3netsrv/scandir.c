#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "dirent2.h"

int alphasort(const void *_a, const void *_b);
int versionsort(const void *_a, const void *_b);
int scandir(const char *dirname,
	struct dirent2 ***ret_namelist,
	int (*select)(const struct dirent2 *),
	int (*compar)(const struct dirent2 **, const struct dirent2 **));

static const int FAILED	= -1;

/*
 * convenience helper function for scandir's |compar()| function:
 * sort directory entries using strcoll(3)
 */
int
alphasort(const void *_a, const void *_b)
{
	struct dirent2 **a = (struct dirent2 **)_a;
	struct dirent2 **b = (struct dirent2 **)_b;
	return strcoll((*a)->d_name, (*b)->d_name);
}


#define strverscmp(a,b) strcoll(a,b) /* for now */

/*
 * convenience helper function for scandir's |compar()| function:
 * sort directory entries using GNU |strverscmp()|
 */
int
versionsort(const void *_a, const void *_b)
{
	struct dirent2 **a = (struct dirent2 **)_a;
	struct dirent2 **b = (struct dirent2 **)_b;
	return strverscmp((*a)->d_name, (*b)->d_name);
}

/*
 * The scandir() function reads the directory dirname and builds an
 * array of pointers to directory entries using malloc(3).  It returns
 * the number of entries in the array.  A pointer to the array of
 * directory entries is stored in the location referenced by namelist.
 *
 * The select parameter is a pointer to a user supplied subroutine
 * which is called by scandir() to select which entries are to be
 * included in the array.  The select routine is passed a pointer to
 * a directory entry and should return a non-zero value if the
 * directory entry is to be included in the array.  If select is null,
 * then all the directory entries will be included.
 *
 * The compar parameter is a pointer to a user supplied subroutine
 * which is passed to qsort(3) to sort the completed array.  If this
 * pointer is null, the array is not sorted.
 */
int
scandir(const char *dirname,
		struct dirent2 ***ret_namelist,
		int (*select)(const struct dirent2 *),
		int (*compar)(const struct dirent2 **, const struct dirent2 **))
{
	int i, len;
	int used, allocated;
	DIR2 *dir;
	struct dirent2 *ent, *ent2;
	struct dirent2 **namelist = NULL;

	if ((dir = opendir2(dirname)) == NULL) return FAILED;

	used = 0;
	allocated = 2;
	namelist = malloc(allocated * sizeof(struct dirent2 *));
	if (!namelist)
		goto error;

	while ((ent = readdir2(dir)) != NULL)
	{
		if (select != NULL && !select(ent)) continue;

		/* duplicate struct direct for this entry */
		len = offsetof(struct dirent2, d_name) + strlen(ent->d_name) + 1;
		if ((ent2 = malloc(len)) == NULL) return FAILED;

		if (used >= allocated)
		{
			allocated *= 2;
			namelist = realloc(namelist, allocated * sizeof(struct dirent2 *));
			if (!namelist)
			goto error;
		}
		memcpy(ent2, ent, len);
		namelist[used++] = ent2;
	}
	closedir2(dir);

	if (compar)
		qsort(namelist, used, sizeof(struct dirent2 *),
			 (int (*)(const void *, const void *)) compar);

	*ret_namelist = namelist;
	return used;


error:
	if (namelist)
	{
		for (i = 0; i < used; i++)
			free(namelist[i]);

		free(namelist);
	}
	return FAILED;
}