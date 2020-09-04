/* Copyright (C) 2020  June McEnroe <june@causal.agency>
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
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

static int shake = 10;
static int delay = 20000;
static int count = 2;

static void move(int x, int y) {
	fprintf(stderr, "\33[3;%d;%dt", x, y);
	usleep(delay);
}

int main(int argc, char *argv[]) {
	for (int opt; 0 < (opt = getopt(argc, argv, "n:s:t:"));) {
		switch (opt) {
			break; case 'n': count = atoi(optarg);
			break; case 's': shake = atoi(optarg);
			break; case 't': delay = atoi(optarg) * 1000;
			break; default:  return EX_USAGE;
		}
	}

	struct termios save;
	int error = tcgetattr(STDIN_FILENO, &save);
	if (error) err(EX_IOERR, "tcgetattr");

	struct termios raw = save;
	cfmakeraw(&raw);
	error = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	if (error) err(EX_IOERR, "tcsetattr");

	int x, y;
	fprintf(stderr, "\33[13t");
	int n = scanf("\33[3;%d;%dt", &x, &y);

	error = tcsetattr(STDIN_FILENO, TCSANOW, &save);
	if (error) err(EX_IOERR, "tcsetattr");
	if (n < 2) return EX_CONFIG;

	fprintf(stderr, "\33[5t");
	for (int i = 0; i < count; ++i) {
		move(x - shake, y);
		move(x, y + shake);
		move(x + shake, y);
		move(x, y - shake);
		move(x, y);
	}
}
