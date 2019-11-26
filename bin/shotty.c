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

#include <assert.h>
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

#define BIT(x) x##Bit, x = 1 << x##Bit, x##Bit_ = x##Bit

typedef unsigned uint;

enum {
	NUL, SOH, STX, ETX, EOT, ENQ, ACK, BEL,
	BS, HT, NL, VT, NP, CR, SO, SI,
	DLE, DC1, DC2, DC3, DC4, NAK, SYN, ETB,
	CAN, EM, SUB, ESC, FS, GS, RS, US,
	DEL = 0x7F,
};

enum Attr {
	BIT(Bold),
	BIT(Dim),
	BIT(Italic),
	BIT(Underline),
	BIT(Blink),
	BIT(Reverse),
};

struct Style {
	enum Attr attr;
	int bg, fg;
};

struct Cell {
	struct Style style;
	wchar_t ch;
};

static uint rows = 24, cols = 80;
static struct Cell *cells;

static struct Cell *cell(uint y, uint x) {
	assert(y <= rows);
	assert(x <= cols);
	assert(y * cols + x <= rows * cols);
	return &cells[y * cols + x];
}

static uint y, x;
static struct Style style = { .bg = -1, .fg = -1 };

static struct {
	uint y, x;
} save;

enum { ParamCap = 16 };
static struct {
	uint s[ParamCap];
	uint n, i;
} param;

static uint p(uint i, uint z) {
	return (i < param.n ? param.s[i] : z);
}

static uint min(uint a, uint b) {
	return (a < b ? a : b);
}

#define _ch ch __attribute__((unused))
typedef void Action(wchar_t ch);

static void nop(wchar_t _ch) {
}

static void csi(wchar_t _ch) {
	memset(&param, 0, sizeof(param));
}

static void csiSep(wchar_t _ch) {
	if (param.n == ParamCap) return;
	if (!param.n) param.n++;
	param.n++;
	param.i++;
}

static void csiDigit(wchar_t ch) {
	param.s[param.i] *= 10;
	param.s[param.i] += ch - L'0';
	if (!param.n) param.n++;
}

static void bs(wchar_t _ch)  { if (x) x--; }
static void ht(wchar_t _ch)  { x = min(x - x % 8 + 8, cols - 1); }
static void cr(wchar_t _ch)  { x = 0; }
static void cuu(wchar_t _ch) { y -= min(p(0, 1), y); }
static void cud(wchar_t _ch) { y  = min(y + p(0, 1), rows - 1); }
static void cuf(wchar_t _ch) { x  = min(x + p(0, 1), cols - 1); }
static void cub(wchar_t _ch) { x -= min(p(0, 1), x); }
static void cnl(wchar_t _ch) { x = 0; cud(0); }
static void cpl(wchar_t _ch) { x = 0; cuu(0); }
static void cha(wchar_t _ch) { x = min(p(0, 1) - 1, cols - 1); }
static void vpa(wchar_t _ch) { y = min(p(0, 1) - 1, rows - 1); }
static void cup(wchar_t _ch) {
	y = min(p(0, 1) - 1, rows - 1);
	x = min(p(1, 1) - 1, cols - 1);
}
static void decsc(wchar_t _ch) {
	save.y = y;
	save.x = x;
}
static void decrc(wchar_t _ch) {
	y = save.y;
	x = save.x;
}

static void move(struct Cell *dst, struct Cell *src, size_t len) {
	memmove(dst, src, sizeof(*dst) * len);
}

static void erase(struct Cell *at, struct Cell *to) {
	for (; at < to; ++at) {
		at->style = style;
		at->ch = L' ';
	}
}

static void ed(wchar_t _ch) {
	erase(
		(p(0, 0) == 0 ? cell(y, x) : cell(0, 0)),
		(p(0, 0) == 1 ? cell(y, x) : cell(rows - 1, cols))
	);
}
static void el(wchar_t _ch) {
	erase(
		(p(0, 0) == 0 ? cell(y, x) : cell(y, 0)),
		(p(0, 0) == 1 ? cell(y, x) : cell(y, cols))
	);
}
static void ech(wchar_t _ch) {
	erase(cell(y, x), cell(y, min(x + p(0, 1), cols)));
}

