/* Copyright (C) 2019, 2021  June McEnroe <june@causal.agency>
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

%option noinput nounput noyywrap

%{

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

#define Q(...) #__VA_ARGS__
#define BIT(x) x##Bit, x = 1 << x##Bit, x##Bit_ = x##Bit

#define ENUM_CODE \
	X(BS) \
	X(CHA) \
	X(CNL) \
	X(CPL) \
	X(CR) \
	X(CUB) \
	X(CUD) \
	X(CUF) \
	X(CUP) \
	X(CUU) \
	X(DCH) \
	X(DECRC) \
	X(DECRST) \
	X(DECSC) \
	X(DECSET) \
	X(DECSTBM) \
	X(DL) \
	X(ECH) \
	X(ED) \
	X(EL) \
	X(HT) \
	X(ICH) \
	X(IL) \
	X(MC) \
	X(NL) \
	X(RI) \
	X(RM) \
	X(SD) \
	X(SGR) \
	X(SM) \
	X(SU) \
	X(VPA)

enum Code {
	Data = 1,
#define X(code) code,
	ENUM_CODE
#undef X
};

static enum {
	USASCII,
	DECSpecial,
} g0;

static const wchar_t AltCharset[128] = {
	['`'] = L'\u25C6', ['a'] = L'\u2592', ['f'] = L'\u00B0', ['g'] = L'\u00B1',
	['i'] = L'\u240B', ['j'] = L'\u2518', ['k'] = L'\u2510', ['l'] = L'\u250C',
	['m'] = L'\u2514', ['n'] = L'\u253C', ['o'] = L'\u23BA', ['p'] = L'\u23BB',
	['q'] = L'\u2500', ['r'] = L'\u23BC', ['s'] = L'\u23BD', ['t'] = L'\u251C',
	['u'] = L'\u2524', ['v'] = L'\u2534', ['w'] = L'\u252C', ['x'] = L'\u2502',
	['y'] = L'\u2264', ['z'] = L'\u2265', ['{'] = L'\u03C0', ['|'] = L'\u2260',
	['}'] = L'\u00A3', ['~'] = L'\u00B7',
};

static int pn;
static int ps[16];
static wchar_t ch;

%}

ESC \x1B

%x CSI CSI_LT CSI_EQ CSI_GT CSI_QM
%x OSC

%%
	pn = 0;

{ESC}"["	BEGIN(CSI);
{ESC}"[<"	BEGIN(CSI_LT);
{ESC}"[="	BEGIN(CSI_EQ);
{ESC}"[>"	BEGIN(CSI_GT);
{ESC}"[?"	BEGIN(CSI_QM);
{ESC}"]"	BEGIN(OSC);

<CSI,CSI_LT,CSI_EQ,CSI_GT,CSI_QM>{
	[0-9]+;?	if (pn < 16) ps[pn++] = atoi(yytext);
	;			if (pn < 16) ps[pn++] = 0;
}

<OSC>{
	\x07	BEGIN(0);
	{ESC}\\	BEGIN(0);
	.|\n	;
}

\b	return BS;
\t	return HT;
\n	return NL;
\r	return CR;

{ESC}7	return DECSC;
{ESC}8	return DECRC;
{ESC}=	// DECKPAM
{ESC}>	// DECKPNM
{ESC}M	return RI;

{ESC}"(0"	g0 = DECSpecial;
{ESC}"(B"	g0 = USASCII;

<CSI>@	BEGIN(0); return ICH;
<CSI>A	BEGIN(0); return CUU;
<CSI>B	BEGIN(0); return CUD;
<CSI>C	BEGIN(0); return CUF;
<CSI>D	BEGIN(0); return CUB;
<CSI>E	BEGIN(0); return CNL;
<CSI>F	BEGIN(0); return CPL;
<CSI>G	BEGIN(0); return CHA;
<CSI>H	BEGIN(0); return CUP;
<CSI>J	BEGIN(0); return ED;
<CSI>K	BEGIN(0); return EL;
<CSI>L	BEGIN(0); return IL;
<CSI>M	BEGIN(0); return DL;
<CSI>P	BEGIN(0); return DCH;
<CSI>S	BEGIN(0); return SU;
<CSI>T	BEGIN(0); return SD;
<CSI>X	BEGIN(0); return ECH;
<CSI>d	BEGIN(0); return VPA;
<CSI>h	BEGIN(0); return SM;
<CSI>i	BEGIN(0); return MC;
<CSI>l	BEGIN(0); return RM;
<CSI>m	BEGIN(0); return SGR;
<CSI>r	BEGIN(0); return DECSTBM;
<CSI>t	BEGIN(0); // XTWINOPS

<CSI_QM>h	BEGIN(0); return DECSET;
<CSI_QM>l	BEGIN(0); return DECRST;

<CSI>[ -/]*.	BEGIN(0); warnx("unhandled CSI %s", yytext);
<CSI_LT>[ -/]*.	BEGIN(0); warnx("unhandled CSI < %s", yytext);
<CSI_EQ>[ -/]*.	BEGIN(0); warnx("unhandled CSI = %s", yytext);
<CSI_GT>[ -/]*.	BEGIN(0); warnx("unhandled CSI > %s", yytext);
<CSI_QM>[ -/]*.	BEGIN(0); warnx("unhandled CSI ? %s", yytext);

[\x00-\x7F] {
	ch = yytext[0];
	if (g0 == DECSpecial && AltCharset[ch]) {
		ch = AltCharset[ch];
	}
	return Data;
}
[\xC0-\xDF][\x80-\xBF] {
	ch = (wchar_t)(yytext[0] & 0x1F) << 6
		| (wchar_t)(yytext[1] & 0x3F);
	return Data;
}
[\xE0-\xEF][\x80-\xBF]{2} {
	ch = (wchar_t)(yytext[0] & 0x0F) << 12
		| (wchar_t)(yytext[1] & 0x3F) << 6
		| (wchar_t)(yytext[2] & 0x3F);
	return Data;
}
[\xF0-\xF7][\x80-\xBF]{3} {
	ch = (wchar_t)(yytext[0] & 0x07) << 18
		| (wchar_t)(yytext[1] & 0x3F) << 12
		| (wchar_t)(yytext[2] & 0x3F) << 6
		| (wchar_t)(yytext[3] & 0x3F);
	return Data;
}

.	ch = yytext[0]; return Data;

%%

static int rows = 24;
static int cols = 80;

static struct Cell {
	enum {
		BIT(Bold),
		BIT(Italic),
		BIT(Underline),
		BIT(Reverse),
	} attr;
	int bg, fg;
	wchar_t ch;
} *cells;

static int y, x;
static struct {
	int y, x;
} sc;
static struct {
	int top, bot;
} scr;

static enum Mode {
	BIT(Insert),
	BIT(Wrap),
	BIT(Cursor),
} mode = Wrap | Cursor;

static struct Cell sgr = {
	.bg = -1,
	.fg = -1,
	.ch = L' ',
};

static struct Cell *cell(int y, int x) {
	assert(y <= rows);
	assert(x <= cols);
	assert(y * cols + x <= rows * cols);
	return &cells[y * cols + x];
}

static int p(int i, int d) {
	return (i < pn ? ps[i] : d);
}

static int bound(int a, int x, int b) {
	if (x < a) return a;
	if (x > b) return b;
	return x;
}

static void move(struct Cell *dst, struct Cell *src, size_t len) {
	memmove(dst, src, sizeof(*dst) * len);
}
static void erase(struct Cell *at, struct Cell *to) {
	for (; at < to; ++at) {
		*at = sgr;
	}
}

static void scrup(int top, int n) {
	n = bound(0, n, scr.bot - top);
	move(cell(top, 0), cell(top+n, 0), cols * (scr.bot-top-n));
	erase(cell(scr.bot-n, 0), cell(scr.bot, 0));
}
static void scrdn(int top, int n) {
	n = bound(0, n, scr.bot - top);
	move(cell(top+n, 0), cell(top, 0), cols * (scr.bot-top-n));
	erase(cell(top, 0), cell(top+n, 0));
}

static enum Mode pmode(void) {
	enum Mode mode = 0;
	for (int i = 0; i < pn; ++i) {
		switch (ps[i]) {
			break; case 4: mode |= Insert;
			break; default: warnx("unhandled SM/RM %d", ps[i]);
		}
	}
	return mode;
}
static enum Mode pdmode(void) {
	enum Mode mode = 0;
	for (int i = 0; i < pn; ++i) {
		switch (ps[i]) {
			break; case 1: // DECCKM
			break; case 7: mode |= Wrap;
			break; case 12: // "Start Blinking Cursor"
			break; case 25: mode |= Cursor;
			break; default: {
				if (ps[i] < 1000) warnx("unhandled DECSET/DECRST %d", ps[i]);
			}
		}
	}
	return mode;
}

static void update(enum Code cc) {
	switch (cc) {
		break; case BS: x--;
		break; case HT: x = x - x % 8 + 8;
		break; case CR: x = 0;
		break; case CUU: y -= p(0, 1);
		break; case CUD: y += p(0, 1);
		break; case CUF: x += p(0, 1);
		break; case CUB: x -= p(0, 1);
		break; case CNL: x = 0; y += p(0, 1);
		break; case CPL: x = 0; y -= p(0, 1);
		break; case CHA: x = p(0, 1) - 1;
		break; case VPA: y = p(0, 1) - 1;
		break; case CUP: y = p(0, 1) - 1; x = p(1, 1) - 1;
		break; case DECSC: sc.y = y; sc.x = x;
		break; case DECRC: y = sc.y; x = sc.x;

		break; case ED: erase(
			(p(0, 0) == 0 ? cell(y, x) : cell(0, 0)),
			(p(0, 0) == 1 ? cell(y, x) : cell(rows-1, cols))
		);
		break; case EL: erase(
			(p(0, 0) == 0 ? cell(y, x) : cell(y, 0)),
			(p(0, 0) == 1 ? cell(y, x) : cell(y, cols))
		);
		break; case ECH: erase(
			cell(y, x), cell(y, bound(0, x + p(0, 1), cols))
		);

		break; case DCH: {
			int n = bound(0, p(0, 1), cols-x);
			move(cell(y, x), cell(y, x+n), cols-x-n);
			erase(cell(y, cols-n), cell(y, cols));
		}
		break; case ICH: {
			int n = bound(0, p(0, 1), cols-x);
			move(cell(y, x+n), cell(y, x), cols-x-n);
			erase(cell(y, x), cell(y, x+n));
		}

		break; case DECSTBM: {
			scr.bot = bound(0, p(1, rows), rows);
			scr.top = bound(0, p(0, 1) - 1, scr.bot);
		}
		break; case SU: scrup(scr.top, p(0, 1));
		break; case SD: scrdn(scr.top, p(0, 1));
		break; case DL: scrup(bound(0, y, scr.bot), p(0, 1));
		break; case IL: scrdn(bound(0, y, scr.bot), p(0, 1));

		break; case NL: {
			if (y+1 == scr.bot) {
				scrup(scr.top, 1);
			} else {
				y++;
			}
		}
		break; case RI: {
			if (y == scr.top) {
				scrdn(scr.top, 1);
			} else {
				y--;
			}
		}

		break; case SM: mode |= pmode();
		break; case RM: mode &= ~pmode();
		break; case DECSET: mode |= pdmode();
		break; case DECRST: mode &= ~pdmode();

		break; case SGR: {
			if (!pn) ps[pn++] = 0;
			for (int i = 0; i < pn; ++i) {
				switch (ps[i]) {
					break; case 0: sgr.attr = 0; sgr.bg = -1; sgr.fg = -1;
					break; case 1: sgr.attr |= Bold;
					break; case 3: sgr.attr |= Italic;
					break; case 4: sgr.attr |= Underline;
					break; case 7: sgr.attr |= Reverse;
					break; case 22: sgr.attr &= ~Bold;
					break; case 23: sgr.attr &= ~Italic;
					break; case 24: sgr.attr &= ~Underline;
					break; case 27: sgr.attr &= ~Reverse;
					break; case 30 ... 37: sgr.fg = ps[i] - 30;
					break; case 38: {
						if (++i < pn && ps[i] == 5) {
							if (++i < pn) sgr.fg = ps[i];
						}
					}
					break; case 39: sgr.fg = -1;
					break; case 40 ... 47: sgr.bg = ps[i] - 40;
					break; case 48: {
						if (++i < pn && ps[i] == 5) {
							if (++i < pn) sgr.bg = ps[i];
						}
					}
					break; case 49: sgr.bg = -1;
					break; case 90 ... 97: sgr.fg = 8 + ps[i] - 90;
					break; case 100 ... 107: sgr.bg = 8 + ps[i] - 100;
					break; default: warnx("unhandled SGR %d", ps[i]);
				}
			}
		}

		break; case Data: {
			int w = wcwidth(ch);
			if (w < 0) {
				warnx("unhandled \\u%04X", ch);
				return;
			}
			if (mode & Insert) {
				int n = bound(0, w, cols-x);
				move(cell(y, x+n), cell(y, x), cols-x-n);
			}
			if (mode & Wrap && x+w > cols) {
				update(CR);
				update(NL);
			}
			*cell(y, x) = sgr;
			cell(y, x)->ch = ch;
			for (int i = 1; i < w && x+i < cols; ++i) {
				*cell(y, x+i) = sgr;
				cell(y, x+i)->ch = L'\0';
			}
			x = bound(0, x+w, (mode & Wrap ? cols : cols-1));
			return;
		}
		break; case MC:;
	}

	x = bound(0, x, cols-1);
	y = bound(0, y, rows-1);
}

static bool bright;
static bool colors;
static int defaultBg = 0;
static int defaultFg = 7;

static const unsigned Palette[256] = {
	0x000000, 0xCD0000, 0x00CD00, 0xCDCD00, 0x0000EE, 0xCD00CD, 0x00CDCD,
	0xE5E5E5, 0x7F7F7F, 0xFF0000, 0x00FF00, 0xFFFF00, 0x5C5CFF, 0xFF00FF,
	0x00FFFF, 0xFFFFFF, 0x000000, 0x00005F, 0x000087, 0x0000AF, 0x0000D7,
	0x0000FF, 0x005F00, 0x005F5F, 0x005F87, 0x005FAF, 0x005FD7, 0x005FFF,
	0x008700, 0x00875F, 0x008787, 0x0087AF, 0x0087D7, 0x0087FF, 0x00AF00,
	0x00AF5F, 0x00AF87, 0x00AFAF, 0x00AFD7, 0x00AFFF, 0x00D700, 0x00D75F,
	0x00D787, 0x00D7AF, 0x00D7D7, 0x00D7FF, 0x00FF00, 0x00FF5F, 0x00FF87,
	0x00FFAF, 0x00FFD7, 0x00FFFF, 0x5F0000, 0x5F005F, 0x5F0087, 0x5F00AF,
	0x5F00D7, 0x5F00FF, 0x5F5F00, 0x5F5F5F, 0x5F5F87, 0x5F5FAF, 0x5F5FD7,
	0x5F5FFF, 0x5F8700, 0x5F875F, 0x5F8787, 0x5F87AF, 0x5F87D7, 0x5F87FF,
	0x5FAF00, 0x5FAF5F, 0x5FAF87, 0x5FAFAF, 0x5FAFD7, 0x5FAFFF, 0x5FD700,
	0x5FD75F, 0x5FD787, 0x5FD7AF, 0x5FD7D7, 0x5FD7FF, 0x5FFF00, 0x5FFF5F,
	0x5FFF87, 0x5FFFAF, 0x5FFFD7, 0x5FFFFF, 0x870000, 0x87005F, 0x870087,
	0x8700AF, 0x8700D7, 0x8700FF, 0x875F00, 0x875F5F, 0x875F87, 0x875FAF,
	0x875FD7, 0x875FFF, 0x878700, 0x87875F, 0x878787, 0x8787AF, 0x8787D7,
	0x8787FF, 0x87AF00, 0x87AF5F, 0x87AF87, 0x87AFAF, 0x87AFD7, 0x87AFFF,
	0x87D700, 0x87D75F, 0x87D787, 0x87D7AF, 0x87D7D7, 0x87D7FF, 0x87FF00,
	0x87FF5F, 0x87FF87, 0x87FFAF, 0x87FFD7, 0x87FFFF, 0xAF0000, 0xAF005F,
	0xAF0087, 0xAF00AF, 0xAF00D7, 0xAF00FF, 0xAF5F00, 0xAF5F5F, 0xAF5F87,
	0xAF5FAF, 0xAF5FD7, 0xAF5FFF, 0xAF8700, 0xAF875F, 0xAF8787, 0xAF87AF,
	0xAF87D7, 0xAF87FF, 0xAFAF00, 0xAFAF5F, 0xAFAF87, 0xAFAFAF, 0xAFAFD7,
	0xAFAFFF, 0xAFD700, 0xAFD75F, 0xAFD787, 0xAFD7AF, 0xAFD7D7, 0xAFD7FF,
	0xAFFF00, 0xAFFF5F, 0xAFFF87, 0xAFFFAF, 0xAFFFD7, 0xAFFFFF, 0xD70000,
	0xD7005F, 0xD70087, 0xD700AF, 0xD700D7, 0xD700FF, 0xD75F00, 0xD75F5F,
	0xD75F87, 0xD75FAF, 0xD75FD7, 0xD75FFF, 0xD78700, 0xD7875F, 0xD78787,
	0xD787AF, 0xD787D7, 0xD787FF, 0xD7AF00, 0xD7AF5F, 0xD7AF87, 0xD7AFAF,
	0xD7AFD7, 0xD7AFFF, 0xD7D700, 0xD7D75F, 0xD7D787, 0xD7D7AF, 0xD7D7D7,
	0xD7D7FF, 0xD7FF00, 0xD7FF5F, 0xD7FF87, 0xD7FFAF, 0xD7FFD7, 0xD7FFFF,
	0xFF0000, 0xFF005F, 0xFF0087, 0xFF00AF, 0xFF00D7, 0xFF00FF, 0xFF5F00,
	0xFF5F5F, 0xFF5F87, 0xFF5FAF, 0xFF5FD7, 0xFF5FFF, 0xFF8700, 0xFF875F,
	0xFF8787, 0xFF87AF, 0xFF87D7, 0xFF87FF, 0xFFAF00, 0xFFAF5F, 0xFFAF87,
	0xFFAFAF, 0xFFAFD7, 0xFFAFFF, 0xFFD700, 0xFFD75F, 0xFFD787, 0xFFD7AF,
	0xFFD7D7, 0xFFD7FF, 0xFFFF00, 0xFFFF5F, 0xFFFF87, 0xFFFFAF, 0xFFFFD7,
	0xFFFFFF, 0x080808, 0x121212, 0x1C1C1C, 0x262626, 0x303030, 0x3A3A3A,
	0x444444, 0x4E4E4E, 0x585858, 0x626262, 0x6C6C6C, 0x767676, 0x808080,
	0x8A8A8A, 0x949494, 0x9E9E9E, 0xA8A8A8, 0xB2B2B2, 0xBCBCBC, 0xC6C6C6,
	0xD0D0D0, 0xDADADA, 0xE4E4E4, 0xEEEEEE,
};

static void span(const struct Cell *prev, const struct Cell *cell) {
	if (
		!prev ||
		cell->attr != prev->attr ||
		cell->bg != prev->bg ||
		cell->fg != prev->fg
	) {
		if (prev) printf("</span>");
		int attr = cell->attr;
		int bg = (attr & Reverse ? cell->fg : cell->bg);
		int fg = (attr & Reverse ? cell->bg : cell->fg);
		if (bg < 0) bg = (attr & Reverse ? defaultFg : defaultBg);
		if (fg < 0) fg = (attr & Reverse ? defaultBg : defaultFg);
		if (bright && cell->attr & Bold) {
			if (fg < 8) fg += 8;
			attr &= ~Bold;
		}
		printf(Q(<span class="bg%d fg%d"), bg, fg);
		if (attr || colors) printf(" style=\"");
		if (attr & Bold) printf("font-weight:bold;");
		if (attr & Italic) printf("font-style:italic;");
		if (attr & Underline) printf("text-decoration:underline;");
		if (colors && bg < 256 && fg < 256) {
			printf(
				"background-color:#%06X;color:#%06X;",
				Palette[bg], Palette[fg]
			);
		}
		printf("%s>", (attr || colors ? "\"" : ""));
	}
	switch (cell->ch) {
		break; case L'&': printf("&amp;");
		break; case L'<': printf("&lt;");
		break; case L'>': printf("&gt;");
		break; case L'"': printf("&quot;");
		break; default: printf("%lc", (wint_t)cell->ch);
	}
}

static void html(void) {
	if (mode & Cursor) cell(y, x)->attr ^= Reverse;
	printf(
		Q(<pre style="width: %dch;" class="bg%d fg%d">),
		cols, defaultBg, defaultFg
	);
	for (int y = 0; y < rows; ++y) {
		for (int x = 0; x < cols; ++x) {
			if (!cell(y, x)->ch) continue;
			span((x ? cell(y, x-1) : NULL), cell(y, x));
		}
		printf("</span>\n");
	}
	printf("</pre>\n");
	if (mode & Cursor) cell(y, x)->attr ^= Reverse;
}

static const char *Debug[] = {
#define X(code) [code] = #code,
	ENUM_CODE
#undef X
};

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");

	bool debug = false;
	bool size = false;
	bool hide = false;

	for (int opt; 0 < (opt = getopt(argc, argv, "Bb:df:h:insw:"));) {
		switch (opt) {
			break; case 'B': bright = true;
			break; case 'b': defaultBg = atoi(optarg);
			break; case 'd': debug = true;
			break; case 'f': defaultFg = atoi(optarg);
			break; case 'h': rows = atoi(optarg);
			break; case 'i': colors = true;
			break; case 'n': hide = true;
			break; case 's': size = true;
			break; case 'w': cols = atoi(optarg);
			break; default:  return EX_USAGE;
		}
	}
	if (optind < argc) {
		yyin = fopen(argv[optind], "r");
		if (!yyin) err(EX_NOINPUT, "%s", argv[optind]);
	}

	if (size) {
		struct winsize win;
		int error = ioctl(STDERR_FILENO, TIOCGWINSZ, &win);
		if (error) err(EX_IOERR, "ioctl");
		cols = win.ws_col;
		rows = win.ws_row;
	}
	scr.bot = rows;

	cells = calloc(cols * rows, sizeof(*cells));
	if (!cells) err(EX_OSERR, "calloc");
	erase(cell(0, 0), cell(rows-1, cols));

	bool mc = false;
	for (int cc; (cc = yylex());) {
		if (cc == MC) {
			mc = true;
			html();
		} else {
			update(cc);
		}
		if (debug && cc != Data) {
			printf("%s", Debug[cc]);
			for (int i = 0; i < pn; ++i) {
				printf("%s%d", (i ? ", " : " "), ps[i]);
			}
			printf("\n");
			html();
		}
	}
	if (hide) mode &= ~Cursor;
	if (!mc) html();
}
