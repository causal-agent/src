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
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <locale.h>
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
#include <wchar.h>

static const wchar_t CP437[256] =
	L"\0☺☻♥♦♣♠•◘○◙♂♀♪♫☼"
	L"►◄↕‼¶§▬↨↑↓→←∟↔▲▼"
	L" !\"#$%&'()*+,-./"
	L"0123456789:;<=>?"
	L"@ABCDEFGHIJKLMNO"
	L"PQRSTUVWXYZ[\\]^_"
	L"`abcdefghijklmno"
	L"pqrstuvwxyz{|}~⌂"
	L"ÇüéâäàåçêëèïîìÄÅ"
	L"ÉæÆôöòûùÿÖÜ¢£¥₧ƒ"
	L"áíóúñÑªº¿⌐¬½¼¡«»"
	L"░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
	L"└┴┬├─┼╞╟╚╔╩╦╠═╬╧"
	L"╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
	L"αßΓπΣσµτΦΘΩδ∞φε∩"
	L"≡±≥≤⌠⌡÷≈°∙·√ⁿ²■\0";

static struct {
	uint32_t width;
	uint32_t height;
	uint32_t *buffer;
	uint32_t background;
} frame;

static void frameClear(void) {
	for (uint32_t i = 0; i < frame.width * frame.height; ++i) {
		frame.buffer[i] = frame.background;
	}
}

static void frameOpen(void) {
	const char *dev = getenv("FRAMEBUFFER");
	if (!dev) dev = "/dev/fb0";

	int fd = open(dev, O_RDWR);
	if (fd < 0) err(EX_OSFILE, "%s", dev);

	struct fb_var_screeninfo info;
	int error = ioctl(fd, FBIOGET_VSCREENINFO, &info);
	if (error) err(EX_IOERR, "%s", dev);

	frame.width = info.xres;
	frame.height = 3 * info.yres / 4;
	frame.buffer = mmap(
		NULL, sizeof(*frame.buffer) * frame.width * frame.height,
		PROT_READ | PROT_WRITE, MAP_SHARED,
		fd, 0
	);
	if (frame.buffer == MAP_FAILED) err(EX_IOERR, "%s", dev);
	close(fd);

	frame.background = frame.buffer[0];
	atexit(frameClear);
}

static const uint32_t Magic = 0x864AB572;
static const uint32_t Version = 0;
static const uint32_t FlagUnicode = 1 << 0;
static uint32_t bytes(uint32_t bits) {
	return (bits + 7) / 8;
}

static char *path;
static struct {
	uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint32_t flags;
	struct {
		uint32_t len;
		uint32_t size;
		uint32_t height;
		uint32_t width;
	} glyph;
} header;
static uint8_t *glyphs;

static void fileRead(uint32_t newLen, uint32_t newWidth, uint32_t newHeight) {
	FILE *file = fopen(path, "r");
	if (file) {
		size_t len = fread(&header, sizeof(header), 1, file);
		if (ferror(file)) err(EX_IOERR, "%s", path);
		if (len < 1) errx(EX_DATAERR, "%s: truncated header", path);

	} else {
		if (errno != ENOENT) err(EX_NOINPUT, "%s", path);
		header.magic = Magic;
		header.version = Version;
		header.size = sizeof(header);
		header.flags = 0;
		header.glyph.len = newLen;
		header.glyph.size = bytes(newWidth) * newHeight;
		header.glyph.height = newHeight;
		header.glyph.width = newWidth;
	}

	if (header.magic != Magic) {
		errx(EX_DATAERR, "%s: invalid magic %08X", path, header.magic);
	}
	if (header.version != Version) {
		errx(EX_DATAERR, "%s: unsupported version %u", path, header.version);
	}
	if (header.flags & FlagUnicode) {
		errx(EX_DATAERR, "%s: unsupported unicode table", path);
	}
	if (header.flags) {
		errx(EX_DATAERR, "%s: unsupported flags %08X", path, header.flags);
	}

	if (file && header.size > sizeof(header)) {
		int error = fseek(file, header.size, SEEK_SET);
		if (error) err(EX_IOERR, "%s", path);

		warnx("%s: truncating long header", path);
		header.size = sizeof(header);
	}

	glyphs = calloc(header.glyph.len, header.glyph.size);
	if (!glyphs) err(EX_OSERR, "calloc");

	if (file) {
		size_t len = fread(glyphs, header.glyph.size, header.glyph.len, file);
		if (ferror(file)) err(EX_IOERR, "%s", path);
		if (len < header.glyph.len) {
			errx(EX_DATAERR, "%s: truncated glyphs", path);
		}
		fclose(file);
	}
}

