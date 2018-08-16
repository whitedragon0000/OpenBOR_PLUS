int scandir(const char *dir, struct dirent ***namelist,
						int (*select)(const struct dirent *),
						int (*compar)(const void *, const void *));
int alphasort(const void *a, const void *b);

