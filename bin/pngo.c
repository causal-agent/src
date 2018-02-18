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
#include <zlib.h>

#define PACKED __attribute__((packed))
#define BYTE_PAIR(a, b) ((uint16_t)(a) << 8 | (uint16_t)(b))

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

static const uint8_t SIGNATURE[8] = {
    0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'
};

static void readSignature(void) {
    uint8_t signature[8];
    readExpect(signature, 8, "signature");
    if (0 != memcmp(signature, SIGNATURE, 8)) {
        errx(EX_DATAERR, "%s: invalid signature", path);
    }
}

static void writeSignature(void) {
    writeExpect(SIGNATURE, sizeof(SIGNATURE));
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
    crc = crc32(CRC_INIT, (Bytef *)chunk.type, sizeof(chunk.type));
    return chunk;
}

static void writeChunk(struct Chunk chunk) {
    chunk.size = htonl(chunk.size);
    writeExpect(&chunk, sizeof(chunk));
    crc = crc32(CRC_INIT, (Bytef *)chunk.type, sizeof(chunk.type));
}

static void readCrc(void) {
    uint32_t expected = crc;
    uint32_t found;
    readExpect(&found, sizeof(found), "CRC32");
    found = ntohl(found);
    if (found != expected) {
        errx(
            EX_DATAERR, "%s: expected CRC32 %08x, found %08x",
            path, expected, found
        );
    }
}

static void writeCrc(void) {
    uint32_t net = htonl(crc);
    writeExpect(&net, sizeof(net));
}

enum PACKED Color {
    GRAYSCALE       = 0,
    TRUECOLOR       = 2,
    INDEXED         = 3,
    GRAYSCALE_ALPHA = 4,
    TRUECOLOR_ALPHA = 6
};
#define ALPHA (0x04)

static struct PACKED {
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    enum Color color;
    enum PACKED { DEFLATE } compression;
    enum PACKED { ADAPTIVE } filter;
    enum PACKED { PROGRESSIVE, ADAM7 } interlace;
} header;

static size_t lineSize(void) {
    switch (header.color) {
        case GRAYSCALE:       return (header.width * 1 * header.depth + 7) / 8;
        case TRUECOLOR:       return (header.width * 3 * header.depth + 7) / 8;
        case INDEXED:         return (header.width * 1 * header.depth + 7) / 8;
        case GRAYSCALE_ALPHA: return (header.width * 2 * header.depth + 7) / 8;
        case TRUECOLOR_ALPHA: return (header.width * 4 * header.depth + 7) / 8;
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
    switch (BYTE_PAIR(header.color, header.depth)) {
        case 0x0001: case 0x0002: case 0x0004: case 0x0008: case 0x0010: break;
        case 0x0208: case 0x0210:                                        break;
        case 0x0301: case 0x0302: case 0x0304: case 0x0308:              break;
        case 0x0408: case 0x0410:                                        break;
        case 0x0608: case 0x0610:                                        break;
        default:
            errx(
                EX_DATAERR, "%s: invalid color type %hhu and bit depth %hhu",
                path, header.color, header.depth
            );
    }
    if (header.compression != DEFLATE) {
        errx(
            EX_DATAERR, "%s: invalid compression method %hhu",
            path, header.compression
        );
    }
    if (header.filter != ADAPTIVE) {
        errx(EX_DATAERR, "%s: invalid filter method %hhu", path, header.filter);
    }
    if (header.interlace > ADAM7) {
        errx(EX_DATAERR, "%s: invalid interlace method %hhu", path, header.interlace);
    }
}

static void writeHeader(void) {
    struct Chunk ihdr = { .size = sizeof(header), .type = { 'I', 'H', 'D', 'R' } };
    writeChunk(ihdr);
    header.width = htonl(header.width);
    header.height = htonl(header.height);
    writeExpect(&header, sizeof(header));
    writeCrc();
    header.width = ntohl(header.width);
    header.height = ntohl(header.height);
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
        if (0 == memcmp(chunk.type, "IEND", 4)) {
            errx(EX_DATAERR, "%s: missing IDAT chunk", path);
        }
        if (0 != memcmp(chunk.type, "IDAT", 4)) {
            if (chunk.type[0] & 0x20) {
                int error = fseek(file, chunk.size + 4, SEEK_CUR);
                if (error) err(EX_IOERR, "%s", path);
                continue;
            }
            errx(
                EX_CONFIG, "%s: unsupported critical chunk %s",
                path, typeStr(chunk)
            );
        }

        uint8_t idat[chunk.size];
        readExpect(idat, sizeof(idat), "image data");
        readCrc();

        stream.next_in = idat;
        stream.avail_in = chunk.size;
        int error = inflate(&stream, Z_SYNC_FLUSH);
        if (error == Z_STREAM_END) break;
        if (error != Z_OK) errx(EX_DATAERR, "%s: inflate: %s", path, stream.msg);
    }
    inflateEnd(&stream);

    if (stream.total_out != dataSize()) {
        errx(
            EX_DATAERR, "%s: expected data size %zu, found %zu",
            path, dataSize(), stream.total_out
        );
    }
}