static void dch(wchar_t _ch) {
	uint n = min(p(0, 1), cols - x);
	move(cell(y, x), cell(y, x + n), cols - x - n);
	erase(cell(y, cols - n), cell(y, cols));
}
static void ich(wchar_t _ch) {
	uint n = min(p(0, 1), cols - x);
	move(cell(y, x + n), cell(y, x), cols - x - n);
	erase(cell(y, x), cell(y, x + n));
}

static struct {
	uint top, bot;
} scroll;

static void scrollUp(uint top, uint n) {
	n = min(n, scroll.bot - top);
	move(cell(top, 0), cell(top + n, 0), cols * (scroll.bot - top - n));
	erase(cell(scroll.bot - n, 0), cell(scroll.bot, 0));
}

static void scrollDown(uint top, uint n) {
	n = min(n, scroll.bot - top);
	move(cell(top + n, 0), cell(top, 0), cols * (scroll.bot - top - n));
	erase(cell(top, 0), cell(top + n, 0));
}

static void decstbm(wchar_t _ch) {
	scroll.bot = min(p(1, rows), rows);
	scroll.top = min(p(0, 1) - 1, scroll.bot);
}

static void su(wchar_t _ch) { scrollUp(scroll.top, p(0, 1)); }
static void sd(wchar_t _ch) { scrollDown(scroll.top, p(0, 1)); }
static void dl(wchar_t _ch) { scrollUp(min(y, scroll.bot), p(0, 1)); }
static void il(wchar_t _ch) { scrollDown(min(y, scroll.bot), p(0, 1)); }

static void nl(wchar_t _ch) {
	if (y + 1 == scroll.bot) {
		scrollUp(scroll.top, 1);
	} else {
		y = min(y + 1, rows - 1);
	}
}
static void ri(wchar_t _ch) {
	if (y == scroll.top) {
		scrollDown(scroll.top, 1);
	} else {
		if (y) y--;
	}
}

static enum Mode {
	BIT(Insert),
	BIT(Wrap),
	BIT(Cursor),
} mode = Wrap | Cursor;

static enum Mode paramMode(void) {
	enum Mode mode = 0;
	for (uint i = 0; i < param.n; ++i) {
		switch (param.s[i]) {
			break; case 4: mode |= Insert;
			break; default: warnx("unhandled SM/RM %u", param.s[i]);
		}
	}
	return mode;
}

static enum Mode paramDECMode(void) {
	enum Mode mode = 0;
	for (uint i = 0; i < param.n; ++i) {
		switch (param.s[i]) {
			break; case 1: // DECCKM
			break; case 7: mode |= Wrap;
			break; case 12: // "Start Blinking Cursor"
			break; case 25: mode |= Cursor;
			break; default: {
				if (param.s[i] < 1000) {
					warnx("unhandled DECSET/DECRST %u", param.s[i]);
				}
			}
		}
	}
	return mode;
}

static void sm(wchar_t _ch) { mode |= paramMode(); }
static void rm(wchar_t _ch) { mode &= ~paramMode(); }
static void decset(wchar_t _ch) { mode |= paramDECMode(); }
static void decrst(wchar_t _ch) { mode &= ~paramDECMode(); }

enum {
	Reset,
	SetBold,
	SetDim,
	SetItalic,
	SetUnderline,
	SetBlink,
	SetReverse = 7,

	UnsetBoldDim = 22,
	UnsetItalic,
	UnsetUnderline,
	UnsetBlink,
	UnsetReverse = 27,

	SetFg0 = 30,
	SetFg7 = 37,
	SetFg,
	ResetFg,
	SetBg0 = 40,
	SetBg7 = 47,
	SetBg,
	ResetBg,

