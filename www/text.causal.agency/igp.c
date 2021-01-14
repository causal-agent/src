/* Copyright (C) 2021  June McEnroe <june@causal.agency>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <sys/capsicum.h>
#endif

static int compar(const void *_a, const void *_b) {
	const struct dirent *a = _a;
	const struct dirent *b = _b;
	if (a->d_type != b->d_type) {
		return (a->d_type > b->d_type) - (a->d_type < b->d_type);
	}
	return strcmp(a->d_name, b->d_name);
}

int main(int argc, char *argv[]) {
	int error;
	const char *host = NULL;
	const char *port = "70";
	for (int opt; 0 < (opt = getopt(argc, argv, "h:p:"));) {
		switch (opt) {
			break; case 'h': host = optarg;
			break; case 'p': port = optarg;
			break; default:  return EX_USAGE;
		}
	}
	if (optind == argc) return EX_USAGE;
	if (!host) {
		static char buf[256];
		error = gethostname(buf, sizeof(buf));
		if (error) abort();
		host = buf;
	}

	const char *path = argv[optind];
	int root = open(path, O_RDONLY | O_DIRECTORY);
	if (root < 0) err(EX_NOINPUT, "/");

#ifdef __FreeBSD__
	cap_rights_t cap;
	error = cap_enter()
		|| cap_rights_limit(STDIN_FILENO, cap_rights_init(&cap, CAP_READ))
		|| cap_rights_limit(STDOUT_FILENO, cap_rights_init(&cap, CAP_WRITE))
		|| cap_rights_limit(
			root, cap_rights_init(&cap, CAP_PREAD, CAP_FSTATAT, CAP_FSTATFS)
		);
	if (error) abort();
#else
#warning "This is completely insecure without capsicum(4)!"
#endif

	char buf[1024];
	if (!fgets(buf, sizeof(buf), stdin)) return EX_PROTOCOL;
	char *ptr = buf;
	char *sel = strsep(&ptr, "\t\r\n");
	if (sel[0] == '/') sel++;

	int fd = (sel[0] ? openat(root, sel, O_RDONLY) : root);
	if (fd < 0) err(EX_NOINPUT, "%s", sel);

	struct stat stat;
	error = fstat(fd, &stat);
	if (error) err(EX_IOERR, "%s", sel);
	if (!(stat.st_mode & (S_IFREG | S_IFDIR))) {
		errx(EX_NOINPUT, "%s: Not a file or directory", sel);
	}

	if (stat.st_mode & S_IFREG) {
#ifdef __FreeBSD__
		error = sendfile(fd, STDOUT_FILENO, 0, 0, NULL, NULL, 0);
		if (!error) return EX_OK;
#endif
		char buf[4096];
		for (ssize_t len; 0 < (len = read(fd, buf, sizeof(buf)));) {
			fwrite(buf, len, 1, stdout);
		}
		return EX_OK;
	}

	DIR *dir = fdopendir(fd);
	if (!dir) err(EX_IOERR, "%s", sel);

	size_t len = 0;
	size_t width = 0;
	static struct dirent ents[4096];
	for (struct dirent *ent; len < 4096 && (ent = readdir(dir));) {
		if (ent->d_name[0] == '.') continue;
		if (ent->d_type != DT_REG && ent->d_type != DT_DIR) continue;
		if (ent->d_namlen > width) width = ent->d_namlen;
		ents[len++] = *ent;
	}

	qsort(ents, len, sizeof(ents[0]), compar);
	for (size_t i = 0; i < len; ++i) {
		char mtime[26] = "";
		if (ents[i].d_type == DT_REG) {
			error = fstatat(fd, ents[i].d_name, &stat, 0);
			if (error) err(EX_IOERR, "%s/%s", sel, ents[i].d_name);
			ctime_r(&stat.st_mtime, mtime);
			mtime[24] = '\0';
		}
		printf(
			"%c%-*s  %s\t%s%s%s\t%s\t%s\r\n",
			(ents[i].d_type == DT_DIR ? '1' : '0'),
			(int)width, ents[i].d_name, mtime,
			sel, (sel[0] ? "/" : ""), ents[i].d_name, host, port
		);
	}

	printf("i-- \t\t%s\t%s\r\n", host, port);
	printf("0Served by IGP (AGPLv3)\tigp.c\ttext.causal.agency\t70\r\n");
	printf(".\r\n");
}