static void fileWrite(void) {
	FILE *file = fopen(path, "w");
	if (!file) err(EX_CANTCREAT, "%s", path);

	fwrite(&header, sizeof(header), 1, file);
	if (ferror(file)) err(EX_IOERR, "%s", path);

	fwrite(glyphs, header.glyph.size, header.glyph.len, file);
	if (ferror(file)) err(EX_IOERR, "%s", path);

	int error = fclose(file);
	if (error) err(EX_IOERR, "%s", path);
}

static uint8_t *glyph(uint32_t index) {
	return &glyphs[header.glyph.size * index];
}
static uint8_t *bitByte(uint32_t index, uint32_t x, uint32_t y) {
	return &glyph(index)[bytes(header.glyph.width) * y + x / 8];
}
static uint8_t bitGet(uint32_t index, uint32_t x, uint32_t y) {
	return *bitByte(index, x, y) >> (7 - x % 8) & 1;
}
static void bitFlip(uint32_t index, uint32_t x, uint32_t y) {
	*bitByte(index, x, y) ^= 1 << (7 - x % 8);
}

static void drawGlyph(
	uint32_t destX, uint32_t destY, uint32_t scale,
	uint32_t index, bool select, uint32_t selectX, uint32_t selectY
) {
	destX <<= scale;
	destY <<= scale;

	for (uint32_t y = 0; y < (header.glyph.height << scale); ++y) {
		if (destY + y >= frame.height) break;
		for (uint32_t x = 0; x < (header.glyph.width << scale); ++x) {
			if (destX + x >= frame.width) break;

			uint32_t glyphX = x >> scale;
			uint32_t glyphY = y >> scale;
			uint32_t fill = -bitGet(index, glyphX, glyphY);
			if (select) fill ^= 0x77;
			if (selectX == glyphX && selectY == glyphY) fill ^= 0x77;

			frame.buffer[frame.width * (destY + y) + destX + x] = fill;
		}
	}
}

static void drawBorder(uint32_t destX, uint32_t destY, uint32_t scale) {
	destX <<= scale;
	destY <<= scale;

	for (uint32_t y = 0; y < destY; ++y) {
		if (y >= frame.height) break;
		uint32_t fill = -(y >> scale & 1) ^ 0x555555;
		for (uint32_t x = 0; x < (uint32_t)(1 << scale); ++x) {
			if (destX + x >= frame.width) break;
			frame.buffer[frame.width * y + destX + x] = fill;
		}
	}

	for (uint32_t x = 0; x < destX; ++x) {
		if (x >= frame.width) break;
		uint32_t fill = -(x >> scale & 1) ^ 0x555555;
		for (uint32_t y = 0; y < (uint32_t)(1 << scale); ++y) {
			if (destY + y >= frame.height) break;
			frame.buffer[frame.width * (destY + y) + x] = fill;
		}
	}
}

enum { LF = '\n', ESC = '\33', DEL = '\177' };

static enum {
	Normal,
	Edit,
	Preview,
	Discard,
} mode;

static struct {
	uint32_t scale;
	uint32_t index;
	bool modified;
} normal;

static struct {
	uint32_t scale;
	uint32_t index;
	uint32_t x;
	uint32_t y;
	uint8_t *undo;
} edit = {
	.scale = 4,
};

static const uint32_t NormalCols = 32;
static void drawNormal(void) {
	for (uint32_t i = 0; i < header.glyph.len; ++i) {
		drawGlyph(
			header.glyph.width * (i % NormalCols),
			header.glyph.height * (i  / NormalCols),
			normal.scale,
			i, (i == normal.index), -1, -1
		);
	}
}

static void normalDec(uint32_t n) {
	if (normal.index >= n) normal.index -= n;
}
static void normalInc(uint32_t n) {
	if (normal.index + n < header.glyph.len) normal.index += n;
}
static void normalPrint(void) {
	if (normal.index <= 256) {
		printf("index: %02X '%lc'\n", normal.index, CP437[normal.index]);
	} else {
		printf("index: %02X\n", normal.index);
	}
}

static void inputNormal(char ch) {
	switch (ch) {
		break; case 'q': {
			if (!normal.modified) exit(EX_OK);
			mode = Discard;
		}
		break; case 'w': {
			fileWrite();
			printf("write: %s\n", path);
			normal.modified = false;
		}
		break; case '-': if (normal.scale) normal.scale--; frameClear();
		break; case '+': normal.scale++;
		break; case 'h': normalDec(1); normalPrint();
		break; case 'l': normalInc(1); normalPrint();
		break; case 'k': normalDec(NormalCols); normalPrint();
		break; case 'j': normalInc(NormalCols); normalPrint();
		break; case 'e': {
			normal.modified = true;
			edit.index = normal.index;
			if (!edit.undo) edit.undo = malloc(header.glyph.size);
			if (!edit.undo) err(EX_OSERR, "malloc");
			memcpy(edit.undo, glyph(edit.index), header.glyph.size);
			mode = Edit;
			frameClear();
		}
		break; case 'i': mode = Preview; frameClear();
	}
}

