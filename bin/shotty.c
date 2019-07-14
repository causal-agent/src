/* Copyright (C) 2019  June McEnroe <june@causal.agency>
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
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sysexits.h>
#include <unistd.h>
#include <wchar.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef unsigned uint;

enum {
	NUL, SOH, STX, ETX, EOT, ENQ, ACK, BEL,
	BS, HT, NL, VT, NP, CR, SO, SI,
	DLE, DC1, DC2, DC3, DC4, NAK, SYN, ETB,
	CAN, EM, SUB, ESC, FS, GS, RS, US,
	DEL = 0x7F,
};

struct Style {
	bool bold, italic, underline, reverse;
	uint bg, fg;
};

struct Cell {
	struct Style style;
	wchar_t ch;
};

static struct Style def = { .fg = 7 };
static uint rows = 24, cols = 80;
static bool debug;

static uint y, x;
static bool insert;
static struct Style style;
static struct Cell *cells;

static struct Cell *cell(uint y, uint x) {
	return &cells[y * cols + x];
}

static void clear(struct Cell *a, struct Cell *b) {
	for (; a <= b; ++a) {
		a->style = style;
		a->ch = ' ';
	}
}

static void move(struct Cell *dst, struct Cell *src, uint len) {
	memmove(dst, src, sizeof(*dst) * len);
}

static void span(const struct Style *prev, const struct Cell *cell) {
	if (!prev || memcmp(&cell->style, prev, sizeof(cell->style))) {
		if (prev) printf("</span>");
		uint bg = (cell->style.reverse ? cell->style.fg : cell->style.bg);
		uint fg = (cell->style.reverse ? cell->style.bg : cell->style.fg);
		printf(
			"<span style=\"%s%s%s\" class=\"bg%u fg%u\">",
			cell->style.bold ? "font-weight:bold;" : "",
			cell->style.italic ? "font-style:italic;" : "",
			cell->style.underline ? "text-decoration:underline;" : "",
			bg, fg
		);
	}
	switch (cell->ch) {
		break; case '&': printf("&amp;");
		break; case '<': printf("&lt;");
		break; case '>': printf("&gt;");
		break; default:  printf("%lc", cell->ch);
	}
}

static void html(bool cursor) {
	if (cursor) cell(y, x)->style.reverse ^= true;
	printf(
		"<pre style=\"width: %uch;\" class=\"bg%u fg%u\">",
		cols, def.bg, def.fg
	);
	for (uint y = 0; y < rows; ++y) {
		for (uint x = 0; x < cols; ++x) {
			if (!cell(y, x)->ch) continue;
			span(x ? &cell(y, x - 1)->style : NULL, cell(y, x));
		}
		printf("</span>\n");
	}
	printf("</pre>\n");
	if (cursor) cell(y, x)->style.reverse ^= true;
}

static char updateNUL(wchar_t ch) {
	switch (ch) {
		case BS: if (x) x--; return NUL;
		case NL: y = MIN(y + 1, rows - 1); return NUL;
		case CR: x = 0; return NUL;
		case ESC: return ESC;
	}
	if (ch < ' ') {
		warnx("unhandled \\x%02X", ch);
		return NUL;
	}

	int width = wcwidth(ch);
	if (width < 0) {
		warnx("unhandled \\u%X", ch);
		return NUL;
	}
	if (x + width > cols) {
		warnx("cannot fit '%lc'", ch);
		return NUL;
	}

	if (insert) {
		move(cell(y, x + width), cell(y, x), cols - x - width);
	}
	cell(y, x)->style = style;
	cell(y, x)->ch = ch;

	for (int i = 1; i < width; ++i) {
		cell(y, x + i)->style = style;
		cell(y, x + i)->ch = '\0';
	}
	x = MIN(x + width, cols - 1);

	return NUL;
}

enum {
	CSI = '[',
	ST  = '\\',
	OSC = ']',
	CUU = 'A',
	CUD = 'B',
	CUF = 'C',
	CUB = 'D',
	CNL = 'E',
	CPL = 'F',
	CHA = 'G',
	CUP = 'H',
	ED  = 'J',
	EL  = 'K',
	DL  = 'M',
	DCH = 'P',
	VPA = 'd',
	SM  = 'h',
	RM  = 'l',
	SGR = 'm',
};

static char updateESC(wchar_t ch) {
	static bool discard;
	if (discard) {
		discard = false;
		return NUL;
	}
	switch (ch) {
		case '(': discard = true; return ESC;
		case '=': return NUL;
		case CSI: return CSI;
		case OSC: return OSC;
		default: warnx("unhandled ESC %lc", ch); return NUL;
	}
}

static char updateCSI(wchar_t ch) {
	static bool dec;
	if (ch == '?') {
		dec = true;
		return CSI;
	}

	static uint n, p, ps[8];
	if (ch == ';') {
		n++;
		p++;
		ps[p %= 8] = 0;
		return CSI;
	}
	if (ch >= '0' && ch <= '9') {
		ps[p] *= 10;
		ps[p] += ch - '0';
		if (!n) n++;
		return CSI;
	}

	switch (ch) {
		break; case CUU: y -= MIN((n ? ps[0] : 1), y);
		break; case CUD: y  = MIN(y + (n ? ps[0] : 1), rows - 1);
		break; case CUF: x  = MIN(x + (n ? ps[0] : 1), cols - 1);
		break; case CUB: x -= MIN((n ? ps[0] : 1), x);
		break; case CNL: y  = MIN(y + (n ? ps[0] : 1), rows - 1); x = 0;
		break; case CPL: y -= MIN((n ? ps[0] : 1), y); x = 0;
		break; case CHA: x  = MIN((n ? ps[0] - 1 : 0), cols - 1);
		break; case VPA: y  = MIN((n ? ps[0] - 1 : 0), rows - 1);
		break; case CUP: {
			y = MIN((n > 0 ? ps[0] - 1 : 0), rows - 1);
			x = MIN((n > 1 ? ps[1] - 1 : 0), cols - 1);
		}

		break; case ED: {
			struct Cell *a = cell(0, 0);
			struct Cell *b = cell(rows - 1, cols - 1);
			if (ps[0] == 0) a = cell(y, x);
			if (ps[0] == 1) b = cell(y, x);
			clear(a, b);
		}
		break; case EL: {
			struct Cell *a = cell(y, 0);
			struct Cell *b = cell(y, cols - 1);
			if (ps[0] == 0) a = cell(y, x);
			if (ps[0] == 1) b = cell(y, x);
			clear(a, b);
		}

		break; case DL: {
			uint i = MIN((n ? ps[0] : 1), rows - y);
			move(cell(y, 0), cell(y + i, 0), cols * (rows - y - i));
			clear(cell(rows - i, 0), cell(rows - 1, cols - 1));
		}
		break; case DCH: {
			uint i = MIN((n ? ps[0] : 1), cols - x);
			move(cell(y, x), cell(y, x + i), cols - x - i);
			clear(cell(y, cols - i), cell(y, cols - 1));
		}

		break; case SM: {
			if (dec) break;
			switch (ps[0]) {
				break; case 4: insert = true;
				break; default: warnx("unhandled SM %u", ps[0]);
			}
		}
		break; case RM: {
			if (dec) break;
			switch (ps[0]) {
				break; case 4: insert = false;
				break; default: warnx("unhandled RM %u", ps[0]);
			}
		}

		break; case SGR: {
			if (ps[0] == 38 && ps[1] == 5) {
				style.fg = ps[2];
				break;
			}
			if (ps[0] == 48 && ps[1] == 5) {
				style.bg = ps[2];
				break;
			}
			for (uint i = 0; i < p + 1; ++i) {
				switch (ps[i]) {
					break; case 0: style = def;
					break; case 1: style.bold = true;
					break; case 3: style.italic = true;
					break; case 4: style.underline = true;
					break; case 7: style.reverse = true;
					break; case 21: style.bold = false;
					break; case 22: style.bold = false;
					break; case 23: style.italic = false;
					break; case 24: style.underline = false;
					break; case 27: style.reverse = false;
					break; case 39: style.fg = def.fg;
					break; case 49: style.bg = def.bg;
					break; default: {
						if (ps[i] >= 30 && ps[i] <= 37) {
							style.fg = ps[i] - 30;
						} else if (ps[i] >= 40 && ps[i] <= 47) {
							style.bg = ps[i] - 40;
						} else if (ps[i] >= 90 && ps[i] <= 97) {
							style.fg = 7 + ps[i] - 90;
						} else if (ps[i] >= 100 && ps[i] <= 107) {
							style.bg = 7 + ps[i] - 100;
						} else {
							warnx("unhandled SGR %u", ps[i]);
						}
					}
				}
			}
		}

		break; default: warnx("unhandled CSI %lc", ch);
	}

	if (debug) {
		printf("CSI %s", (dec ? "? " : ""));
		for (uint i = 0; i < n; ++i) {
			printf("%s%u ", (i ? "; " : ""), ps[i]);
		}
		printf("%lc\n", ch);
		html(true);
	}

	dec = false;
	ps[n = p = 0] = 0;
	return NUL;
}

static char updateOSC(wchar_t ch) {
	static bool esc;
	switch (ch) {
		break; case BEL: return NUL;
		break; case ESC: esc = true;
		break; case ST: {
			if (!esc) break;
			esc = false;
			return NUL;
		}
	}
	esc = false;
	return NUL;
}

static void update(wchar_t ch) {
	static char seq;
	switch (seq) {
		break; case NUL: seq = updateNUL(ch);
		break; case ESC: seq = updateESC(ch);
		break; case CSI: seq = updateCSI(ch);
		break; case OSC: seq = updateOSC(ch);
	}
}

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");

	bool bright = false;
	bool cursor = false;
	bool size = false;
	FILE *file = stdin;

	int opt;
	while (0 < (opt = getopt(argc, argv, "Bb:cdf:h:sw:"))) {
		switch (opt) {
			break; case 'B': bright = true;
			break; case 'b': def.bg = strtoul(optarg, NULL, 0);
			break; case 'c': cursor = true;
			break; case 'd': debug = true;
			break; case 'f': def.fg = strtoul(optarg, NULL, 0);
			break; case 'h': rows = strtoul(optarg, NULL, 0);
			break; case 's': size = true;
			break; case 'w': cols = strtoul(optarg, NULL, 0);
			break; default:  return EX_USAGE;
		}
	}

	if (optind < argc) {
		file = fopen(argv[optind], "r");
		if (!file) err(EX_NOINPUT, "%s", argv[optind]);
	}

	if (size) {
		struct winsize window;
		int error = ioctl(STDERR_FILENO, TIOCGWINSZ, &window);
		if (error) err(EX_IOERR, "ioctl");
		rows = window.ws_row;
		cols = window.ws_col;
	}

	cells = calloc(rows * cols, sizeof(*cells));
	if (!cells) err(EX_OSERR, "calloc");

	style = def;
	clear(cell(0, 0), cell(rows - 1, cols - 1));

	wint_t ch;
	while (WEOF != (ch = getwc(file))) {
		update(ch);
	}
	if (ferror(file)) err(EX_IOERR, "getwc");

	if (bright) {
		for (uint i = 0; i < rows * cols; ++i) {
			if (!cells[i].style.bold || cells[i].style.fg > 7) continue;
			cells[i].style.bold = false;
			cells[i].style.fg += 8;
		}
	}

	html(cursor);
}
