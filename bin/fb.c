#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

extern void init(int argc, char *argv[]);
extern void draw(uint32_t *buf, uint32_t xres, uint32_t yres);
extern void input(char in);

static size_t bufLen;
static uint32_t *buf;
static uint32_t *saveBuf;

static struct termios saveTerm;

static void restoreBuf(void) {
    memcpy(buf, saveBuf, bufLen);
}

static void restoreTerm(void) {
    tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
    int error;

    char *path = getenv("FRAMEBUFFER");
    if (!path) path = "/dev/fb0";

    int fd = open(path, O_RDWR);
    if (fd < 0) err(EX_OSFILE, "%s", path);

    struct fb_fix_screeninfo fixInfo;
    error = ioctl(fd, FBIOGET_FSCREENINFO, &fixInfo);
    if (error) err(EX_IOERR, "%s", path);

    struct fb_var_screeninfo varInfo;
    error = ioctl(fd, FBIOGET_VSCREENINFO, &varInfo);
    if (error) err(EX_IOERR, "%s", path);

    assert(!varInfo.xoffset);
    assert(!varInfo.yoffset);
    assert(varInfo.bits_per_pixel == 32);
    assert(!varInfo.grayscale);
    assert(varInfo.red.offset == 16);
    assert(varInfo.green.offset == 8);
    assert(varInfo.blue.offset == 0);
    assert(fixInfo.line_length == varInfo.xres * 4);

    bufLen = varInfo.xres * varInfo.yres * 4;
    buf = mmap(NULL, bufLen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) err(EX_IOERR, "%s", path);

    saveBuf = malloc(bufLen);
    if (!saveBuf) err(EX_OSERR, "malloc(%zu)", bufLen);
    memcpy(saveBuf, buf, bufLen);
    atexit(restoreBuf);

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr");
    atexit(restoreTerm);

    struct termios raw;
    cfmakeraw(&raw);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &raw);
    if (error) err(EX_IOERR, "tcsetattr");

    init(argc, argv);

    for(;;) {
        draw(buf, varInfo.xres, varInfo.yres);

        char in;
        ssize_t len = read(STDIN_FILENO, &in, 1);
        if (len < 0) err(EX_IOERR, "read");
        if (!len) return EX_DATAERR;
        if (in == CTRL('C')) return EX_USAGE;

        input(in);
    }
}
