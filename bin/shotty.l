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

%option noyywrap

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
	(void)input;
	(void)yyunput;
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
static int defaultBg = 0;
static int defaultFg = 7;

static void span(const struct Cell *prev, const struct Cell *cell) {
	if (
		!prev ||
		cell->attr != prev->attr ||
		cell->bg != prev->bg ||
		cell->fg != prev->fg
	) {
		if (prev) printf("</span>");
		int attr = cell->attr;
		int bg = (cell->bg < 0 ? defaultBg : cell->bg);
		int fg = (cell->fg < 0 ? defaultFg : cell->fg);
		if (bright && cell->attr & Bold) {
			if (fg < 8) fg += 8;
			attr &= ~Bold;
		}
		printf(
			Q(<span style="%s%s%s" class="bg%d fg%d">),
			(attr & Bold ? "font-weight:bold;" : ""),
			(attr & Italic ? "font-style:italic;" : ""),
			(attr & Underline ? "text-decoration:underline;" : ""),
			(attr & Reverse ? fg : bg), (attr & Reverse ? bg : fg)
		);
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

	for (int opt; 0 < (opt = getopt(argc, argv, "Bb:df:h:nsw:"));) {
		switch (opt) {
			break; case 'B': bright = true;
			break; case 'b': defaultBg = atoi(optarg);
			break; case 'd': debug = true;
			break; case 'f': defaultFg = atoi(optarg);
			break; case 'h': rows = atoi(optarg);
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
