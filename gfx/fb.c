/* Copyright (C) 2018  June McEnroe <june@causal.agency>
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

#include <err.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

#include "gfx.h"

static struct termios saveTerm;
static void restoreTerm(void) {
	tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
	int error;

	error = init(argc, argv);
	if (error) return error;

	const char *path = getenv("FRAMEBUFFER");
	if (!path) path = "/dev/fb0";

	int fb = open(path, O_RDWR);
	if (fb < 0) err(EX_OSFILE, "%s", path);

	struct fb_var_screeninfo info;
	error = ioctl(fb, FBIOGET_VSCREENINFO, &info);
	if (error) err(EX_IOERR, "%s", path);

	size_t size = 4 * info.xres * info.yres;
	uint32_t *buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	if (buf == MAP_FAILED) err(EX_IOERR, "%s", path);

	error = tcgetattr(STDERR_FILENO, &saveTerm);
	if (error) err(EX_IOERR, "tcgetattr");
	atexit(restoreTerm);

	struct termios term = saveTerm;
	term.c_lflag &= ~(ICANON | ECHO);
	error = tcsetattr(STDERR_FILENO, TCSADRAIN, &term);
	if (error) err(EX_IOERR, "tcsetattr");

	uint32_t saveBg = buf[0];

	uint32_t back[info.xres * info.yres];
	for (;;) {
		draw(back, info.xres, info.yres);
		memcpy(buf, back, size);

		char in;
		ssize_t len = read(STDERR_FILENO, &in, 1);
		if (len < 0) err(EX_IOERR, "read");
		if (!len) return EX_DATAERR;

		if (!input(in)) {
			for (uint32_t i = 0; i < info.xres * info.yres; ++i) {
				buf[i] = saveBg;
			}
			fprintf(stderr, "%s\n", status());
			return EX_OK;
		}
	}
}
