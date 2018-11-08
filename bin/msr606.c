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
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

enum {
	Esc = 0x1B,
	FS  = 0x1C,
};

static const char *path = "/dev/ttyUSB0";
static FILE *tty;

static char ttyGet(void) {
	int ch = fgetc(tty);
	if (ferror(tty)) err(EX_IOERR, "%s", path);
	if (ch == EOF) errx(EX_PROTOCOL, "unexpected EOF");
	return ch;
}

static void ttyPut(char ch) {
	fputc(ch, tty);
	if (ferror(tty)) err(EX_IOERR, "%s", path);
}

static void msrReq(char req) {
	ttyPut(Esc);
	ttyPut(req);
}
static char msrRes(void) {
	char esc = ttyGet();
	if (esc != Esc) errx(EX_PROTOCOL, "response fail %hhX", esc);
	return ttyGet();
}
static char msr(char req) {
	msrReq(req);
	return msrRes();
}

static void msrReset(void) {
	msrReq('a');
}

static void msrTest(void) {
	char test = msr('e');
	if (test != 'y') errx(EX_PROTOCOL, "test fail %hhX", test);

	char ram = msr(0x87);
	if (ram != '0') errx(EX_PROTOCOL, "ram fail %hhX", ram);

	msrReq('t');
	printf("model: %c%c\n", msrRes(), ttyGet());

	printf("firmware: %hhu\n", msr('v'));
}

static void msrSensorTest(void) {
	char test = msr(0x86);
	if (test != '0') errx(EX_PROTOCOL, "sensor fail %hhX", test);
}

static void msrLED(char led) {
	switch (led) {
		break; case 'a': msrReq(0x82);
		break; case 'g': msrReq(0x83);
		break; case 'y': msrReq(0x84);
		break; case 'r': msrReq(0x85);
		break; default:  msrReq(0x81);
	}
}

static void msrSetZero(char track1, char track2) {
	msrReq('z');
	ttyPut(track1);
	ttyPut(track2);
	char zero = msrRes();
	if (zero != '0') errx(EX_PROTOCOL, "set leading zero fail %hhX", zero);
}

static void msrGetZero(void) {
	char track1 = msr('l');
	char track2 = ttyGet();
	printf("leading zero: %hhu,%hhu\n", track1, track2);
}

static void msrSetBPI(char track1, char track2, char track3) {
	msrReq('b');
	switch (track) {
		break; case 1: ttyPut(bpi == 210 ? 0xA1 : 0xA0);
		break; case 2: ttyPut(bpi == 210 ? 0xD2 : 0x4B);
		break; case 3: ttyPut(bpi == 210 ? 0xC1 : 0xC0);
		break; default: ttyPut(0);
	}
	char set = msrRes();
	if (set != '0') errx(EX_PROTOCOL, "set BPI fail %hhX", set);
}

static void msrSetBPC(char track1, char track2, char track3) {
	msrReq('o');
	ttyPut(track1);
	ttyPut(track2);
	ttyPut(track3);
	char bpc = msrRes();
	if (bpc != '0') errx(EX_PROTOCOL, "set BPC fail %hhX", bpc);
	track1 = ttyGet();
	track2 = ttyGet();
	track3 = ttyGet();
	printf("BPC: %hhu,%hhu,%hhu\n", track1, track2, track3);
}

static void msrHiCo(void) {
	char co = msr('x');
	if (co != '0') errx(EX_PROTOCOL, "hi-co fail %hhX", co);
}

static void msrLoCo(void) {
	char co = msr('y');
	if (co != '0') errx(EX_PROTOCOL, "lo-co fail %hhX", co);
}

static void msrGetCo(void) {
	char co = msr('d');
	switch (co) {
		break; case 'h': printf("hi-co\n");
		break; case 'l': printf("lo-co\n");
		break; default:  errx(EX_PROTOCOL, "get co fail %hhX", co);
	}
}

static void msrStatus(void) {
	char status = msrRes();
	switch (status) {
		case '0': return;
		case '1': errx(EX_DATAERR, "read/write error");
		case '2': errx(EX_DATAERR, "command format error");
		case '4': errx(EX_DATAERR, "invalid command");
		case '9': errx(EX_DATAERR, "invalid card swipe");
		default: errx(EX_PROTOCOL, "status fail %hhX", status);
	}
}

static void msrRead(void) {
	char read = msr('r');
	if (read != 's') errx(EX_PROTOCOL, "read fail %hhX", read);
	for (;;) {
		read = ttyGet();
		if (read == '?') {
			read = ttyGet();
			if (read == FS) break;
			printf("?");
		}
		printf("%c", read);
	}
	msrStatus();
}

static void msrWrite(void) {
	msrReq('w');
	msrReq('s');
	int write;
	while (EOF != (write = getchar())) {
		ttyPut(write);
	}
	ttyPut('?');
	ttyPut(FS);
	msrStatus();
}

static void msrErase(char track) {
	msrReq('c');
	ttyPut(track);
	char erase = msrRes();
	if (erase != '0') errx(EX_PROTOCOL, "erase fail %hhX", erase);
}

static void msrReadRaw(void) {
	char read = msr('m');
	if (read != 's') errx(EX_PROTOCOL, "read raw fail %hhX", read);
	for (;;) {
		read = ttyGet();
		if (read == '?') {
			read = ttyGet();
			if (read == FS) break;
			printf("?");
		}
		printf("%c", read);
	}
	msrStatus();
}

static void msrWriteRaw(void) {
	msrReq('n');
	msrReq('s');
	int write;
	while (EOF != (write = getchar())) {
		ttyPut(write);
	}
	ttyPut('?');
	ttyPut(FS);
	msrStatus();
}

static char parse(char **arg) {
	char n = strtoul(*arg, arg, 0);
	if ((*arg)[0]) (*arg)++;
	return n;
}

int main(int argc, char *argv[]) {
	char func = 't';
	char *arg = NULL;

	int opt;
	while (0 < (opt = getopt(argc, argv, "L:RTWZ:b:cd:e:f:hilrtwz"))) {
		switch (opt) {
			break; case 'f': path = optarg;
			break; case '?': return EX_USAGE;
			break; default:  func = opt; arg = optarg;
		}
	}

	int fd = open(path, O_RDWR);
	if (fd < 0) err(EX_NOINPUT, "%s", path);

	struct termios attr;
	int error = tcgetattr(fd, &attr);
	if (error) err(EX_IOERR, "tcgetattr");

	cfmakeraw(&attr);
	error = tcsetattr(fd, TCSANOW, &attr);
	if (error) err(EX_IOERR, "tcsetattr");

	tty = fdopen(fd, "r+");
	if (!tty) err(EX_IOERR, "fdopen");

	switch (func) {
		break; case 'L': msrLED(arg[0]);
		break; case 'R': msrReadRaw();
		break; case 'T': msrSensorTest();
		break; case 'W': msrWriteRaw();
		break; case 'Z': msrSetZero(parse(&arg), parse(&arg));
		break; case 'b': msrSetBPC(parse(&arg), parse(&arg), parse(&arg));
		break; case 'c': msrGetCo();
		break; case 'd': msrSetBPI(parse(&arg), parse(&arg));
		break; case 'e': msrErase(arg[0] - '0');
		break; case 'h': msrHiCo();
		break; case 'i': msrReset();
		break; case 'l': msrLoCo();
		break; case 'r': msrRead();
		break; case 't': msrTest();
		break; case 'w': msrWrite();
		break; case 'z': msrGetZero();
		break; default:  return EX_USAGE;
	}
}