	SetFg8 = 90,
	SetFgF = 97,
	SetBg8 = 100,
	SetBgF = 107,

	Color256 = 5,
};

static void sgr(wchar_t _ch) {
	uint n = param.i + 1;
	for (uint i = 0; i < n; ++i) {
		switch (param.s[i]) {
			break; case Reset: style = (struct Style) { .bg = -1, .fg = -1 };

			break; case SetBold:      style.attr |= Bold; style.attr &= ~Dim;
			break; case SetDim:       style.attr |= Dim; style.attr &= ~Bold;
			break; case SetItalic:    style.attr |= Italic;
			break; case SetUnderline: style.attr |= Underline;
			break; case SetBlink:     style.attr |= Blink;
			break; case SetReverse:   style.attr |= Reverse;

			break; case UnsetBoldDim:   style.attr &= ~(Bold | Dim);
			break; case UnsetItalic:    style.attr &= ~Italic;
			break; case UnsetUnderline: style.attr &= ~Underline;
			break; case UnsetBlink:     style.attr &= ~Blink;
			break; case UnsetReverse:   style.attr &= ~Reverse;

			break; case SetFg: {
				if (++i < n && param.s[i] == Color256) {
					if (++i < n) style.fg = param.s[i];
				}
			}
			break; case SetBg: {
				if (++i < n && param.s[i] == Color256) {
					if (++i < n) style.bg = param.s[i];
				}
			}

			break; case ResetFg: style.fg = -1;
			break; case ResetBg: style.bg = -1;

			break; default: {
				uint p = param.s[i];
				if (p >= SetFg0 && p <= SetFg7) {
					style.fg = p - SetFg0;
				} else if (p >= SetBg0 && p <= SetBg7) {
					style.bg = p - SetBg0;
				} else if (p >= SetFg8 && p <= SetFgF) {
					style.fg = 8 + p - SetFg8;
				} else if (p >= SetBg8 && p <= SetBgF) {
					style.bg = 8 + p - SetBg8;
				} else {
					warnx("unhandled SGR %u", p);
				}
			}
		}
	}
}

static enum {
	USASCII,
	DECSpecial,
} charset;

static void usascii(wchar_t _ch) { charset = USASCII; }
static void decSpecial(wchar_t _ch) { charset = DECSpecial; }

static const wchar_t AltCharset[128] = {
	['`'] = L'◆', ['a'] = L'▒', ['f'] = L'°', ['g'] = L'±', ['i'] = L'␋',
	['j'] = L'┘', ['k'] = L'┐', ['l'] = L'┌', ['m'] = L'└', ['n'] = L'┼',
	['o'] = L'⎺', ['p'] = L'⎻', ['q'] = L'─', ['r'] = L'⎼', ['s'] = L'⎽',
	['t'] = L'├', ['u'] = L'┤', ['v'] = L'┴', ['w'] = L'┬', ['x'] = L'│',
	['y'] = L'≤', ['z'] = L'≥', ['{'] = L'π', ['|'] = L'≠', ['}'] = L'£',
	['~'] = L'·',
};

static void add(wchar_t ch) {
	if (charset == DECSpecial && ch < 128 && AltCharset[ch]) {
		ch = AltCharset[ch];
	}

	int width = wcwidth(ch);
	if (width < 0) {
		warnx("unhandled \\u%02X", ch);
		return;
	}

	if (mode & Insert) {
		uint n = min(width, cols - x);
		move(cell(y, x + n), cell(y, x), cols - x - n);
	}
	if (mode & Wrap && x + width > cols) {
		cr(0);
		nl(0);
	}

	cell(y, x)->style = style;
	cell(y, x)->ch = ch;
	for (int i = 1; i < width && x + i < cols; ++i) {
		cell(y, x + i)->style = style;
		cell(y, x + i)->ch = L'\0';
	}
	x = min(x + width, (mode & Wrap ? cols : cols - 1));
}

