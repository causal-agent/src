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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

static void writeAll(int fd, const uint8_t *ptr, size_t size) {
	while (size) {
		ssize_t ret = write(fd, ptr, size);
		if (ret < 0) err(EX_IOERR, "write");
		ptr += ret;
		size -= ret;
	}
}
static void readAll(int fd, uint8_t *ptr, size_t size) {
	while (size) {
		ssize_t ret = read(fd, ptr, size);
		if (ret < 0) err(EX_IOERR, "read");
		if (!ret) errx(EX_DATAERR, "unexpected eof");
		ptr += ret;
		size -= ret;
	}
}

static void req2(int fd, uint8_t c) {
	uint8_t buf[] = { 0x1B, c };
	writeAll(fd, buf, 2);
}
static uint8_t res2(int fd) {
	uint8_t buf[2];
	readAll(fd, buf, 2);
	if (buf[0] != 0x1B) errx(EX_PROTOCOL, "response fail %hhX", buf[0]);
	return buf[1];
}

static void req3(int fd, uint8_t c, uint8_t d) {
	uint8_t buf[] = { 0x1B, c, d };
	writeAll(fd, buf, 3);
}
static struct Res3 { uint8_t c, d; } res3(int fd) {
	uint8_t buf[3];
	readAll(fd, buf, 3);
	if (buf[0] != 0x1B) errx(EX_PROTOCOL, "response fail %hhX", buf[0]);
	return (struct Res3) { buf[1], buf[2] };
}

static void reset(int tty) {
	req2(tty, 'a');
}

static void test(int tty) {
	uint8_t res;

	req2(tty, 'e');
	res = res2(tty);
	if (res != 'y') errx(EX_PROTOCOL, "test fail %hhX", res);

	req2(tty, 0x87);
	res = res2(tty);
	if (res != '0') errx(EX_PROTOCOL, "ram fail %hhX", res);

	req2(tty, 't');
	struct Res3 model = res3(tty);
	printf("model: %c%c\n", model.c, model.d);

	req2(tty, 'v');
	res = res2(tty);
	printf("firmware: %hhu\n", res);
}

static void sensorTest(int tty) {
	req2(tty, 0x86);
	uint8_t res = res2(tty);
	if (res != '0') errx(EX_PROTOCOL, "sensor fail %hhX", res);
}

static void led(int tty, char c) {
	switch (c) {
		break; case 'a': req2(tty, 0x82);
		break; case 'g': req2(tty, 0x83);
		break; case 'y': req2(tty, 0x84);
		break; case 'r': req2(tty, 0x85);
		break; default:  req2(tty, 0x81);
	}
}

static void status(int tty) {
	uint8_t res = res2(tty);
	switch (res) {
		break; case '0': return;
		break; case '1': errx(EX_DATAERR, "read/write error");
		break; case '2': errx(EX_DATAERR, "command format error");
		break; case '4': errx(EX_DATAERR, "invalid command");
		break; case '9': errx(EX_DATAERR, "invalid card swipe");
		break; default:  errx(EX_PROTOCOL, "status fail %hhX", res);
	}
}

static void readCard(int tty) {
	req2(tty, 'r');
	uint8_t res = res2(tty);
	if (res != 's') errx(EX_PROTOCOL, "read fail %hhX", res);
	for (;;) {
		readAll(tty, &res, 1);
		if (res == '?') {
			readAll(tty, &res, 1);
			if (res == 0x1C) break;
			printf("?");
		} else {
			printf("%c", res);
		}
	}
	status(tty);
}

static void writeCard(int tty) {
	req2(tty, 'w');
	req2(tty, 's');
	uint8_t buf[2];
	ssize_t size;
	while (0 < (size = read(STDIN_FILENO, buf, 2))) {
		writeAll(tty, buf, 2);
	}
	if (size < 0) err(EX_IOERR, "(stdin)");
	buf[0] = '?';
	buf[1] = 0x1C;
	writeAll(tty, buf, 2);
	status(tty);
}

static void eraseCard(int tty, char c) {
	req3(tty, 'c', c - '0');
	uint8_t res = res2(tty);
	if (res != '0') errx(EX_PROTOCOL, "erase fail %hhX", res);
}

static void setHiCo(int tty) {
	req2(tty, 'x');
	uint8_t res = res2(tty);
	if (res != '0') errx(EX_PROTOCOL, "hi-co fail %hhX", res);
}
static void setLoCo(int tty) {
	req2(tty, 'y');
	uint8_t res = res2(tty);
	if (res != '0') errx(EX_PROTOCOL, "lo-co fail %hhX", res);
}
static void getCo(int tty) {
	req2(tty, 'd');
	uint8_t res = res2(tty);
	switch (res) {
		break; case 'h': printf("hi-co\n");
		break; case 'l': printf("lo-co\n");
		break; default:  errx(EX_PROTOCOL, "get co fail %hhX", res);
	}
}

int main(int argc, char *argv[]) {
	const char *file = "/dev/ttyUSB0";
	char func = 't';
	const char *arg = NULL;

	int opt;
	while (0 < (opt = getopt(argc, argv, "L:RTce:f:hlrtw"))) {
		switch (opt) {
			break; case 'f': file = optarg;
			break; case '?': return EX_USAGE;
			break; default:  func = opt; arg = optarg;
		}
	}

	int tty = open(file, O_RDWR);
	if (tty < 0) err(EX_NOINPUT, "%s", file);

	struct termios attr;
	int error = tcgetattr(tty, &attr);
	if (error) err(EX_IOERR, "tcgetattr");

	cfmakeraw(&attr);
	error = tcsetattr(tty, TCSANOW, &attr);
	if (error) err(EX_IOERR, "tcsetattr");

	switch (func) {
		break; case 'L': led(tty, arg[0]);
		break; case 'R': reset(tty);
		break; case 'T': sensorTest(tty);
		break; case 'c': getCo(tty);
		break; case 'e': eraseCard(tty, arg[0]);
		break; case 'h': setHiCo(tty);
		break; case 'l': setLoCo(tty);
		break; case 'r': readCard(tty);
		break; case 't': test(tty);
		break; case 'w': writeCard(tty);
		break; default:  return EX_USAGE;
	}
}
