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

#define CRC_INIT (crc32(0, Z_NULL, 0))

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
	uint8_t compression;
	uint8_t filter;
	uint8_t interlace;
} header;
static_assert(13 == sizeof(header), "header size");

static size_t lineSize(void) {
	switch (header.color) {
		case Grayscale:      return (header.width * 1 * header.depth + 7) / 8;
		case Truecolor:      return (header.width * 3 * header.depth + 7) / 8;
		case Indexed:        return (header.width * 1 * header.depth + 7) / 8;
		case GrayscaleAlpha: return (header.width * 2 * header.depth + 7) / 8;
		case TruecolorAlpha: return (header.width * 4 * header.depth + 7) / 8;
		default: abort();
	}
}

static size_t dataSize(void) {
	return (1 + lineSize()) * header.height;
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
}

static void writeHeader(void) {
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

static void readPalette(void) {
	struct Chunk chunk;
	for (;;) {
		chunk = readChunk();
		if (0 == memcmp(chunk.type, "PLTE", 4)) break;
		skipChunk(chunk);
	}
	palette.len = chunk.size / 3;
	readExpect(palette.entries, chunk.size, "palette data");
	readCrc();
}

static void writePalette(void) {
	struct Chunk plte = { .size = 3 * palette.len, .type = "PLTE" };
	writeChunk(plte);
	writeExpect(palette.entries, plte.size);
	writeCrc();
}

static uint8_t *data;

static void readData(void) {
	data = malloc(dataSize());
	if (!data) err(EX_OSERR, "malloc(%zu)", dataSize());

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
}

static void writeData(void) {
	uLong size = compressBound(dataSize());
	uint8_t *deflate = malloc(size);
	if (!deflate) err(EX_OSERR, "malloc");

	int error = compress2(deflate, &size, data, dataSize(), Z_BEST_SPEED);
	if (error != Z_OK) errx(EX_SOFTWARE, "%s: compress2: %d", path, error);

	struct Chunk idat = { .size = size, .type = "IDAT" };
	writeChunk(idat);
	writeExpect(deflate, size);
	writeCrc();

	free(deflate);
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

static struct {
	bool brokenPaeth;
	bool filt;
	bool recon;
	uint8_t declareFilter;
	uint8_t applyFilter;
	enum Filter declareFilters[255];
	enum Filter applyFilters[255];
	bool invert;
	bool mirror;
} options;

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
	if (options.brokenPaeth) {
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

static void scanlines(void) {
	lines = calloc(header.height, sizeof(*lines));
	if (!lines) err(EX_OSERR, "calloc(%u, %zu)", header.height, sizeof(*lines));

	size_t stride = 1 + lineSize();
	for (uint32_t y = 0; y < header.height; ++y) {
		lines[y] = (struct Line *)&data[y * stride];
		if (lines[y]->type >= FilterCount) {
			errx(EX_DATAERR, "%s: invalid filter type %hhu", path, lines[y]->type);
		}
	}
}

static struct Bytes origBytes(uint32_t y, size_t i) {
	size_t pixelSize = lineSize() / header.width;
	if (!pixelSize) pixelSize = 1;
	bool a = (i >= pixelSize), b = (y > 0), c = (a && b);
	return (struct Bytes) {
		.x = lines[y]->data[i],
		.a = a ? lines[y]->data[i - pixelSize] : 0,
		.b = b ? lines[y - 1]->data[i] : 0,
		.c = c ? lines[y - 1]->data[i - pixelSize] : 0,
	};
}

static void reconData(void) {
	for (uint32_t y = 0; y < header.height; ++y) {
		for (size_t i = 0; i < lineSize(); ++i) {
			if (options.filt) {
				lines[y]->data[i] = filt(lines[y]->type, origBytes(y, i));
			} else {
				lines[y]->data[i] = recon(lines[y]->type, origBytes(y, i));
			}
		}
		lines[y]->type = None;
	}
}

static void filterData(void) {
	for (uint32_t y = header.height - 1; y < header.height; --y) {
		uint8_t filter[FilterCount][lineSize()];
		uint32_t heuristic[FilterCount] = {0};
		enum Filter minType = None;
		for (enum Filter type = None; type < FilterCount; ++type) {
			for (size_t i = 0; i < lineSize(); ++i) {
				if (options.recon) {
					filter[type][i] = recon(type, origBytes(y, i));
				} else {
					filter[type][i] = filt(type, origBytes(y, i));
				}
				heuristic[type] += abs((int8_t)filter[type][i]);
			}
			if (heuristic[type] < heuristic[minType]) minType = type;
		}

		if (options.declareFilter) {
			lines[y]->type = options.declareFilters[y % options.declareFilter];
		} else {
			lines[y]->type = minType;
		}

		if (options.applyFilter) {
			enum Filter type = options.applyFilters[y % options.applyFilter];
			memcpy(lines[y]->data, filter[type], lineSize());
		} else {
			memcpy(lines[y]->data, filter[minType], lineSize());
		}
	}
}

static void invert(void) {
	for (uint32_t y = 0; y < header.height; ++y) {
		for (size_t i = 0; i < lineSize(); ++i) {
			lines[y]->data[i] ^= 0xFF;
		}
	}
}

static void mirror(void) {
	for (uint32_t y = 0; y < header.height; ++y) {
		for (size_t i = 0, j = lineSize() - 1; i < j; ++i, --j) {
			uint8_t t = lines[y]->data[i];
			lines[y]->data[i] = lines[y]->data[j];
			lines[y]->data[j] = t;
		}
	}
}

static void glitch(const char *inPath, const char *outPath) {
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
	if (header.color == Indexed) readPalette();
	readData();
	fclose(file);

	scanlines();
	reconData();
	filterData();
	if (options.invert) invert();
	if (options.mirror) mirror();
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

static enum Filter parseFilter(const char *s) {
	switch (s[0]) {
		case 'N': case 'n': return None;
		case 'S': case 's': return Sub;
		case 'U': case 'u': return Up;
		case 'A': case 'a': return Average;
		case 'P': case 'p': return Paeth;
		default: errx(EX_USAGE, "invalid filter type %s", s);
	}
}

static uint8_t parseFilters(enum Filter *filters, const char *s) {
	uint8_t len = 0;
	do {
		filters[len++] = parseFilter(s);
		s = strchr(s, ',');
	} while (s++);
	return len;
}

int main(int argc, char *argv[]) {
	bool stdio = false;
	char *output = NULL;

	int opt;
	while (0 < (opt = getopt(argc, argv, "a:cd:fimo:pr"))) {
		switch (opt) {
			break; case 'a':
				options.applyFilter = parseFilters(options.applyFilters, optarg);
			break; case 'c': stdio = true;
			break; case 'd':
				options.declareFilter = parseFilters(options.declareFilters, optarg);
			break; case 'f': options.filt = true;
			break; case 'i': options.invert = true;
			break; case 'm': options.mirror = true;
			break; case 'o': output = optarg;
			break; case 'p': options.brokenPaeth = true;
			break; case 'r': options.recon = true;
			break; default: return EX_USAGE;
		}
	}

	if (argc - optind == 1 && (output || stdio)) {
		glitch(argv[optind], output);
	} else if (optind < argc) {
		for (int i = optind; i < argc; ++i) {
			glitch(argv[i], argv[i]);
		}
	} else {
		glitch(NULL, output);
	}

	return EX_OK;
}