static void html(void);
static void mc(wchar_t _ch) {
	if (p(0, 0) == 10) {
		html();
	} else {
		warnx("unhandled CSI %u MC", p(0, 0));
	}
}

static enum {
	Data,
	Esc,
	G0,
	CSI,
	CSILt,
	CSIEq,
	CSIGt,
	CSIQm,
	CSIInter,
	OSC,
	OSCEsc,
} state;

static void escDefault(wchar_t ch) {
	warnx("unhandled ESC %lc", ch);
}

static void g0Default(wchar_t ch) {
	warnx("unhandled G0 %lc", ch);
	charset = USASCII;
}

static void csiInter(wchar_t ch) {
	switch (state) {
		break; case CSI: warnx("unhandled CSI %lc ...", ch);
		break; case CSILt: warnx("unhandled CSI < %lc ...", ch);
		break; case CSIEq: warnx("unhandled CSI = %lc ...", ch);
		break; case CSIGt: warnx("unhandled CSI > %lc ...", ch);
		break; case CSIQm: warnx("unhandled CSI ? %lc ...", ch);
		break; default: abort();
	}
}

static void csiFinal(wchar_t ch) {
	switch (state) {
		break; case CSI: warnx("unhandled CSI %lc", ch);
		break; case CSILt: warnx("unhandled CSI < %lc", ch);
		break; case CSIEq: warnx("unhandled CSI = %lc", ch);
		break; case CSIGt: warnx("unhandled CSI > %lc", ch);
		break; case CSIQm: warnx("unhandled CSI ? %lc", ch);
		break; case CSIInter: warnx("unhandled CSI ... %lc", ch);
		break; default: abort();
	}
}

#define S(s) break; case s: switch (ch)
#define A(c, a, s) break; case c: a(ch); state = s
#define D(a, s) break; default: a(ch); state = s
static void update(wchar_t ch) {
	switch (state) {
		default: abort();

		S(Data) {
			A(BEL, nop, Data);
			A(BS,  bs,  Data);
			A(HT,  ht,  Data);
			A(NL,  nl,  Data);
			A(CR,  cr,  Data);
			A(ESC, nop, Esc);
			D(add, Data);
		}

		S(Esc) {
			A('(', nop, G0);
			A('7', decsc, Data);
			A('8', decrc, Data);
			A('=', nop, Data);
			A('>', nop, Data);
			A('M', ri,  Data);
			A('[', csi, CSI);
			A(']', nop, OSC);
			D(escDefault, Data);
		}
		S(G0) {
			A('0', decSpecial, Data);
			A('B', usascii, Data);
			D(g0Default, Data);
		}

		S(CSI) {
			A(' ' ... '/', csiInter, CSIInter);
			A('0' ... '9', csiDigit, CSI);
			A(':', nop, CSI);
			A(';', csiSep, CSI);
			A('<', nop, CSILt);
			A('=', nop, CSIEq);
			A('>', nop, CSIGt);
			A('?', nop, CSIQm);
			A('@', ich, Data);
			A('A', cuu, Data);
			A('B', cud, Data);
			A('C', cuf, Data);
			A('D', cub, Data);
			A('E', cnl, Data);
			A('F', cpl, Data);
			A('G', cha, Data);
			A('H', cup, Data);
			A('J', ed,  Data);
			A('K', el,  Data);
			A('L', il,  Data);
			A('M', dl,  Data);
			A('P', dch, Data);
			A('S', su,  Data);
			A('T', sd,  Data);
			A('X', ech, Data);
			A('d', vpa, Data);
			A('h', sm,  Data);
			A('i', mc,  Data);
			A('l', rm,  Data);
			A('m', sgr, Data);
			A('r', decstbm, Data);
			A('t', nop, Data);
			D(csiFinal, Data);
		}

		S(CSILt ... CSIGt) {
			A(' ' ... '/', csiInter, CSIInter);
			A('0' ... '9', csiDigit, state);
			A(':', nop, state);
			A(';', csiSep, state);
			D(csiFinal, Data);
		}

		S(CSIQm) {
			A(' ' ... '/', csiInter, CSIInter);
			A('0' ... '9', csiDigit, CSIQm);
			A(':', nop, CSIQm);
			A(';', csiSep, CSIQm);
			A('h', decset, Data);
			A('l', decrst, Data);
			D(csiFinal, Data);
		}

		S(CSIInter) {
			D(csiFinal, Data);
		}

		S(OSC) {
			A(BEL, nop, Data);
			A(ESC, nop, OSCEsc);
			D(nop, OSC);
		}
		S(OSCEsc) {
			A('\\', nop, Data);
			D(nop, OSC);
		}
	}
}

