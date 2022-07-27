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

static size_t pixelLen;
static size_t lineLen;
static size_t dataLen;

static void recalc(void) {
	size_t pixelBits = header.depth;
	switch (header.color) {
		break; case GrayscaleAlpha: pixelBits *= 2;
		break; case Truecolor: pixelBits *= 3;
		break; case TruecolorAlpha: pixelBits *= 4;
	}
	pixelLen = (pixelBits + 7) / 8;
	lineLen = (header.width * pixelBits + 7) / 8;
	dataLen = (1 + lineLen) * header.height;
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
	recalc();
}

static void headerWrite(void) {
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
}

static void palWrite(void) {
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
}

static void transWrite(void) {
	struct Chunk trns = { trans.len, "tRNS" };
	chunkWrite(trns);
	pngWrite(trans.a, trns.len);
	crcWrite();
}

static uint8_t *data;

static void dataAlloc(void) {
	data = malloc(dataLen);
	if (!data) err(EX_OSERR, "malloc");
}

static void dataRead(struct Chunk chunk) {
	z_stream stream = { .next_out = data, .avail_out = dataLen };
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
	if ((size_t)stream.total_out != dataLen) {
		errx(
			EX_DATAERR, "%s: expected data length %zu, found %zu",
			path, dataLen, (size_t)stream.total_out
		);
	}
}