static void drawEdit(void) {
	drawGlyph(0, 0, edit.scale, edit.index, false, edit.x, edit.y);
	drawBorder(header.glyph.width, header.glyph.height, edit.scale);
	drawGlyph(
		header.glyph.width << edit.scale,
		header.glyph.height << edit.scale,
		0,
		edit.index, false, -1, -1
	);
}

static void inputEdit(char ch) {
	switch (ch) {
		break; case ESC: mode = Normal; frameClear();
		break; case '-': if (edit.scale) edit.scale--; frameClear();
		break; case '+': edit.scale++;
		break; case 'h': if (edit.x) edit.x--;
		break; case 'l': if (edit.x + 1 < header.glyph.width) edit.x++;
		break; case 'k': if (edit.y) edit.y--;
		break; case 'j': if (edit.y + 1 < header.glyph.height) edit.y++;
		break; case ' ': bitFlip(edit.index, edit.x, edit.y);
		break; case 'u': {
			memcpy(glyph(edit.index), edit.undo, header.glyph.size);
		}
	}
}

enum { PreviewRows = 8, PreviewCols = 64 };
static struct {
	uint32_t glyphs[PreviewRows * PreviewCols];
	uint32_t index;
} preview;

static void drawPreview(void) {
	for (uint32_t i = 0; i < PreviewRows * PreviewCols; ++i) {
		drawGlyph(
			header.glyph.width * (i % PreviewCols),
			header.glyph.height * (i / PreviewCols),
			0,
			preview.glyphs[i], (i == preview.index), -1, -1
		);
	}
}

static void inputPreview(char ch) {
	switch (ch) {
		break; case ESC: mode = Normal; frameClear();
		break; case DEL: {
			if (preview.index) preview.index--;
			preview.glyphs[preview.index] = 0;
		}
		break; case LF: {
			uint32_t tail = PreviewCols - (preview.index % PreviewCols);
			memset(
				&preview.glyphs[preview.index],
				0, sizeof(preview.glyphs[0]) * tail
			);
			preview.index += tail;
		}
		break; default: preview.glyphs[preview.index++] = ch;
	}
	preview.index %= PreviewRows * PreviewCols;
}

static void drawDiscard(void) {
	printf("discard modifications? ");
	fflush(stdout);
}

static void inputDiscard(char ch) {
	printf("%c\n", ch);
	if (ch == 'Y' || ch == 'y') exit(EX_OK);
	mode = Normal;
}

static void draw(void) {
	switch (mode) {
		break; case Normal: drawNormal();
		break; case Edit: drawEdit();
		break; case Preview: drawPreview();
		break; case Discard: drawDiscard();
	}
}

static void input(char ch) {
	switch (mode) {
		break; case Normal: inputNormal(ch);
		break; case Edit: inputEdit(ch);
		break; case Preview: inputPreview(ch);
		break; case Discard: inputDiscard(ch);
	}
}

static struct termios saveTerm;
static void restoreTerm(void) {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");

	uint32_t newLen = 256;
	uint32_t newWidth = 8;
	uint32_t newHeight = 16;

	int opt;
	while (0 < (opt = getopt(argc, argv, "g:h:w:"))) {
		switch (opt) {
			break; case 'g': newLen = strtoul(optarg, NULL, 0);
			break; case 'h': newHeight = strtoul(optarg, NULL, 0);
			break; case 'w': newWidth = strtoul(optarg, NULL, 0);
			break; default:  return EX_USAGE;
		}
	}
	if (!newLen || !newWidth || !newHeight) return EX_USAGE;
	if (optind == argc) return EX_USAGE;

	path = strdup(argv[optind]);
	fileRead(newLen, newWidth, newHeight);

	frameOpen();
	frameClear();

	int error = tcgetattr(STDIN_FILENO, &saveTerm);
	if (error) err(EX_IOERR, "tcgetattr");
	atexit(restoreTerm);

	struct termios term = saveTerm;
	term.c_lflag &= ~(ICANON | ECHO);
	error = tcsetattr(STDIN_FILENO, TCSADRAIN, &term);
	if (error) err(EX_IOERR, "tcsetattr");

	for (;;) {
		draw();
		char ch;
		ssize_t size = read(STDIN_FILENO, &ch, 1);
		if (size < 0) err(EX_IOERR, "read");
		if (!size) return EX_SOFTWARE;
		input(ch);
	}
}
