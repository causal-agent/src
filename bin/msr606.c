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

static int tty = -1;

static void ttyWrite(const uint8_t *ptr, size_t size) {
	ssize_t ret = write(tty, ptr, size);
	if (ret < 0) err(EX_IOERR, "write");
}
static void ttyRead(uint8_t *ptr, size_t size) {
	while (size) {
		ssize_t ret = read(tty, ptr, size);
		if (ret < 0) err(EX_IOERR, "read");
		ptr += ret;
		size -= ret;
	}
}

static void reset(void) {
	uint8_t buf[] = { 0x1B, 'a' };
	ttyWrite(buf, 2);
}

static void test(void) {
	uint8_t buf[3] = { 0x1B, 'e' };
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] != 'y') errx(EX_PROTOCOL, "test fail %hhX", buf[1]);

	buf[1] = 0x87;
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] != '0') errx(EX_PROTOCOL, "ram fail %hhX", buf[1]);

	buf[1] = 't';
	ttyWrite(buf, 2);
	ttyRead(buf, 3);
	printf("model: %c%c\n", buf[1], buf[2]);

	buf[1] = 'v';
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	printf("firmware: %hhu\n", buf[1]);
}

static void sensorTest(void) {
	uint8_t buf[] = { 0x1B, 0x86 };
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] != '0') errx(EX_PROTOCOL, "sensor fail %hhX", buf[1]);
}

static char led;
static void setLed(void) {
	uint8_t buf[2] = { 0x1B };
	switch (led) {
		break; case 'a': buf[1] = 0x82;
		break; case 'g': buf[1] = 0x83;
		break; case 'y': buf[1] = 0x84;
		break; case 'r': buf[1] = 0x85;
		break; default:  buf[1] = 0x81;
	}
	ttyWrite(buf, 2);
}

static void status(void) {
	uint8_t buf[2];
	ttyRead(buf, 2);
	switch (buf[1]) {
		break; case '0': return;
		break; case '1': errx(EX_DATAERR, "read/write error");
		break; case '2': errx(EX_DATAERR, "command format error");
		break; case '4': errx(EX_DATAERR, "invalid command");
		break; case '9': errx(EX_DATAERR, "invalid card swipe");
		break; default:  errx(EX_PROTOCOL, "status fail %hhX", buf[1]);
	}
}

static void readCard(void) {
	uint8_t buf[] = { 0x1B, 'r' };
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] != 's') errx(EX_PROTOCOL, "read fail %hhX", buf[1]);
	for (;;) {
		ttyRead(buf, 1);
		if (buf[0] == '?') {
			ttyRead(buf, 1);
			if (buf[0] == 0x1C) break;
			printf("?");
		} else {
			printf("%c", buf[0]);
		}
	}
	status();
}

static void writeCard(void) {
	uint8_t buf[] = { 0x1B, 'w', 0x1B, 's' };
	ttyWrite(buf, 4);
	ssize_t size;
	while (0 < (size = read(STDIN_FILENO, buf, 1))) {
		ttyWrite(buf, 1);
	}
	if (size < 0) err(EX_IOERR, "(stdin)");
	buf[0] = '?';
	buf[1] = 0x1C;
	ttyWrite(buf, 2);
	status();
}

static char erase;
static void eraseCard(void) {
	uint8_t buf[] = { 0x1B, 'c', erase - '0' };
	ttyWrite(buf, 3);
	ttyRead(buf, 2);
	if (buf[1] != '0') errx(EX_DATAERR, "erase fail %hhX", buf[1]);
}

static void setHiCo(void) {
	uint8_t buf[] = { 0x1B, 'x' };
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] != '0') errx(EX_PROTOCOL, "hi-co fail %hhX", buf[1]);
}
static void setLoCo(void) {
	uint8_t buf[] = { 0x1B, 'y' };
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] != '0') errx(EX_PROTOCOL, "lo-co fail %hhX", buf[1]);
}
static void getCo(void) {
	uint8_t buf[] = { 0x1B, 'd' };
	ttyWrite(buf, 2);
	ttyRead(buf, 2);
	if (buf[1] == 'h') printf("hi-co\n");
	else if (buf[1] == 'l') printf("lo-co\n");
	else errx(EX_PROTOCOL, "get co fail %hhX", buf[1]);
}

int main(int argc, char *argv[]) {
	const char *file = "/dev/ttyUSB0";
	void (*func)(void) = test;

	int opt;
	while (0 < (opt = getopt(argc, argv, "L:RTce:f:hlrtw"))) {
		switch (opt) {
			break; case 'L': led = optarg[0]; func = setLed;
			break; case 'R': func = reset;
			break; case 'T': func = sensorTest;
			break; case 'c': func = getCo;
			break; case 'e': erase = optarg[0]; func = eraseCard;
			break; case 'f': file = optarg;
			break; case 'h': func = setHiCo;
			break; case 'l': func = setLoCo;
			break; case 'r': func = readCard;
			break; case 't': func = test;
			break; case 'w': func = writeCard;
			break; default: return EX_USAGE;
		}
	}

	tty = open(file, O_RDWR);
	if (tty < 0) err(EX_NOINPUT, "%s", file);

	struct termios attr;
	int error = tcgetattr(tty, &attr);
	if (error) err(EX_IOERR, "tcgetattr");

	cfmakeraw(&attr);
	error = tcsetattr(tty, TCSANOW, &attr);
	if (error) err(EX_IOERR, "tcsetattr");

	func();
}
