/* Copyright (C) 2018, 2021  June McEnroe <june@causal.agency>
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
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <zlib.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

static bool verbose;
static const char *path;
static FILE *file;
static uint32_t crc;

static void pngRead(void *ptr, size_t len, const char *desc) {
	size_t n = fread(ptr, len, 1, file);
	if (!n && ferror(file)) err(EX_IOERR, "%s", path);
	if (!n) errx(EX_DATAERR, "%s: missing %s", path, desc);
	crc = crc32(crc, ptr, len);
}

static void pngWrite(const void *ptr, size_t len) {
	size_t n = fwrite(ptr, len, 1, file);
	if (!n) err(EX_IOERR, "%s", path);
	crc = crc32(crc, ptr, len);
}

static const uint8_t Sig[8] = "\x89PNG\r\n\x1A\n";

static void sigRead(void) {
	uint8_t sig[sizeof(Sig)];
	pngRead(sig, sizeof(sig), "signature");
	if (memcmp(sig, Sig, sizeof(sig))) {
		errx(EX_DATAERR, "%s: invalid signature", path);
	}
}

static void sigWrite(void) {
	pngWrite(Sig, sizeof(Sig));
}

static uint32_t u32Read(const char *desc) {
	uint8_t b[4];
	pngRead(b, sizeof(b), desc);
	return (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16
		| (uint32_t)b[2] << 8 | (uint32_t)b[3];
}

static void u32Write(uint32_t x) {
	uint8_t b[4] = { x >> 24 & 0xFF, x >> 16 & 0xFF, x >> 8 & 0xFF, x & 0xFF };
	pngWrite(b, sizeof(b));
}

struct Chunk {
	uint32_t len;
	char type[5];
};

static struct Chunk chunkRead(void) {
	struct Chunk chunk;
	chunk.len = u32Read("chunk length");
	crc = crc32(0, Z_NULL, 0);
	pngRead(chunk.type, 4, "chunk type");
	chunk.type[4] = 0;
	return chunk;
}

static void chunkWrite(struct Chunk chunk) {
	u32Write(chunk.len);
	crc = crc32(0, Z_NULL, 0);
	pngWrite(chunk.type, 4);
}

static void crcRead(void) {
	uint32_t expect = crc;
	uint32_t actual = u32Read("CRC32");
	if (actual == expect) return;
	errx(
		EX_DATAERR, "%s: expected CRC32 %08X, found %08X",
		path, expect, actual
	);
}

static void crcWrite(void) {
	u32Write(crc);
}

static void chunkSkip(struct Chunk chunk) {
	if (!(chunk.type[0] & 0x20)) {
		errx(EX_CONFIG, "%s: unsupported critical chunk %s", path, chunk.type);
	}
	uint8_t buf[4096];
	while (chunk.len > sizeof(buf)) {
		pngRead(buf, sizeof(buf), "chunk data");
		chunk.len -= sizeof(buf);
	}
	if (chunk.len) pngRead(buf, chunk.len, "chunk data");
	crcRead();
}

enum Color {
	Grayscale = 0,
	Truecolor = 2,
	Indexed = 3,
	GrayscaleAlpha = 4,
	TruecolorAlpha = 6,
};
enum Compression {
	Deflate,
};
enum FilterMethod {
	Adaptive,
};
enum Interlace {
	Progressive,
	Adam7,
};

enum { HeaderLen = 13 };
static struct {
	uint32_t width;
	uint32_t height;
	uint8_t depth;
	uint8_t color;
	uint8_t compression;
	uint8_t filter;
	uint8_t interlace;
} header;

static size_t pixelBits(void) {
	switch (header.color) {
		case Grayscale:
		case Indexed:
			return 1 * header.depth;
		case GrayscaleAlpha:
			return 2 * header.depth;
		case Truecolor:
			return 3 * header.depth;
		case TruecolorAlpha:
			return 4 * header.depth;
		default:
			abort();
	}
}
static size_t pixelLen(void) {
	return (pixelBits() + 7) / 8;
}
static size_t lineLen(void) {
	return (header.width * pixelBits() + 7) / 8;
}
static size_t dataLen(void) {
	return (1 + lineLen()) * header.height;
}

static void headerPrint(void) {
	static const char *String[] = {
		[Grayscale] = "grayscale",
		[Truecolor] = "truecolor",
		[Indexed] = "indexed",
		[GrayscaleAlpha] = "grayscale alpha",
		[TruecolorAlpha] = "truecolor alpha",
	};
	fprintf(
		stderr, "%s: %" PRIu32 "x%" PRIu32 " %" PRIu8 "-bit %s\n",
		path, header.width, header.height, header.depth, String[header.color]
	);
}

static void headerRead(struct Chunk chunk) {
	if (chunk.len != HeaderLen) {
		errx(
			EX_DATAERR, "%s: expected %s length %" PRIu32 ", found %" PRIu32,
			path, chunk.type, (uint32_t)HeaderLen, chunk.len
		);
	}
	header.width = u32Read("header width");
	header.height = u32Read("header height");
	pngRead(&header.depth, 1, "header depth");
	pngRead(&header.color, 1, "header color");
	pngRead(&header.compression, 1, "header compression");
	pngRead(&header.filter, 1, "header filter");
	pngRead(&header.interlace, 1, "header interlace");
	crcRead();

	if (!header.width) errx(EX_DATAERR, "%s: invalid width 0", path);
	if (!header.height) errx(EX_DATAERR, "%s: invalid height 0", path);
	static const struct {
		uint8_t color;
		uint8_t depth;
	} Valid[] = {
		{ Grayscale, 1 },
		{ Grayscale, 2 },
		{ Grayscale, 4 },
		{ Grayscale, 8 },
		{ Grayscale, 16 },
		{ Truecolor, 8 },
		{ Truecolor, 16 },
		{ Indexed, 1 },
		{ Indexed, 2 },
		{ Indexed, 4 },
		{ Indexed, 8 },
		{ Indexed, 16 },
		{ GrayscaleAlpha, 8 },
		{ GrayscaleAlpha, 16 },
		{ TruecolorAlpha, 8 },
		{ TruecolorAlpha, 16 },
	};
	bool valid = false;
	for (size_t i = 0; i < ARRAY_LEN(Valid); ++i) {
		valid = (
			header.color == Valid[i].color &&
			header.depth == Valid[i].depth
		);
		if (valid) break;
	}
	if (!valid) {
		errx(
			EX_DATAERR,
			"%s: invalid color type %" PRIu8 " and bit depth %" PRIu8,
			path, header.color, header.depth
		);
	}
	if (header.compression != Deflate) {
		errx(
			EX_DATAERR, "%s: invalid compression method %" PRIu8,
			path, header.compression
		);
	}
	if (header.filter != Adaptive) {
		errx(
			EX_DATAERR, "%s: invalid filter method %" PRIu8,
			path, header.filter
		);
	}
	if (header.interlace > Adam7) {
		errx(
			EX_DATAERR, "%s: invalid interlace method %" PRIu8,
			path, header.interlace
		);
	}

	if (verbose) headerPrint();
}

static void headerWrite(void) {
	if (verbose) headerPrint();

	struct Chunk ihdr = { HeaderLen, "IHDR" };
	chunkWrite(ihdr);
	u32Write(header.width);
	u32Write(header.height);
	pngWrite(&header.depth, 1);
	pngWrite(&header.color, 1);
	pngWrite(&header.compression, 1);
	pngWrite(&header.filter, 1);
	pngWrite(&header.interlace, 1);
	crcWrite();
}

static struct {
	uint32_t len;
	uint8_t rgb[256][3];
} pal;

static struct {
	uint32_t len;
	uint8_t a[256];
} trans;

static void palClear(void) {
	pal.len = 0;
	trans.len = 0;
}

static uint32_t palIndex(bool alpha, const uint8_t *rgba) {
	uint32_t i;
	for (i = 0; i < pal.len; ++i) {
		if (alpha && i < trans.len && trans.a[i] != rgba[3]) continue;
		if (!memcmp(pal.rgb[i], rgba, 3)) break;
	}
	return i;
}

static bool palAdd(bool alpha, const uint8_t *rgba) {
	uint32_t i = palIndex(alpha, rgba);
	if (i < pal.len) return true;
	if (i == 256) return false;
	memcpy(pal.rgb[i], rgba, 3);
	pal.len++;
	if (alpha) {
		trans.a[i] = rgba[3];
		trans.len++;
	}
	return true;
}

static void transCompact(void) {
	uint32_t i;
	for (i = 0; i < trans.len; ++i) {
		if (trans.a[i] == 0xFF) break;
	}
	if (i == trans.len) return;

	for (uint32_t j = i+1; j < trans.len; ++j) {
		if (trans.a[j] == 0xFF) continue;
		uint8_t a = trans.a[i];
		trans.a[i] = trans.a[j];
		trans.a[j] = a;
		uint8_t rgb[3];
		memcpy(rgb, pal.rgb[i], 3);
		memcpy(pal.rgb[i], pal.rgb[j], 3);
		memcpy(pal.rgb[j], rgb, 3);
		i++;
	}
	trans.len = i;
}

static void palRead(struct Chunk chunk) {
	if (chunk.len % 3) {
		errx(
			EX_DATAERR, "%s: %s length %" PRIu32 " not divisible by 3",
			path, chunk.type, chunk.len
		);
	}
	pal.len = chunk.len / 3;
	if (pal.len > 256) {
		errx(
			EX_DATAERR, "%s: %s length %" PRIu32 " > 256",
			path, chunk.type, pal.len
		);
	}
	pngRead(pal.rgb, chunk.len, "palette data");
	crcRead();
	if (verbose) {
		fprintf(stderr, "%s: palette length %" PRIu32 "\n", path, pal.len);
	}
}

static void palWrite(void) {
	if (verbose) {
		fprintf(stderr, "%s: palette length %" PRIu32 "\n", path, pal.len);
	}
	struct Chunk plte = { 3 * pal.len, "PLTE" };
	chunkWrite(plte);
	pngWrite(pal.rgb, plte.len);
	crcWrite();
}

static void transRead(struct Chunk chunk) {
	trans.len = chunk.len;
	if (trans.len > 256) {
		errx(
			EX_DATAERR, "%s: %s length %" PRIu32 " > 256",
			path, chunk.type, trans.len
		);
	}
	pngRead(trans.a, chunk.len, "transparency data");
	crcRead();
	if (verbose) {
		fprintf(stderr, "%s: trans length %" PRIu32 "\n", path, trans.len);
	}
}

static void transWrite(void) {
	if (verbose) {
		fprintf(stderr, "%s: trans length %" PRIu32 "\n", path, trans.len);
	}
	struct Chunk trns = { trans.len, "tRNS" };
	chunkWrite(trns);
	pngWrite(trans.a, trns.len);
	crcWrite();
}

static uint8_t *data;

static void dataAlloc(void) {
	data = malloc(dataLen());
	if (!data) err(EX_OSERR, "malloc");
}

static const char *humanize(size_t n) {
	static char buf[64];
	if (n >> 10) {
		snprintf(buf, sizeof(buf), "%zuK", n >> 10);
	} else {
		snprintf(buf, sizeof(buf), "%zuB", n);
	}
	return buf;
}

static void dataRead(struct Chunk chunk) {
	if (verbose) {
		fprintf(stderr, "%s: data size %s\n", path, humanize(dataLen()));
	}

	z_stream stream = { .next_out = data, .avail_out = dataLen() };
	int error = inflateInit(&stream);
	if (error != Z_OK) errx(EX_SOFTWARE, "inflateInit: %s", stream.msg);

	for (;;) {
		if (strcmp(chunk.type, "IDAT")) {
			errx(EX_DATAERR, "%s: missing IDAT chunk", path);
		}

		uint8_t *idat = malloc(chunk.len);
		if (!idat) err(EX_OSERR, "malloc");

		pngRead(idat, chunk.len, "image data");
		crcRead();
		
		stream.next_in = idat;
		stream.avail_in = chunk.len;
		error = inflate(&stream, Z_SYNC_FLUSH);
		free(idat);

		if (error == Z_STREAM_END) break;
		if (error != Z_OK) {
			errx(EX_DATAERR, "%s: inflate: %s", path, stream.msg);
		}

		chunk = chunkRead();
	}
	inflateEnd(&stream);
	if ((size_t)stream.total_out != dataLen()) {
		errx(
			EX_DATAERR, "%s: expected data length %zu, found %zu",
			path, dataLen(), (size_t)stream.total_out
		);
	}

	if (verbose) {
		fprintf(
			stderr, "%s: deflate size %s\n",
			path, humanize(stream.total_in)
		);
	}
}

static void dataWrite(void) {
	if (verbose) {
		fprintf(stderr, "%s: data size %s\n", path, humanize(dataLen()));
	}

	uLong len = compressBound(dataLen());
	uint8_t *deflate = malloc(len);
	if (!deflate) err(EX_OSERR, "malloc");

	int error = compress2(deflate, &len, data, dataLen(), Z_BEST_COMPRESSION);
	if (error != Z_OK) errx(EX_SOFTWARE, "compress2: %d", error);

	struct Chunk idat = { len, "IDAT" };
	chunkWrite(idat);
	pngWrite(deflate, len);
	crcWrite();
	free(deflate);

	struct Chunk iend = { 0, "IEND" };
	chunkWrite(iend);
	crcWrite();

	if (verbose) fprintf(stderr, "%s: deflate size %s\n", path, humanize(len));
}

enum Filter {
	None,
	Sub,
	Up,
	Average,
	Paeth,
	FilterCap,
};

struct Bytes {
	uint8_t x, a, b, c;
};

static uint8_t paethPredictor(struct Bytes f) {
	int32_t p = (int32_t)f.a + (int32_t)f.b - (int32_t)f.c;
	int32_t pa = labs(p - (int32_t)f.a);
	int32_t pb = labs(p - (int32_t)f.b);
	int32_t pc = labs(p - (int32_t)f.c);
	if (pa <= pb && pa <= pc) return f.a;
	if (pb <= pc) return f.b;
	return f.c;
}

static uint8_t recon(enum Filter type, struct Bytes f) {
	switch (type) {
		case None:    return f.x;
		case Sub:     return f.x + f.a;
		case Up:      return f.x + f.b;
		case Average: return f.x + ((uint32_t)f.a + (uint32_t)f.b) / 2;
		case Paeth:   return f.x + paethPredictor(f);
		default: abort();
	}
}

static uint8_t filt(enum Filter type, struct Bytes f) {
	switch (type) {
		case None:    return f.x;
		case Sub:     return f.x - f.a;
		case Up:      return f.x - f.b;
		case Average: return f.x - ((uint32_t)f.a + (uint32_t)f.b) / 2;
		case Paeth:   return f.x - paethPredictor(f);
		default: abort();
	}
}

static uint8_t *lineType(uint32_t y) {
	return &data[y * (1 + lineLen())];
}
static uint8_t *lineData(uint32_t y) {
	return 1 + lineType(y);
}

static struct Bytes origBytes(uint32_t y, size_t i) {
	bool a = (i >= pixelLen()), b = (y > 0), c = (a && b);
	return (struct Bytes) {
		.x = lineData(y)[i],
		.a = (a ? lineData(y)[i-pixelLen()] : 0),
		.b = (b ? lineData(y-1)[i] : 0),
		.c = (c ? lineData(y-1)[i-pixelLen()] : 0),
	};
}

static void dataRecon(void) {
	for (uint32_t y = 0; y < header.height; ++y) {
		for (size_t i = 0; i < lineLen(); ++i) {
			lineData(y)[i] = recon(*lineType(y), origBytes(y, i));
		}
		*lineType(y) = None;
	}
}

static void dataFilter(void) {
	if (header.color == Indexed || header.depth < 8) return;
	uint8_t *filter[FilterCap];
	for (enum Filter i = None; i < FilterCap; ++i) {
		filter[i] = malloc(lineLen());
		if (!filter[i]) err(EX_OSERR, "malloc");
	}
	for (uint32_t y = header.height-1; y < header.height; --y) {
		uint32_t heuristic[FilterCap] = {0};
		enum Filter minType = None;
		for (enum Filter type = None; type < FilterCap; ++type) {
			for (size_t i = 0; i < lineLen(); ++i) {
				filter[type][i] = filt(type, origBytes(y, i));
				heuristic[type] += abs((int8_t)filter[type][i]);
			}
			if (heuristic[type] < heuristic[minType]) minType = type;
		}
		*lineType(y) = minType;
		memcpy(lineData(y), filter[minType], lineLen());
	}
	for (enum Filter i = None; i < FilterCap; ++i) {
		free(filter[i]);
	}
}

static bool alphaUnused(void) {
	if (header.color != GrayscaleAlpha && header.color != TruecolorAlpha) {
		return false;
	}
	size_t sampleLen = header.depth / 8;
	size_t colorLen = pixelLen() - sampleLen;
	for (uint32_t y = 0; y < header.height; ++y)
	for (uint32_t x = 0; x < header.width; ++x)
	for (size_t i = 0; i < sampleLen; ++i) {
		if (lineData(y)[x * pixelLen() + colorLen + i] != 0xFF) return false;
	}
	return true;
}

static void alphaDiscard(void) {
	if (header.color != GrayscaleAlpha && header.color != TruecolorAlpha) {
		return;
	}
	size_t sampleLen = header.depth / 8;
	size_t colorLen = pixelLen() - sampleLen;
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (uint32_t x = 0; x < header.width; ++x) {
			memmove(ptr, &lineData(y)[x * pixelLen()], colorLen);
			ptr += colorLen;
		}
	}
	header.color = (header.color == GrayscaleAlpha ? Grayscale : Truecolor);
}

static bool depth16Unused(void) {
	if (header.color != Grayscale && header.color != Truecolor) return false;
	if (header.depth != 16) return false;
	for (uint32_t y = 0; y < header.height; ++y)
	for (size_t i = 0; i < lineLen(); i += 2) {
		if (lineData(y)[i] != lineData(y)[i+1]) return false;
	}
	return true;
}

static void depth16Reduce(void) {
	if (header.depth != 16) return;
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (size_t i = 0; i < lineLen() / 2; ++i) {
			*ptr++ = lineData(y)[i*2];
		}
	}
	header.depth = 8;
}

static bool colorUnused(void) {
	if (header.color != Truecolor && header.color != TruecolorAlpha) {
		return false;
	}
	if (header.depth != 8) return false;
	for (uint32_t y = 0; y < header.height; ++y)
	for (uint32_t x = 0; x < header.width; ++x) {
		uint8_t r = lineData(y)[x * pixelLen() + 0];
		uint8_t g = lineData(y)[x * pixelLen() + 1];
		uint8_t b = lineData(y)[x * pixelLen() + 2];
		if (r != g || g != b) return false;
	}
	return true;
}

static void colorDiscard(void) {
	if (header.color != Truecolor && header.color != TruecolorAlpha) return;
	if (header.depth != 8) return;
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (uint32_t x = 0; x < header.width; ++x) {
			uint8_t r = lineData(y)[x * pixelLen() + 0];
			uint8_t g = lineData(y)[x * pixelLen() + 1];
			uint8_t b = lineData(y)[x * pixelLen() + 2];
			*ptr++ = ((uint32_t)r + (uint32_t)g + (uint32_t)b) / 3;
			if (header.color == TruecolorAlpha) {
				*ptr++ = lineData(y)[x * pixelLen() + 3];
			}
		}
	}
	header.color = (header.color == Truecolor ? Grayscale : GrayscaleAlpha);
}

static void colorIndex(void) {
	if (header.color != Truecolor && header.color != TruecolorAlpha) return;
	if (header.depth != 8) return;
	bool alpha = (header.color == TruecolorAlpha);
	for (uint32_t y = 0; y < header.height; ++y)
	for (uint32_t x = 0; x < header.width; ++x) {
		if (!palAdd(alpha, &lineData(y)[x * pixelLen()])) return;
	}

	transCompact();
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (uint32_t x = 0; x < header.width; ++x) {
			*ptr++ = palIndex(alpha, &lineData(y)[x * pixelLen()]);
		}
	}
	header.color = Indexed;
}

static bool depth8Unused(void) {
	if (header.depth != 8) return false;
	if (header.color == Indexed) return pal.len <= 16;
	if (header.color != Grayscale) return false;
	for (uint32_t y = 0; y < header.height; ++y)
	for (size_t i = 0; i < lineLen(); ++i) {
		if ((lineData(y)[i] >> 4) != (lineData(y)[i] & 0x0F)) return false;
	}
	return true;
}

static void depth8Reduce(void) {
	if (header.color != Grayscale && header.color != Indexed) return;
	if (header.depth != 8) return;
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (size_t i = 0; i < lineLen(); i += 2) {
			uint8_t a, b;
			uint8_t aa = lineData(y)[i];
			uint8_t bb = (i+1 < lineLen() ? lineData(y)[i+1] : 0);
			if (header.color == Grayscale) {
				a = aa >> 4;
				b = bb >> 4;
			} else {
				a = aa & 0x0F;
				b = bb & 0x0F;
			}
			*ptr++ = a << 4 | b;
		}
	}
	header.depth = 4;
}

static bool depth4Unused(void) {
	if (header.depth != 4) return false;
	if (header.color == Indexed) return pal.len <= 4;
	if (header.color != Grayscale) return false;
	for (uint32_t y = 0; y < header.height; ++y)
	for (size_t i = 0; i < lineLen(); ++i) {
		uint8_t a = lineData(y)[i] >> 4;
		uint8_t b = lineData(y)[i] & 0x0F;
		if ((a >> 2) != (a & 0x03)) return false;
		if ((b >> 2) != (b & 0x03)) return false;
	}
	return true;
}

static void depth4Reduce(void) {
	if (header.color != Grayscale && header.color != Indexed) return;
	if (header.depth != 4) return;
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (size_t i = 0; i < lineLen(); i += 2) {
			uint8_t a, b, c, d;
			uint8_t aabb = lineData(y)[i];
			uint8_t ccdd = (i+1 < lineLen() ? lineData(y)[i+1] : 0);
			if (header.color == Grayscale) {
				a = aabb >> 6;
				c = ccdd >> 6;
				b = aabb >> 2 & 0x03;
				d = ccdd >> 2 & 0x03;
			} else {
				a = aabb >> 4 & 0x03;
				c = ccdd >> 4 & 0x03;
				b = aabb & 0x03;
				d = ccdd & 0x03;
			}
			*ptr++ = a << 6 | b << 4 | c << 2 | d;
		}
	}
	header.depth = 2;
}

static bool depth2Unused(void) {
	if (header.depth != 2) return false;
	if (header.color == Indexed) return pal.len <= 2;
	if (header.color != Grayscale) return false;
	for (uint32_t y = 0; y < header.height; ++y)
	for (size_t i = 0; i < lineLen(); ++i) {
		uint8_t a = lineData(y)[i] >> 6;
		uint8_t b = lineData(y)[i] >> 4 & 0x03;
		uint8_t c = lineData(y)[i] >> 2 & 0x03;
		uint8_t d = lineData(y)[i] & 0x03;
		if ((a >> 1) != (a & 1)) return false;
		if ((b >> 1) != (b & 1)) return false;
		if ((c >> 1) != (c & 1)) return false;
		if ((d >> 1) != (d & 1)) return false;
	}
	return true;
}

static void depth2Reduce(void) {
	if (header.color != Grayscale && header.color != Indexed) return;
	if (header.depth != 2) return;
	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = *lineType(y);
		for (size_t i = 0; i < lineLen(); i += 2) {
			uint8_t a, b, c, d, e, f, g, h;
			uint8_t aabbccdd = lineData(y)[i];
			uint8_t eeffgghh = (i+1 < lineLen() ? lineData(y)[i+1] : 0);
			if (header.color == Grayscale) {
				a = aabbccdd >> 7;
				b = aabbccdd >> 5 & 1;
				c = aabbccdd >> 3 & 1;
				d = aabbccdd >> 1 & 1;
				e = eeffgghh >> 7;
				f = eeffgghh >> 5 & 1;
				g = eeffgghh >> 3 & 1;
				h = eeffgghh >> 1 & 1;
			} else {
				a = aabbccdd >> 6 & 1;
				b = aabbccdd >> 4 & 1;
				c = aabbccdd >> 2 & 1;
				d = aabbccdd & 1;
				e = eeffgghh >> 6 & 1;
				f = eeffgghh >> 4 & 1;
				g = eeffgghh >> 2 & 1;
				h = eeffgghh & 1;
			}
			*ptr++ = 0
				| a << 7 | b << 6 | c << 5 | d << 4
				| e << 3 | f << 2 | g << 1 | h;
		}
	}
	header.depth = 1;
}

static bool discardAlpha;
static bool discardColor;
static uint8_t reduceDepth = 16;

static void optimize(const char *inPath, const char *outPath) {
	if (inPath) {
		path = inPath;
		file = fopen(path, "r");
		if (!file) err(EX_NOINPUT, "%s", path);
	} else {
		path = "stdin";
		file = stdin;
	}

	sigRead();
	struct Chunk ihdr = chunkRead();
	if (strcmp(ihdr.type, "IHDR")) {
		errx(EX_DATAERR, "%s: expected IHDR, found %s", path, ihdr.type);
	}
	headerRead(ihdr);
	if (header.interlace != Progressive) {
		errx(EX_CONFIG, "%s: unsupported interlacing", path);
	}

	palClear();
	dataAlloc();
	for (;;) {
		struct Chunk chunk = chunkRead();
		if (!strcmp(chunk.type, "PLTE")) {
			palRead(chunk);
		} else if (!strcmp(chunk.type, "tRNS")) {
			transRead(chunk);
		} else if (!strcmp(chunk.type, "IDAT")) {
			dataRead(chunk);
		} else if (!strcmp(chunk.type, "IEND")) {
			break;
		} else {
			chunkSkip(chunk);
		}
	}
	fclose(file);

	dataRecon();
	if (discardAlpha || alphaUnused()) alphaDiscard();
	if (reduceDepth < 16 || depth16Unused()) depth16Reduce();
	if (discardColor || colorUnused()) colorDiscard();
	colorIndex();
	if (reduceDepth < 8 || depth8Unused()) depth8Reduce();
	if (reduceDepth < 4 || depth4Unused()) depth4Reduce();
	if (reduceDepth < 2 || depth2Unused()) depth2Reduce();
	dataFilter();

	char buf[PATH_MAX];
	if (outPath) {
		path = outPath;
		if (outPath == inPath) {
			snprintf(buf, sizeof(buf), "%so", outPath);
			file = fopen(buf, "wx");
			if (!file) err(EX_CANTCREAT, "%s", buf);
		} else {
			file = fopen(path, "w");
			if (!file) err(EX_CANTCREAT, "%s", outPath);
		}
	} else {
		path = "stdout";
		file = stdout;
	}

	sigWrite();
	headerWrite();
	if (header.color == Indexed) {
		palWrite();
		if (trans.len) transWrite();
	}
	dataWrite();
	free(data);
	int error = fclose(file);
	if (error) err(EX_IOERR, "%s", path);

	if (outPath && outPath == inPath) {
		error = rename(buf, outPath);
		if (error) err(EX_CANTCREAT, "%s", outPath);
	}
}

int main(int argc, char *argv[]) {
	bool stdio = false;
	char *outPath = NULL;

	for (int opt; 0 < (opt = getopt(argc, argv, "ab:cgo:v"));) {
		switch (opt) {
			break; case 'a': discardAlpha = true;
			break; case 'b': reduceDepth = strtoul(optarg, NULL, 10);
			break; case 'c': stdio = true;
			break; case 'g': discardColor = true;
			break; case 'o': outPath = optarg;
			break; case 'v': verbose = true;
			break; default:  return EX_USAGE;
		}
	}

	if (optind < argc) {
		for (int i = optind; i < argc; ++i) {
			optimize(argv[i], (stdio ? NULL : outPath ? outPath : argv[i]));
		}
	} else {
		optimize(NULL, outPath);
	}
}
