#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

static const char *path[1024];
static size_t depth;

static void traverse(int parent, const char *name) {
	path[depth++] = name;

	int fd = openat(parent, name, O_RDONLY);
	if (fd < 0) err(EX_NOINPUT, "%s", name);

	DIR *dir = fdopendir(fd);
	if (!dir) err(EX_IOERR, "%s", name);

	struct dirent *entry;
	while (NULL != (entry = readdir(dir))) {
		if (entry->d_namlen == 1 && entry->d_name[0] == '.') continue;
		if (
			entry->d_namlen == 2
			&& entry->d_name[0] == '.'
			&& entry->d_name[1] == '.'
		) continue;

		if (entry->d_type == DT_DIR) {
			traverse(fd, entry->d_name);
		}

		for (size_t i = 0; i < depth; ++i) {
			printf("%s/", path[i]);
		}
		printf("%s\n", entry->d_name);
	}
	// TODO: Check error.

	closedir(dir);
	depth--;
}

int main(int argc, char *argv[]) {
	if (argc < 2) return EX_USAGE;
	traverse(AT_FDCWD, argv[1]);
	return EX_OK;
}
