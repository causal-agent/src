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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <zlib.h>

#define PACKED __attribute__((packed))

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
    if (ntohl(found) != expected) {
        errx(
            EX_DATAERR, "%s: expected CRC32 %08x, found %08x",
            path, expected, found
        );
    }
}

struct PACKED Header {
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    enum PACKED {
        GRAYSCALE       = 0,
        TRUECOLOR       = 2,
        INDEXED         = 3,
        GRAYSCALE_ALPHA = 4,
        TRUECOLOR_ALPHA = 6,
    } color;
    uint8_t compression;
    uint8_t filter;
    uint8_t interlace;
};

static uint8_t bytesPerPixel(struct Header header) {
    assert(header.depth == 8);
    switch (header.color) {
        case GRAYSCALE:       return 1;
        case TRUECOLOR:       return 3;
        case INDEXED:         return 1;
        case GRAYSCALE_ALPHA: return 2;
        case TRUECOLOR_ALPHA: return 4;
    }
}

static size_t dataSize(struct Header header) {
    return (1 + bytesPerPixel(header) * header.width) * header.height;
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
    readExpect(path, file, &header, sizeof(header), "IHDR data");
    readCrc(path, file, crc32(crc, (Bytef *)&header, sizeof(header)));

    header.width = ntohl(header.width);
    header.height = ntohl(header.height);

    if (!header.width) errx(EX_DATAERR, "%s: invalid width 0", path);
    if (!header.height) errx(EX_DATAERR, "%s: invalid height 0", path);
    if (
        header.depth != 1
        && header.depth != 2
        && header.depth != 4
        && header.depth != 8
        && header.depth != 16
    ) errx(EX_DATAERR, "%s: invalid bit depth %hhu", path, header.depth);
    if (
        header.color != GRAYSCALE
        && header.color != TRUECOLOR
        && header.color != INDEXED
        && header.color != GRAYSCALE_ALPHA
        && header.color != TRUECOLOR_ALPHA
    ) errx(EX_DATAERR, "%s: invalid color type %hhu", path, header.color);
    if (header.compression) {
        errx(
            EX_DATAERR, "%s: invalid compression method %hhu",
            path, header.compression
        );
    }
    if (header.filter) {
        errx(EX_DATAERR, "%s: invalid filter method %hhu", path, header.filter);
    }
    if (header.interlace > 1) {
        errx(EX_DATAERR, "%s: invalid interlace method %hhu", path, header.interlace);
    }

    return header;
}

