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

static void readExpect(
    const char *path, FILE *file,
    void *ptr, size_t size,
    const char *expect
) {
    fread(ptr, size, 1, file);
    if (ferror(file)) err(EX_IOERR, "%s", path);
    if (feof(file)) errx(EX_DATAERR, "%s: missing %s", path, expect);
}

static const uint8_t SIGNATURE[8] = {
    0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'
};

static void readSignature(const char *path, FILE *file) {
    uint8_t signature[8];
    readExpect(path, file, signature, 8, "signature");
    if (0 != memcmp(signature, SIGNATURE, 8)) {
        errx(EX_DATAERR, "%s: invalid signature", path);
    }
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

static struct Chunk readChunk(const char *path, FILE *file) {
    struct Chunk chunk;
    readExpect(path, file, &chunk, sizeof(chunk), "chunk");
    chunk.size = ntohl(chunk.size);
    return chunk;
}

static void readCrc(const char *path, FILE *file, uint32_t expected) {
    uint32_t found;
    readExpect(path, file, &found, sizeof(found), "CRC32");
    found = ntohl(found);
    if (found != expected) {
        errx(
            EX_DATAERR, "%s: expected CRC32 %08x, found %08x",
            path, expected, found
        );
    }
}

enum PACKED Color {
    GRAYSCALE       = 0,
    TRUECOLOR       = 2,
    INDEXED         = 3,
    GRAYSCALE_ALPHA = 4,
    TRUECOLOR_ALPHA = 6
};
#define ALPHA (0x04)

struct PACKED Header {
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    enum Color color;
    enum PACKED { DEFLATE } compression;
    enum PACKED { ADAPTIVE } filter;
    enum PACKED { PROGRESSIVE, ADAM7 } interlace;
};

static size_t lineSize(struct Header header) {
    switch (header.color) {
        case GRAYSCALE:       return (header.width * 1 * header.depth + 7) / 8;
        case TRUECOLOR:       return (header.width * 3 * header.depth + 7) / 8;
        case INDEXED:         return (header.width * 1 * header.depth + 7) / 8;
        case GRAYSCALE_ALPHA: return (header.width * 2 * header.depth + 7) / 8;
        case TRUECOLOR_ALPHA: return (header.width * 4 * header.depth + 7) / 8;
    }
}

static size_t dataSize(struct Header header) {
    return (1 + lineSize(header)) * header.height;
}

static struct Header readHeader(const char *path, FILE *file) {
    struct Chunk ihdr = readChunk(path, file);
    if (0 != memcmp(ihdr.type, "IHDR", 4)) {
        errx(EX_DATAERR, "%s: expected IHDR, found %s", path, typeStr(ihdr));
    }
    if (ihdr.size != sizeof(struct Header)) {
        errx(
            EX_DATAERR, "%s: expected IHDR size %zu, found %u",
            path, sizeof(struct Header), ihdr.size
        );
    }
    uint32_t crc = crc32(CRC_INIT, (Bytef *)ihdr.type, sizeof(ihdr.type));

    struct Header header;
    readExpect(path, file, &header, sizeof(header), "header");
    readCrc(path, file, crc32(crc, (Bytef *)&header, sizeof(header)));

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

    return header;
}

static uint8_t *readData(const char *path, FILE *file, struct Header header) {
    uint8_t *data = malloc(dataSize(header));
    if (!data) err(EX_OSERR, "malloc(%zu)", dataSize(header));

    struct z_stream_s stream = { .next_out = data, .avail_out = dataSize(header) };
    int error = inflateInit(&stream);
    if (error != Z_OK) errx(EX_SOFTWARE, "%s: inflateInit: %s", path, stream.msg);

    for (;;) {
        struct Chunk chunk = readChunk(path, file);
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

        uint32_t crc = crc32(CRC_INIT, (Bytef *)chunk.type, sizeof(chunk.type));

        uint8_t idat[chunk.size];
        readExpect(path, file, idat, sizeof(idat), "image data");
        readCrc(path, file, crc32(crc, idat, sizeof(idat)));

        stream.next_in = idat;
        stream.avail_in = chunk.size;
        int error = inflate(&stream, Z_SYNC_FLUSH);
        if (error == Z_STREAM_END) break;
        if (error != Z_OK) errx(EX_DATAERR, "%s: inflate: %s", path, stream.msg);
    }
    inflateEnd(&stream);

    if (stream.total_out != dataSize(header)) {
        errx(
            EX_DATAERR, "%s: expected data size %zu, found %zu",
            path, dataSize(header), stream.total_out
        );
    }

    return data;
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

struct Scanline {
    enum Filter *type;
    uint8_t *data;
};

static struct Scanline *scanlines(
    const char *path, struct Header header, uint8_t *data
) {
    struct Scanline *lines = calloc(header.height, sizeof(*lines));
    if (!lines) err(EX_OSERR, "calloc(%u, %zu)", header.height, sizeof(*lines));

    size_t stride = 1 + lineSize(header);
    for (uint32_t y = 0; y < header.height; ++y) {
        lines[y].type = &data[y * stride];
        lines[y].data = &data[y * stride + 1];
        if (*lines[y].type >= FILTER_COUNT) {
            errx(EX_DATAERR, "%s: invalid filter type %hhu", path, *lines[y].type);
        }
    }

    return lines;
}

static struct Bytes origBytes(
    struct Header header, const struct Scanline *lines,
    uint32_t y, size_t i
) {
    size_t pixelSize = lineSize(header) / header.width;
    if (!pixelSize) pixelSize = 1;
    bool a = (i >= pixelSize), b = (y > 0), c = (a && b);
    return (struct Bytes) {
        .x = lines[y].data[i],
        .a = a ? lines[y].data[i - pixelSize] : 0,
        .b = b ? lines[y - 1].data[i] : 0,
        .c = c ? lines[y - 1].data[i - pixelSize] : 0,
    };
}

static void reconData(struct Header header, const struct Scanline *lines) {
    for (uint32_t y = 0; y < header.height; ++y) {
        for (size_t i = 0; i < lineSize(header); ++i) {
            lines[y].data[i] =
                recon(*lines[y].type, origBytes(header, lines, y, i));
        }
        *lines[y].type = NONE;
    }
}

static void filterData(struct Header header, const struct Scanline *lines) {
    for (uint32_t y = header.height - 1; y < header.height; --y) {
        uint8_t filter[FILTER_COUNT][lineSize(header)];
        uint32_t heuristic[FILTER_COUNT] = { 0 };
        enum Filter minType = NONE;
        for (enum Filter type = NONE; type < FILTER_COUNT; ++type) {
            for (uint32_t i = 0; i < lineSize(header); ++i) {
                filter[type][i] = filt(type, origBytes(header, lines, y, i));
                heuristic[type] += abs((int8_t)filter[type][i]);
            }
            if (heuristic[type] < heuristic[minType]) minType = type;
        }
        *lines[y].type = minType;
        memcpy(lines[y].data, filter[minType], lineSize(header));
    }
}

static void writeExpect(const char *path, FILE *file, const void *ptr, size_t size) {
    fwrite(ptr, size, 1, file);
    if (ferror(file)) err(EX_IOERR, "%s", path);
}

static void writeSignature(const char *path, FILE *file) {
    writeExpect(path, file, SIGNATURE, sizeof(SIGNATURE));
}

static void writeChunk(const char *path, FILE *file, struct Chunk chunk) {
    chunk.size = htonl(chunk.size);
    writeExpect(path, file, &chunk, sizeof(chunk));
}

static void writeCrc(const char *path, FILE *file, uint32_t crc) {
    uint32_t net = htonl(crc);
    writeExpect(path, file, &net, sizeof(net));
}

static void writeHeader(const char *path, FILE *file, struct Header header) {
    struct Chunk ihdr = { .size = sizeof(header), .type = { 'I', 'H', 'D', 'R' } };
    writeChunk(path, file, ihdr);
    uint32_t crc = crc32(CRC_INIT, (Bytef *)ihdr.type, sizeof(ihdr.type));
    header.width = htonl(header.width);
    header.height = htonl(header.height);
    writeExpect(path, file, &header, sizeof(header));
    writeCrc(path, file, crc32(crc, (Bytef *)&header, sizeof(header)));
}

static void writeData(const char *path, FILE *file, uint8_t *data, size_t size) {
    size_t bound = compressBound(size);
    uint8_t deflate[bound];
    int error = compress2(deflate, &bound, data, size, Z_BEST_COMPRESSION);
    if (error != Z_OK) errx(EX_SOFTWARE, "%s: compress2: %d", path, error);

    struct Chunk idat = { .size = bound, .type = { 'I', 'D', 'A', 'T' } };
    writeChunk(path, file, idat);
    uint32_t crc = crc32(CRC_INIT, (Bytef *)idat.type, sizeof(idat.type));
    writeExpect(path, file, deflate, bound);
    writeCrc(path, file, crc32(crc, deflate, bound));
}

static void writeEnd(const char *path, FILE *file) {
    struct Chunk iend = { .size = 0, .type = { 'I', 'E', 'N', 'D' } };
    writeChunk(path, file, iend);
    writeCrc(path, file, crc32(CRC_INIT, (Bytef *)iend.type, sizeof(iend.type)));
}

static void eliminateAlpha(
    struct Header *header, uint8_t *data, struct Scanline *lines
) {
    if (!(header->color & ALPHA)) return;

    size_t pixelSize = lineSize(*header) / header->width;
    size_t alphaSize = header->depth / 8;
    size_t colorSize = pixelSize - alphaSize;

    for (uint32_t y = 0; y < header->height; ++y) {
        for (uint32_t x = 0; x < header->width; ++x) {
            for (size_t i = 0; i < alphaSize; ++i) {
                if (lines[y].data[x * pixelSize + colorSize + i] != 0xFF) return;
            }
        }
    }

    uint8_t *ptr = data;
    for (uint32_t y = 0; y < header->height; ++y) {
        uint8_t *type = ptr++;
        uint8_t *data = ptr;
        *type = *lines[y].type;
        for (uint32_t x = 0; x < header->width; ++x) {
            memcpy(ptr, &lines[y].data[x * pixelSize], colorSize);
            ptr += colorSize;
        }
        lines[y].type = type;
        lines[y].data = data;
    }

    header->color &= ~ALPHA;
}

static void optimize(const char *inPath, const char *outPath) {
    FILE *input = stdin;
    if (inPath) {
        input = fopen(inPath, "r");
        if (!input) err(EX_NOINPUT, "%s", inPath);
    } else {
        inPath = "stdin";
    }

    readSignature(inPath, input);
    struct Header header = readHeader(inPath, input);
    if (header.interlace != PROGRESSIVE) {
        errx(
            EX_CONFIG, "%s: unsupported interlace method %hhu",
            inPath, header.interlace
        );
    }
    uint8_t *data = readData(inPath, input, header);

    int error = fclose(input);
    if (error) err(EX_IOERR, "%s", inPath);

    struct Scanline *lines = scanlines(inPath, header, data);
    reconData(header, lines);

    eliminateAlpha(&header, data, lines);
    filterData(header, lines);

    FILE *output = stdout;
    if (outPath) {
        output = fopen(outPath, "wx");
        if (!output) err(EX_CANTCREAT, "%s", outPath);
    } else {
        outPath = "stdout";
    }

    writeSignature(outPath, output);
    writeHeader(outPath, output, header);
    writeData(outPath, output, data, dataSize(header));
    writeEnd(outPath, output);

    error = fclose(output);
    if (error) err(EX_IOERR, "%s", outPath);

    free(lines);
    free(data);
}

int main(int argc, char *argv[]) {
    if (argc < 2) return EX_USAGE;
    optimize(argv[1], NULL);
    return EX_OK;
}
