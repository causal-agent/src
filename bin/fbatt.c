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

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>

#include "scheme.h"

static const char *Class = "/sys/class/power_supply";

static const uint32_t Right  = 5 * 8 + 1; // fbclock width.
static const uint32_t Width  = 8;
static const uint32_t Height = 16;

int main() {
	int error;

	DIR *dir = opendir(Class);
	if (!dir) err(EX_OSFILE, "%s", Class);

	FILE *chargeFull = NULL;
	FILE *chargeNow = NULL;

	const struct dirent *entry;
	while (NULL != (errno = 0, entry = readdir(dir))) {
		if (entry->d_name[0] == '.') continue;

		error = chdir(Class);
		if (error) err(EX_OSFILE, "%s", Class);

		error = chdir(entry->d_name);
		if (error) err(EX_OSFILE, "%s/%s", Class, entry->d_name);

		chargeFull = fopen("charge_full", "r");
		chargeNow = fopen("charge_now", "r");
		if (chargeFull && chargeNow) break;
	}
	if (!chargeFull || !chargeNow) {
		if (errno) err(EX_OSFILE, "%s", Class);
		errx(EX_CONFIG, "%s: empty", Class);
	}
	closedir(dir);

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

	for (;;) {
		int match;

		rewind(chargeFull);
		fflush(chargeFull);
		uint32_t full;
		match = fscanf(chargeFull, "%u", &full);
		if (match == EOF) err(EX_IOERR, "charge_full");
		if (match < 1) errx(EX_DATAERR, "charge_full");

		rewind(chargeNow);
		fflush(chargeNow);
		uint32_t now;
		match = fscanf(chargeNow, "%u", &now);
		if (match == EOF) err(EX_IOERR, "charge_now");
		if (match < 1) errx(EX_DATAERR, "charge_now");

		uint32_t percent = 100 * now / full;
		uint32_t height = 16 * now / full;

		for (int i = 0; i < 60; ++i, sleep(1)) {
			uint32_t left = info.xres - Right - Width;

			for (uint32_t y = 0; y <= Height; ++y) {
				buf[y * info.xres + left - 1] = DarkWhite;
				buf[y * info.xres + left + Width] = DarkWhite;
			}
			for (uint32_t x = left; x < left + Width; ++x) {
				buf[Height * info.xres + x] = DarkWhite;
			}

			for (uint32_t y = 0; y < Height; ++y) {
				for (uint32_t x = left; x < left + Width; ++x) {
					buf[y * info.xres + x] =
						(Height - 1 - y > height) ? DarkBlack
						: (percent <= 10) ? DarkRed
						: (percent <= 30) ? DarkYellow
						: LightBlack;
				}
			}
		}
	}
}