static void dataWrite(void) {
	z_stream stream = {
		.next_in = data,
		.avail_in = dataLen,
	};
	int error = deflateInit2(
		&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15, 8, Z_FILTERED
	);
	if (error != Z_OK) errx(EX_SOFTWARE, "deflateInit2: %s", stream.msg);

	uLong bound = deflateBound(&stream, dataLen);
	uint8_t *buf = malloc(bound);
	if (!buf) err(EX_OSERR, "malloc");

	stream.next_out = buf;
	stream.avail_out = bound;
	deflate(&stream, Z_FINISH);
	deflateEnd(&stream);

	struct Chunk idat = { stream.total_out, "IDAT" };
	chunkWrite(idat);
	pngWrite(buf, stream.total_out);
	crcWrite();
	free(buf);

	struct Chunk iend = { 0, "IEND" };
	chunkWrite(iend);
	crcWrite();
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

static bool brokenPaeth;
static uint8_t paethPredictor(struct Bytes f) {
	int32_t p = (int32_t)f.a + (int32_t)f.b - (int32_t)f.c;
	int32_t pa = labs(p - (int32_t)f.a);
	int32_t pb = labs(p - (int32_t)f.b);
	int32_t pc = labs(p - (int32_t)f.c);
	if (pa <= pb && pa <= pc) return f.a;
	if (brokenPaeth) {
		if (pb < pc) return f.b;
	} else {
		if (pb <= pc) return f.b;
	}
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
	return &data[y * (1 + lineLen)];
}
static uint8_t *lineData(uint32_t y) {
	return 1 + lineType(y);
}

static struct Bytes origBytes(uint32_t y, size_t i) {
	bool a = (i >= pixelLen), b = (y > 0), c = (a && b);
	return (struct Bytes) {
		.x = lineData(y)[i],
		.a = (a ? lineData(y)[i-pixelLen] : 0),
		.b = (b ? lineData(y-1)[i] : 0),
		.c = (c ? lineData(y-1)[i-pixelLen] : 0),
	};
}

static bool reconFilter;
static void dataRecon(void) {
	for (uint32_t y = 0; y < header.height; ++y) {
		for (size_t i = 0; i < lineLen; ++i) {
			if (reconFilter) {
				lineData(y)[i] = filt(*lineType(y), origBytes(y, i));
			} else {
				lineData(y)[i] = recon(*lineType(y), origBytes(y, i));
			}
		}
		*lineType(y) = None;
	}
}

static bool filterRecon;
static size_t applyFilter;
static enum Filter applyFilters[256];
static size_t declFilter;
static enum Filter declFilters[256];

static void dataFilter(void) {
	uint8_t *filter[FilterCap];
	for (enum Filter i = None; i < FilterCap; ++i) {
		filter[i] = malloc(lineLen);
		if (!filter[i]) err(EX_OSERR, "malloc");
	}
	for (uint32_t y = header.height-1; y < header.height; --y) {
		uint32_t heuristic[FilterCap] = {0};
		enum Filter minType = None;
		for (enum Filter type = None; type < FilterCap; ++type) {
			for (size_t i = 0; i < lineLen; ++i) {
				if (filterRecon) {
					filter[type][i] = recon(type, origBytes(y, i));
				} else {
					filter[type][i] = filt(type, origBytes(y, i));
				}
				heuristic[type] += abs((int8_t)filter[type][i]);
			}
			if (heuristic[type] < heuristic[minType]) minType = type;
		}
		if (declFilter) {
			*lineType(y) = declFilters[y % declFilter];
		} else {
			*lineType(y) = minType;
		}
		if (applyFilter) {
			memcpy(lineData(y), filter[applyFilters[y % applyFilter]], lineLen);
		} else {
			memcpy(lineData(y), filter[minType], lineLen);
		}
	}
	for (enum Filter i = None; i < FilterCap; ++i) {
		free(filter[i]);
	}
}

static bool invertData;
static bool mirrorData;
static bool zeroX;
static bool zeroY;

static void glitch(const char *inPath, const char *outPath) {
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
	dataFilter();

	if (invertData) {
		for (uint32_t y = 0; y < header.height; ++y) {
			for (size_t i = 0; i < lineLen; ++i) {
				lineData(y)[i] ^= 0xFF;
			}
		}
	}
	if (mirrorData) {
		for (uint32_t y = 0; y < header.height; ++y) {
			for (size_t i = 0, j = lineLen-1; i < j; ++i, --j) {
				uint8_t x = lineData(y)[i];
				lineData(y)[i] = lineData(y)[j];
				lineData(y)[j] = x;
			}
		}
	}
	if (zeroX) {
		for (uint32_t y = 0; y < header.height; ++y) {
			memset(lineData(y), 0, pixelLen);
		}
	}
	if (zeroY) {
		memset(lineData(0), 0, lineLen);
	}

	char buf[PATH_MAX];
	if (outPath) {
		path = outPath;
		if (outPath == inPath) {
			snprintf(buf, sizeof(buf), "%sg", outPath);
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

static enum Filter parseFilter(const char *str) {
	switch (str[0]) {
		case 'N': case 'n': return None;
		case 'S': case 's': return Sub;
		case 'U': case 'u': return Up;
		case 'A': case 'a': return Average;
		case 'P': case 'p': return Paeth;
		default: errx(EX_USAGE, "invalid filter type %s", str);
	}
}

static size_t parseFilters(enum Filter *filters, char *str) {
	size_t len = 0;
	while (str) {
		char *filt = strsep(&str, ",");
		filters[len++] = parseFilter(filt);
	}
	return len;
}

int main(int argc, char *argv[]) {
	bool stdio = false;
	char *outPath = NULL;

	for (int opt; 0 < (opt = getopt(argc, argv, "a:cd:fimo:prxy"));) {
		switch (opt) {
			break; case 'a': applyFilter = parseFilters(applyFilters, optarg);
			break; case 'c': stdio = true;
			break; case 'd': declFilter = parseFilters(declFilters, optarg);
			break; case 'f': reconFilter = true;
			break; case 'i': invertData = true;
			break; case 'm': mirrorData = true;
			break; case 'o': outPath = optarg;
			break; case 'p': brokenPaeth = true;
			break; case 'r': filterRecon = true;
			break; case 'x': zeroX = true;
			break; case 'y': zeroY = true;
			break; default:  return EX_USAGE;
		}
	}

	if (optind < argc) {
		for (int i = optind; i < argc; ++i) {
			glitch(argv[i], (stdio ? NULL : outPath ? outPath : argv[i]));
		}
	} else {
		glitch(NULL, outPath);
	}
}
