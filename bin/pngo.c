/* Copyright (c) 2018, June McEnroe <programble@gmail.com>
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

#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <zlib.h>

#define PACKED __attribute__((packed))
#define PAIR(a, b) ((uint16_t)(a) << 8 | (uint16_t)(b))

#define CRC_INIT (crc32(0, Z_NULL, 0))

static bool verbose;
static const char *path;
static FILE *file;
static uint32_t crc;

static void readExpect(void *ptr, size_t size, const char *expect) {
	fread(ptr, size, 1, file);
	if (ferror(file)) err(EX_IOERR, "%s", path);
	if (feof(file)) errx(EX_DATAERR, "%s: missing %s", path, expect);
	crc = crc32(crc, ptr, size);
}

static void writeExpect(const void *ptr, size_t size) {
	fwrite(ptr, size, 1, file);
	if (ferror(file)) err(EX_IOERR, "%s", path);
	crc = crc32(crc, ptr, size);
}

static const uint8_t Signature[8] = "\x89PNG\r\n\x1A\n";

static void readSignature(void) {
	uint8_t signature[8];
	readExpect(signature, 8, "signature");
	if (0 != memcmp(signature, Signature, 8)) {
		errx(EX_DATAERR, "%s: invalid signature", path);
	}
}

static void writeSignature(void) {
	writeExpect(Signature, sizeof(Signature));
}

struct PACKED Chunk {
	uint32_t size;
	char type[4];
};

static const char *typeStr(struct Chunk chunk) {
	static char buf[5];
	memcpy(buf, chunk.type, 4);
	return buf;
}

static struct Chunk readChunk(void) {
	struct Chunk chunk;
	readExpect(&chunk, sizeof(chunk), "chunk");
	chunk.size = ntohl(chunk.size);
	crc = crc32(CRC_INIT, (Byte *)chunk.type, sizeof(chunk.type));
	return chunk;
}

static void writeChunk(struct Chunk chunk) {
	chunk.size = htonl(chunk.size);
	writeExpect(&chunk, sizeof(chunk));
	crc = crc32(CRC_INIT, (Byte *)chunk.type, sizeof(chunk.type));
}

static void readCrc(void) {
	uint32_t expected = crc;
	uint32_t found;
	readExpect(&found, sizeof(found), "CRC32");
	found = ntohl(found);
	if (found != expected) {
		errx(
			EX_DATAERR, "%s: expected CRC32 %08X, found %08X",
			path, expected, found
		);
	}
}

static void writeCrc(void) {
	uint32_t net = htonl(crc);
	writeExpect(&net, sizeof(net));
}

static void skipChunk(struct Chunk chunk) {
	if (!(chunk.type[0] & 0x20)) {
		errx(EX_CONFIG, "%s: unsupported critical chunk %s", path, typeStr(chunk));
	}
	uint8_t discard[chunk.size];
	readExpect(discard, sizeof(discard), "chunk data");
	readCrc();
}

static struct PACKED {
	uint32_t width;
	uint32_t height;
	uint8_t depth;
	enum PACKED {
		Grayscale      = 0,
		Truecolor      = 2,
		Indexed        = 3,
		GrayscaleAlpha = 4,
		TruecolorAlpha = 6,
	} color;
	enum PACKED { Deflate } compression;
	enum PACKED { Adaptive } filter;
	enum PACKED { Progressive, Adam7 } interlace;
} header;
static_assert(13 == sizeof(header), "header size");

static size_t pixelBits(void) {
	switch (header.color) {
		case Grayscale:      return 1 * header.depth;
		case Truecolor:      return 3 * header.depth;
		case Indexed:        return 1 * header.depth;
		case GrayscaleAlpha: return 2 * header.depth;
		case TruecolorAlpha: return 4 * header.depth;
		default: abort();
	}
}

static size_t pixelSize(void) {
	return (pixelBits() + 7) / 8;
}

static size_t lineSize(void) {
	return (header.width * pixelBits() + 7) / 8;
}

static size_t dataSize(void) {
	return (1 + lineSize()) * header.height;
}

static const char *ColorStr[] = {
	[Grayscale] = "grayscale",
	[Truecolor] = "truecolor",
	[Indexed] = "indexed",
	[GrayscaleAlpha] = "grayscale alpha",
	[TruecolorAlpha] = "truecolor alpha",
};
static void printHeader(void) {
	fprintf(
		stderr,
		"%s: %ux%u %hhu-bit %s\n",
		path,
		header.width, header.height,
		header.depth, ColorStr[header.color]
	);
}

static void readHeader(void) {
	struct Chunk ihdr = readChunk();
	if (0 != memcmp(ihdr.type, "IHDR", 4)) {
		errx(EX_DATAERR, "%s: expected IHDR, found %s", path, typeStr(ihdr));
	}
	if (ihdr.size != sizeof(header)) {
		errx(
			EX_DATAERR, "%s: expected IHDR size %zu, found %u",
			path, sizeof(header), ihdr.size
		);
	}
	readExpect(&header, sizeof(header), "header");
	readCrc();

	header.width = ntohl(header.width);
	header.height = ntohl(header.height);

	if (!header.width) errx(EX_DATAERR, "%s: invalid width 0", path);
	if (!header.height) errx(EX_DATAERR, "%s: invalid height 0", path);
	switch (PAIR(header.color, header.depth)) {
		case PAIR(Grayscale, 1):
		case PAIR(Grayscale, 2):
		case PAIR(Grayscale, 4):
		case PAIR(Grayscale, 8):
		case PAIR(Grayscale, 16):
		case PAIR(Truecolor, 8):
		case PAIR(Truecolor, 16):
		case PAIR(Indexed, 1):
		case PAIR(Indexed, 2):
		case PAIR(Indexed, 4):
		case PAIR(Indexed, 8):
		case PAIR(GrayscaleAlpha, 8):
		case PAIR(GrayscaleAlpha, 16):
		case PAIR(TruecolorAlpha, 8):
		case PAIR(TruecolorAlpha, 16):
			break;
		default:
			errx(
				EX_DATAERR, "%s: invalid color type %hhu and bit depth %hhu",
				path, header.color, header.depth
			);
	}
	if (header.compression != Deflate) {
		errx(
			EX_DATAERR, "%s: invalid compression method %hhu",
			path, header.compression
		);
	}
	if (header.filter != Adaptive) {
		errx(EX_DATAERR, "%s: invalid filter method %hhu", path, header.filter);
	}
	if (header.interlace > Adam7) {
		errx(EX_DATAERR, "%s: invalid interlace method %hhu", path, header.interlace);
	}

	if (verbose) printHeader();
}

static void writeHeader(void) {
	if (verbose) printHeader();

	struct Chunk ihdr = { .size = sizeof(header), .type = "IHDR" };
	writeChunk(ihdr);
	header.width = htonl(header.width);
	header.height = htonl(header.height);
	writeExpect(&header, sizeof(header));
	writeCrc();

	header.width = ntohl(header.width);
	header.height = ntohl(header.height);
}

static struct {
	uint32_t len;
	uint8_t entries[256][3];
} palette;

static uint16_t paletteIndex(const uint8_t *rgb) {
	uint16_t i;
	for (i = 0; i < palette.len; ++i) {
		if (0 == memcmp(palette.entries[i], rgb, 3)) break;
	}
	return i;
}

static bool paletteAdd(const uint8_t *rgb) {
	uint16_t i = paletteIndex(rgb);
	if (i < palette.len) return true;
	if (i == 256) return false;
	memcpy(palette.entries[palette.len++], rgb, 3);
	return true;
}

static void readPalette(void) {
	struct Chunk chunk;
	for (;;) {
		chunk = readChunk();
		if (0 == memcmp(chunk.type, "PLTE", 4)) break;
		skipChunk(chunk);
	}
	if (chunk.size % 3) {
		errx(EX_DATAERR, "%s: PLTE size %u not divisible by 3", path, chunk.size);
	}

	palette.len = chunk.size / 3;
	readExpect(palette.entries, chunk.size, "palette data");
	readCrc();

	if (verbose) fprintf(stderr, "%s: palette length %u\n", path, palette.len);
}

static void writePalette(void) {
	if (verbose) fprintf(stderr, "%s: palette length %u\n", path, palette.len);
	struct Chunk plte = { .size = 3 * palette.len, .type = "PLTE" };
	writeChunk(plte);
	writeExpect(palette.entries, plte.size);
	writeCrc();
}

static uint8_t *data;

static void allocData(void) {
	data = malloc(dataSize());
	if (!data) err(EX_OSERR, "malloc(%zu)", dataSize());
}

static void readData(void) {
	if (verbose) fprintf(stderr, "%s: data size %zu\n", path, dataSize());

	struct z_stream_s stream = { .next_out = data, .avail_out = dataSize() };
	int error = inflateInit(&stream);
	if (error != Z_OK) errx(EX_SOFTWARE, "%s: inflateInit: %s", path, stream.msg);

	for (;;) {
		struct Chunk chunk = readChunk();
		if (0 == memcmp(chunk.type, "IDAT", 4)) {
			uint8_t *idat = malloc(chunk.size);
			if (!idat) err(EX_OSERR, "malloc");

			readExpect(idat, chunk.size, "image data");
			readCrc();

			stream.next_in = idat;
			stream.avail_in = chunk.size;
			int error = inflate(&stream, Z_SYNC_FLUSH);
			free(idat);

			if (error == Z_STREAM_END) break;
			if (error != Z_OK) errx(EX_DATAERR, "%s: inflate: %s", path, stream.msg);

		} else if (0 == memcmp(chunk.type, "IEND", 4)) {
			errx(EX_DATAERR, "%s: missing IDAT chunk", path);
		} else {
			skipChunk(chunk);
		}
	}

	inflateEnd(&stream);
	if (stream.total_out != dataSize()) {
		errx(
			EX_DATAERR, "%s: expected data size %zu, found %lu",
			path, dataSize(), stream.total_out
		);
	}

	if (verbose) fprintf(stderr, "%s: deflate size %lu\n", path, stream.total_in);
}

static void writeData(void) {
	if (verbose) fprintf(stderr, "%s: data size %zu\n", path, dataSize());

	uLong size = compressBound(dataSize());
	uint8_t *deflate = malloc(size);
	if (!deflate) err(EX_OSERR, "malloc");

	int error = compress2(deflate, &size, data, dataSize(), Z_BEST_COMPRESSION);
	if (error != Z_OK) errx(EX_SOFTWARE, "%s: compress2: %d", path, error);

	struct Chunk idat = { .size = size, .type = "IDAT" };
	writeChunk(idat);
	writeExpect(deflate, size);
	writeCrc();

	free(deflate);

	if (verbose) fprintf(stderr, "%s: deflate size %lu\n", path, size);
}

static void writeEnd(void) {
	struct Chunk iend = { .size = 0, .type = "IEND" };
	writeChunk(iend);
	writeCrc();
}

enum PACKED Filter {
	None,
	Sub,
	Up,
	Average,
	Paeth,
	FilterCount,
};

struct Bytes {
	uint8_t x;
	uint8_t a;
	uint8_t b;
	uint8_t c;
};

static uint8_t paethPredictor(struct Bytes f) {
	int32_t p = (int32_t)f.a + (int32_t)f.b - (int32_t)f.c;
	int32_t pa = abs(p - (int32_t)f.a);
	int32_t pb = abs(p - (int32_t)f.b);
	int32_t pc = abs(p - (int32_t)f.c);
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
		default:      abort();
	}
}

static uint8_t filt(enum Filter type, struct Bytes f) {
	switch (type) {
		case None:    return f.x;
		case Sub:     return f.x - f.a;
		case Up:      return f.x - f.b;
		case Average: return f.x - ((uint32_t)f.a + (uint32_t)f.b) / 2;
		case Paeth:   return f.x - paethPredictor(f);
		default:      abort();
	}
}

static struct Line {
	enum Filter type;
	uint8_t data[];
} **lines;

static void allocLines(void) {
	lines = calloc(header.height, sizeof(*lines));
	if (!lines) err(EX_OSERR, "calloc(%u, %zu)", header.height, sizeof(*lines));
}

static void scanlines(void) {
	size_t stride = 1 + lineSize();
	for (uint32_t y = 0; y < header.height; ++y) {
		lines[y] = (struct Line *)&data[y * stride];
		if (lines[y]->type >= FilterCount) {
			errx(EX_DATAERR, "%s: invalid filter type %hhu", path, lines[y]->type);
		}
	}
}

static struct Bytes origBytes(uint32_t y, size_t i) {
	bool a = (i >= pixelSize()), b = (y > 0), c = (a && b);
	return (struct Bytes) {
		.x = lines[y]->data[i],
		.a = a ? lines[y]->data[i - pixelSize()] : 0,
		.b = b ? lines[y - 1]->data[i] : 0,
		.c = c ? lines[y - 1]->data[i - pixelSize()] : 0,
	};
}

static void reconData(void) {
	for (uint32_t y = 0; y < header.height; ++y) {
		for (size_t i = 0; i < lineSize(); ++i) {
			lines[y]->data[i] =
				recon(lines[y]->type, origBytes(y, i));
		}
		lines[y]->type = None;
	}
}

static void filterData(void) {
	if (header.color == Indexed || header.depth < 8) return;
	for (uint32_t y = header.height - 1; y < header.height; --y) {
		uint8_t filter[FilterCount][lineSize()];
		uint32_t heuristic[FilterCount] = {0};
		enum Filter minType = None;
		for (enum Filter type = None; type < FilterCount; ++type) {
			for (size_t i = 0; i < lineSize(); ++i) {
				filter[type][i] = filt(type, origBytes(y, i));
				heuristic[type] += abs((int8_t)filter[type][i]);
			}
			if (heuristic[type] < heuristic[minType]) minType = type;
		}
		lines[y]->type = minType;
		memcpy(lines[y]->data, filter[minType], lineSize());
	}
}

static void discardAlpha(void) {
	if (header.color != GrayscaleAlpha && header.color != TruecolorAlpha) return;
	size_t sampleSize = header.depth / 8;
	size_t colorSize = pixelSize() - sampleSize;
	for (uint32_t y = 0; y < header.height; ++y) {
		for (uint32_t x = 0; x < header.width; ++x) {
			for (size_t i = 0; i < sampleSize; ++i) {
				if (lines[y]->data[x * pixelSize() + colorSize + i] != 0xFF) return;
			}
		}
	}

	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = lines[y]->type;
		for (uint32_t x = 0; x < header.width; ++x) {
			memmove(ptr, &lines[y]->data[x * pixelSize()], colorSize);
			ptr += colorSize;
		}
	}
	header.color = (header.color == GrayscaleAlpha) ? Grayscale : Truecolor;
	scanlines();
}

static void discardColor(void) {
	if (header.color != Truecolor && header.color != TruecolorAlpha) return;
	size_t sampleSize = header.depth / 8;
	for (uint32_t y = 0; y < header.height; ++y) {
		for (uint32_t x = 0; x < header.width; ++x) {
			uint8_t *r = &lines[y]->data[x * pixelSize()];
			uint8_t *g = r + sampleSize;
			uint8_t *b = g + sampleSize;
			if (0 != memcmp(r, g, sampleSize)) return;
			if (0 != memcmp(g, b, sampleSize)) return;
		}
	}

	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = lines[y]->type;
		for (uint32_t x = 0; x < header.width; ++x) {
			uint8_t *pixel = &lines[y]->data[x * pixelSize()];
			memmove(ptr, pixel, sampleSize);
			ptr += sampleSize;
			if (header.color == TruecolorAlpha) {
				memmove(ptr, pixel + 3 * sampleSize, sampleSize);
				ptr += sampleSize;
			}
		}
	}
	header.color = (header.color == Truecolor) ? Grayscale : GrayscaleAlpha;
	scanlines();
}

static void indexColor(void) {
	if (header.color != Truecolor || header.depth != 8) return;
	for (uint32_t y = 0; y < header.height; ++y) {
		for (uint32_t x = 0; x < header.width; ++x) {
			if (!paletteAdd(&lines[y]->data[x * 3])) return;
		}
	}

	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = lines[y]->type;
		for (uint32_t x = 0; x < header.width; ++x) {
			*ptr++ = paletteIndex(&lines[y]->data[x * 3]);
		}
	}
	header.color = Indexed;
	scanlines();
}

static void reduceDepth8(void) {
	if (header.color != Grayscale && header.color != Indexed) return;
	if (header.depth != 8) return;
	if (header.color == Grayscale) {
		for (uint32_t y = 0; y < header.height; ++y) {
			for (size_t i = 0; i < lineSize(); ++i) {
				uint8_t a = lines[y]->data[i];
				if ((a >> 4) != (a & 0x0F)) return;
			}
		}
	} else if (palette.len > 16) {
		return;
	}

	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = lines[y]->type;
		for (size_t i = 0; i < lineSize(); i += 2) {
			uint8_t iByte = lines[y]->data[i];
			uint8_t jByte = (i + 1 < lineSize()) ? lines[y]->data[i + 1] : 0;
			uint8_t a = iByte & 0x0F;
			uint8_t b = jByte & 0x0F;
			*ptr++ = a << 4 | b;
		}
	}
	header.depth = 4;
	scanlines();
}

static void reduceDepth4(void) {
	if (header.depth != 4) return;
	if (header.color == Grayscale) {
		for (uint32_t y = 0; y < header.height; ++y) {
			for (size_t i = 0; i < lineSize(); ++i) {
				uint8_t a = lines[y]->data[i] >> 4;
				uint8_t b = lines[y]->data[i] & 0x0F;
				if ((a >> 2) != (a & 0x03)) return;
				if ((b >> 2) != (b & 0x03)) return;
			}
		}
	} else if (palette.len > 4) {
		return;
	}

	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = lines[y]->type;
		for (size_t i = 0; i < lineSize(); i += 2) {
			uint8_t iByte = lines[y]->data[i];
			uint8_t jByte = (i + 1 < lineSize()) ? lines[y]->data[i + 1] : 0;
			uint8_t a = iByte >> 4 & 0x03, b = iByte & 0x03;
			uint8_t c = jByte >> 4 & 0x03, d = jByte & 0x03;
			*ptr++ = a << 6 | b << 4 | c << 2 | d;
		}
	}
	header.depth = 2;
	scanlines();
}

static void reduceDepth2(void) {
	if (header.depth != 2) return;
	if (header.color == Grayscale) {
		for (uint32_t y = 0; y < header.height; ++y) {
			for (size_t i = 0; i < lineSize(); ++i) {
				uint8_t a = lines[y]->data[i] >> 6;
				uint8_t b = lines[y]->data[i] >> 4 & 0x03;
				uint8_t c = lines[y]->data[i] >> 2 & 0x03;
				uint8_t d = lines[y]->data[i] & 0x03;
				if ((a >> 1) != (a & 0x01)) return;
				if ((b >> 1) != (b & 0x01)) return;
				if ((c >> 1) != (c & 0x01)) return;
				if ((d >> 1) != (d & 0x01)) return;
			}
		}
	} else if (palette.len > 2) {
		return;
	}

	uint8_t *ptr = data;
	for (uint32_t y = 0; y < header.height; ++y) {
		*ptr++ = lines[y]->type;
		for (size_t i = 0; i < lineSize(); i += 2) {
			uint8_t iByte = lines[y]->data[i];
			uint8_t jByte = (i + 1 < lineSize()) ? lines[y]->data[i + 1] : 0;
			uint8_t a = iByte >> 6 & 0x01, b = iByte >> 4 & 0x01;
			uint8_t c = iByte >> 2 & 0x01, d = iByte & 0x01;
			uint8_t e = jByte >> 6 & 0x01, f = jByte >> 4 & 0x01;
			uint8_t g = jByte >> 2 & 0x01, h = jByte & 0x01;
			*ptr++ = a << 7 | b << 6 | c << 5 | d << 4 | e << 3 | f << 2 | g << 1 | h;
		}
	}
	header.depth = 1;
	scanlines();
}

static void reduceDepth(void) {
	reduceDepth8();
	reduceDepth4();
	reduceDepth2();
}

static void optimize(const char *inPath, const char *outPath) {
	if (inPath) {
		path = inPath;
		file = fopen(path, "r");
		if (!file) err(EX_NOINPUT, "%s", path);
	} else {
		path = "(stdin)";
		file = stdin;
	}

	readSignature();
	readHeader();
	if (header.interlace != Progressive) {
		errx(
			EX_CONFIG, "%s: unsupported interlace method %hhu",
			path, header.interlace
		);
	}
	if (header.color == Indexed) readPalette();
	allocData();
	readData();
	fclose(file);

	allocLines();
	scanlines();
	reconData();

	discardAlpha();
	discardColor();
	indexColor();
	reduceDepth();
	filterData();
	free(lines);

	if (outPath) {
		path = outPath;
		file = fopen(path, "w");
		if (!file) err(EX_CANTCREAT, "%s", path);
	} else {
		path = "(stdout)";
		file = stdout;
	}

	writeSignature();
	writeHeader();
	if (header.color == Indexed) writePalette();
	writeData();
	writeEnd();
	free(data);

	int error = fclose(file);
	if (error) err(EX_IOERR, "%s", path);
}

int main(int argc, char *argv[]) {
	bool stdio = false;
	char *output = NULL;

	int opt;
	while (0 < (opt = getopt(argc, argv, "co:v"))) {
		switch (opt) {
			break; case 'c': stdio = true;
			break; case 'o': output = optarg;
			break; case 'v': verbose = true;
			break; default: return EX_USAGE;
		}
	}

	if (argc - optind == 1 && (output || stdio)) {
		optimize(argv[optind], output);
	} else if (optind < argc) {
		for (int i = optind; i < argc; ++i) {
			optimize(argv[i], argv[i]);
		}
	} else {
		optimize(NULL, output);
	}

	return EX_OK;
}