static uint8_t *readData(const char *path, FILE *file, struct Header header) {
    size_t size = dataSize(header);
    uint8_t *data = malloc(size);
    if (!data) err(EX_OSERR, "malloc(%zu)", size);

    struct z_stream_s stream = { .next_out = data, .avail_out = size };
    int error = inflateInit(&stream);
    if (error != Z_OK) errx(EX_SOFTWARE, "%s: inflateInit: %s", path, stream.msg);

    for (;;) {
        struct Chunk chunk = readChunk(path, file);
        if (0 == memcmp(chunk.type, "IEND", 4)) {
            errx(EX_DATAERR, "%s: expected IDAT chunk, found IEND", path);
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

    if (stream.total_out != size) {
        errx(
            EX_DATAERR, "%s: expected data size %zu, found %zu",
            path, size, stream.total_out
        );
    }

    return data;
}

enum PACKED FilterType {
    FILT_NONE,
    FILT_SUB,
    FILT_UP,
    FILT_AVERAGE,
    FILT_PAETH,
};

static uint8_t paethPredictor(uint8_t a, uint8_t b, uint8_t c) {
    int32_t p = (int32_t)a + (int32_t)b - (int32_t)c;
    int32_t pa = labs(p - (int32_t)a);
    int32_t pb = labs(p - (int32_t)b);
    int32_t pc = labs(p - (int32_t)c);
    if (pa <= pb && pa <= pc) return a;
    if (pb < pc) return b;
    return c;
}

static uint8_t recon(
    enum FilterType type,
    uint8_t x, uint8_t a, uint8_t b, uint8_t c
) {
    switch (type) {
        case FILT_NONE:    return x;
        case FILT_SUB:     return x + a;
        case FILT_UP:      return x + b;
        case FILT_AVERAGE: return x + ((uint32_t)a + (uint32_t)b) / 2;
        case FILT_PAETH:   return x + paethPredictor(a, b, c);
    }
}

static uint8_t filt(
    enum FilterType type,
    uint8_t x, uint8_t a, uint8_t b, uint8_t c
) {
    switch (type) {
        case FILT_NONE:    return x;
        case FILT_SUB:     return x - a;
        case FILT_UP:      return x - b;
        case FILT_AVERAGE: return x - ((uint32_t)a + (uint32_t)b) / 2;
        case FILT_PAETH:   return x - paethPredictor(a, b, c);
    }
}

struct Scanline {
    enum FilterType *type;
    uint8_t *ptr;
};

static void reconData(const char *path, struct Header header, uint8_t *data) {
    uint8_t bpp = bytesPerPixel(header);
    uint32_t stride = 1 + bpp * header.width;

    struct Scanline lines[header.height];
    for (uint32_t y = 0; y < header.height; ++y) {
        lines[y].type = &data[y * stride];
        lines[y].ptr = &data[y * stride + 1];
        if (*lines[y].type > FILT_PAETH) {
            errx(EX_DATAERR, "%s: invalid filter type %hhu", path, *lines[y].type);
        }
    }

    for (uint32_t y = 0; y < header.height; ++y) {
        for (uint32_t x = 0; x < header.width; ++x) {
            for (uint8_t i = 0; i < bpp; ++i) {
                lines[y].ptr[x * bpp + i] = recon(
                    *lines[y].type,
                    lines[y].ptr[x * bpp + i],
                    x ? lines[y].ptr[(x - 1) * bpp + i] : 0,
                    y ? lines[y - 1].ptr[x * bpp + i] : 0,
                    (x && y) ? lines[y - 1].ptr[(x - 1) * bpp + i] : 0
                );
            }
        }
        *lines[y].type = FILT_NONE;
    }
}

static void filterData(struct Header header, uint8_t *data) {
    uint8_t bpp = bytesPerPixel(header);
    uint32_t stride = 1 + bpp * header.width;

    struct Scanline lines[header.height];
    for (uint32_t y = 0; y < header.height; ++y) {
        lines[y].type = &data[y * stride];
        lines[y].ptr = &data[y * stride + 1];
        assert(*lines[y].type == FILT_NONE);
    }

    for (uint32_t y = header.height - 1; y < header.height; --y) {
        for (uint32_t x = header.width - 1; x < header.width; --x) {
            for (uint8_t i = 0; i < bpp; ++i) {
                // TODO: Filter type heuristic.
                *lines[y].type = FILT_PAETH;
                lines[y].ptr[x * bpp + i] = filt(
                    FILT_PAETH,
                    lines[y].ptr[x * bpp + i],
                    x ? lines[y].ptr[(x - 1) * bpp + i] : 0,
                    y ? lines[y - 1].ptr[x * bpp + i] : 0,
                    (x && y) ? lines[y - 1].ptr[(x - 1) * bpp + i] : 0
                );
            }
        }
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

int main(int argc, char *argv[]) {
    const char *path = "stdin";
    FILE *file = stdin;
    if (argc > 1) {
        path = argv[1];
        file = fopen(path, "r");
        if (!file) err(EX_NOINPUT, "%s", path);
    }

    readSignature(path, file);
    struct Header header = readHeader(path, file);
    if (header.interlace) {
        errx(
            EX_CONFIG, "%s: unsupported interlace method %hhu",
            path, header.interlace
        );
    }
    if (header.depth != 8) {
        errx(EX_CONFIG, "%s: unsupported bit depth %hhu", path, header.depth);
    }

    uint8_t *data = readData(path, file, header);

    reconData(path, header, data);

    // TODO: "Optimize".

    filterData(header, data);

    // TODO: -o
    path = "stdout";
    file = stdout;

    writeSignature(path, file);
    writeHeader(path, file, header);
    writeData(path, file, data, dataSize(header));
    writeEnd(path, file);

    int error = fclose(file);
    if (error) err(EX_IOERR, "%s", path);
}