static void writeData(void) {
    size_t size = compressBound(dataSize());
    uint8_t deflate[size];
    int error = compress2(deflate, &size, data, dataSize(), Z_BEST_COMPRESSION);
    if (error != Z_OK) errx(EX_SOFTWARE, "%s: compress2: %d", path, error);

    struct Chunk idat = { .size = size, .type = { 'I', 'D', 'A', 'T' } };
    writeChunk(idat);
    writeExpect(deflate, size);
    writeCrc();
}

static void writeEnd(void) {
    struct Chunk iend = { .size = 0, .type = { 'I', 'E', 'N', 'D' } };
    writeChunk(iend);
    writeCrc();
}

enum PACKED Filter {
    NONE,
    SUB,
    UP,
    AVERAGE,
    PAETH,
};
#define FILTER_COUNT (PAETH + 1)

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
        case NONE:    return f.x;
        case SUB:     return f.x + f.a;
        case UP:      return f.x + f.b;
        case AVERAGE: return f.x + ((uint32_t)f.a + (uint32_t)f.b) / 2;
        case PAETH:   return f.x + paethPredictor(f);
    }
}

static uint8_t filt(enum Filter type, struct Bytes f) {
    switch (type) {
        case NONE:    return f.x;
        case SUB:     return f.x - f.a;
        case UP:      return f.x - f.b;
        case AVERAGE: return f.x - ((uint32_t)f.a + (uint32_t)f.b) / 2;
        case PAETH:   return f.x - paethPredictor(f);
    }
}

static struct {
    enum Filter *type;
    uint8_t *data;
} *lines;

static void scanlines(void) {
    lines = calloc(header.height, sizeof(*lines));
    if (!lines) err(EX_OSERR, "calloc(%u, %zu)", header.height, sizeof(*lines));

    size_t stride = 1 + lineSize();
    for (uint32_t y = 0; y < header.height; ++y) {
        lines[y].type = &data[y * stride];
        lines[y].data = &data[y * stride + 1];
        if (*lines[y].type >= FILTER_COUNT) {
            errx(EX_DATAERR, "%s: invalid filter type %hhu", path, *lines[y].type);
        }
    }
}

static struct Bytes origBytes(uint32_t y, size_t i) {
    size_t pixelSize = lineSize() / header.width;
    if (!pixelSize) pixelSize = 1;
    bool a = (i >= pixelSize), b = (y > 0), c = (a && b);
    return (struct Bytes) {
        .x = lines[y].data[i],
        .a = a ? lines[y].data[i - pixelSize] : 0,
        .b = b ? lines[y - 1].data[i] : 0,
        .c = c ? lines[y - 1].data[i - pixelSize] : 0,
    };
}

static void reconData(void) {
    for (uint32_t y = 0; y < header.height; ++y) {
        for (size_t i = 0; i < lineSize(); ++i) {
            lines[y].data[i] =
                recon(*lines[y].type, origBytes(y, i));
        }
        *lines[y].type = NONE;
    }
}