static bool bright;
static int defaultBg = 0;
static int defaultFg = 7;

static void span(const struct Style *prev, const struct Cell *cell) {
	struct Style style = cell->style;
	if (!prev || memcmp(prev, &style, sizeof(*prev))) {
		if (prev) printf("</span>");
		if (style.bg < 0) style.bg = defaultBg;
		if (style.fg < 0) style.fg = defaultFg;
		if (bright && style.attr & Bold) {
			if (style.fg < 8) style.fg += 8;
			style.attr ^= Bold;
		}
		printf(
			"<span style=\"%s%s%s\" class=\"bg%u fg%u\">",
			(style.attr & Bold ? "font-weight:bold;" : ""),
			(style.attr & Italic ? "font-style:italic;" : ""),
			(style.attr & Underline ? "text-decoration:underline;" : ""),
			(style.attr & Reverse ? style.fg : style.bg),
			(style.attr & Reverse ? style.bg : style.fg)
		);
	}
	switch (cell->ch) {
		break; case '&': printf("&amp;");
		break; case '<': printf("&lt;");
		break; case '>': printf("&gt;");
		break; default:  printf("%lc", (wint_t)cell->ch);
	}
}

static bool mediaCopy;
static void html(void) {
	mediaCopy = true;
	if (mode & Cursor) cell(y, x)->style.attr ^= Reverse;
	printf(
		"<pre style=\"width: %uch;\" class=\"bg%u fg%u\">",
		cols, defaultBg, defaultFg
	);
	for (uint y = 0; y < rows; ++y) {
		for (uint x = 0; x < cols; ++x) {
			if (!cell(y, x)->ch) continue;
			span(x ? &cell(y, x - 1)->style : NULL, cell(y, x));
		}
		printf("</span>\n");
	}
	printf("</pre>\n");
	if (mode & Cursor) cell(y, x)->style.attr ^= Reverse;
}

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");

	bool debug = false;
	bool size = false;
	bool hide = false;

	int opt;
	while (0 < (opt = getopt(argc, argv, "Bb:df:h:nsw:"))) {
		switch (opt) {
			break; case 'B': bright = true;
			break; case 'b': defaultBg = strtol(optarg, NULL, 0);
			break; case 'd': debug = true;
			break; case 'f': defaultFg = strtol(optarg, NULL, 0);
			break; case 'h': rows = strtoul(optarg, NULL, 0);
			break; case 'n': hide = true;
			break; case 's': size = true;
			break; case 'w': cols = strtoul(optarg, NULL, 0);
			break; default:  return EX_USAGE;
		}
	}

	FILE *file = stdin;
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
	scroll.bot = rows;

	cells = calloc(rows * cols, sizeof(*cells));
	if (!cells) err(EX_OSERR, "calloc");
	erase(cell(0, 0), cell(rows - 1, cols));

	wint_t ch;
	while (WEOF != (ch = getwc(file))) {
		uint prev = state;
		update(ch);
		if (debug && state != prev && state == Data) html();
	}
	if (ferror(file)) err(EX_IOERR, "getwc");

	if (!mediaCopy) {
		if (hide) mode &= ~Cursor;
		html();
	}
}