static void filterData(void) {
    for (uint32_t y = header.height - 1; y < header.height; --y) {
        uint8_t filter[FILTER_COUNT][lineSize()];
        uint32_t heuristic[FILTER_COUNT] = { 0 };
        enum Filter minType = NONE;
        for (enum Filter type = NONE; type < FILTER_COUNT; ++type) {
            for (uint32_t i = 0; i < lineSize(); ++i) {
                filter[type][i] = filt(type, origBytes(y, i));
                heuristic[type] += abs((int8_t)filter[type][i]);
            }
            if (heuristic[type] < heuristic[minType]) minType = type;
        }
        *lines[y].type = minType;
        memcpy(lines[y].data, filter[minType], lineSize());
    }
}

static void eliminateAlpha(void) {
    if (!(header.color & ALPHA)) return;
    size_t sampleSize = header.depth / 8;
    size_t pixelSize = 4 * sampleSize;
    for (uint32_t y = 0; y < header.height; ++y) {
        for (uint32_t x = 0; x < header.width; ++x) {
            for (size_t i = 0; i < sampleSize; ++i) {
                if (lines[y].data[x * pixelSize + 3 * sampleSize + i] != 0xFF) return;
            }
        }
    }

    uint8_t *ptr = data;
    for (uint32_t y = 0; y < header.height; ++y) {
        uint8_t *type = ptr++;
        uint8_t *data = ptr;
        *type = *lines[y].type;
        for (uint32_t x = 0; x < header.width; ++x) {
            memcpy(ptr, &lines[y].data[x * pixelSize], 3 * sampleSize);
            ptr += 3 * sampleSize;
        }
        lines[y].type = type;
        lines[y].data = data;
    }
    header.color &= ~ALPHA;
}

static void eliminateColor(void) {
    if (!(header.color & TRUECOLOR)) return;
    size_t sampleSize = header.depth / 8;
    size_t pixelSize = ((header.color & ALPHA) ? 4 : 3) * sampleSize;
    for (uint32_t y = 0; y < header.height; ++y) {
        for (uint32_t x = 0; x < header.width; ++x) {
            if (
                0 != memcmp(
                    &lines[y].data[x * pixelSize],
                    &lines[y].data[x * pixelSize + 1 * sampleSize],
                    sampleSize
                )
            ) return;
            if (
                0 != memcmp(
                    &lines[y].data[x * pixelSize + 1 * sampleSize],
                    &lines[y].data[x * pixelSize + 2 * sampleSize],
                    sampleSize
                )
            ) return;
        }
    }

    uint8_t *ptr = data;
    for (uint32_t y = 0; y < header.height; ++y) {
        uint8_t *type = ptr++;
        uint8_t *data = ptr;
        *type = *lines[y].type;
        for (uint32_t x = 0; x < header.width; ++x) {
            memcpy(ptr, &lines[y].data[x * pixelSize], sampleSize);
            ptr += sampleSize;
            if (header.color & ALPHA) {
                memcpy(
                    ptr,
                    &lines[y].data[x * pixelSize + 3 * sampleSize],
                    sampleSize
                );
                ptr += sampleSize;
            }
        }
        lines[y].type = type;
        lines[y].data = data;
    }
    header.color &= ~TRUECOLOR;
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
    if (header.interlace != PROGRESSIVE) {
        errx(
            EX_CONFIG, "%s: unsupported interlace method %hhu",
            path, header.interlace
        );
    }
    readData();

    int error = fclose(file);
    if (error) err(EX_IOERR, "%s", path);

    scanlines();
    reconData();
    eliminateAlpha();
    eliminateColor();
    filterData();
    free(lines);

    if (outPath) {
        path = outPath;
        file = fopen(path, "wx");
        if (!file) err(EX_CANTCREAT, "%s", path);
    } else {
        path = "(stdout)";
        file = stdout;
    }

    writeSignature();
    writeHeader();
    writeData();
    writeEnd();
    free(data);

    error = fclose(file);
    if (error) err(EX_IOERR, "%s", path);
}

int main(int argc, char *argv[]) {
    if (argc < 2) return EX_USAGE;
    optimize(argv[1], NULL);
    return EX_OK;
}
