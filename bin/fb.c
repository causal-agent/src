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

extern void draw(uint32_t *buf, uint32_t xres, uint32_t yres);
extern void input(char c);

static uint32_t size;
static uint32_t *buf;
static uint32_t *saveBuf;

static void restoreBuf(void) {
    memcpy(buf, saveBuf, size);
}

static struct termios saveTerm;

static void restoreTerm(void) {
    tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
}

int main() {
    int error;

    char *path = getenv("FRAMEBUFFER");
    if (!path) path = "/dev/fb0";

    int fd = open(path, O_RDWR);
    if (fd < 0) err(EX_OSFILE, "%s", path);

    struct fb_fix_screeninfo fix;
    error = ioctl(fd, FBIOGET_FSCREENINFO, &fix);
    if (error) err(EX_IOERR, "%s", path);

    struct fb_var_screeninfo var;
    error = ioctl(fd, FBIOGET_VSCREENINFO, &var);
    if (error) err(EX_IOERR, "%s", path);

    assert(!var.xoffset);
    assert(!var.yoffset);
    assert(var.bits_per_pixel == 32);
    assert(!var.grayscale);
    assert(var.red.offset == 16);
    assert(var.red.length == 8);
    assert(var.green.offset == 8);
    assert(var.green.length == 8);
    assert(var.blue.offset == 0);
    assert(var.blue.length == 8);
    assert(fix.line_length == var.xres * 4);

    size = var.xres * var.yres * 4;
    buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) err(EX_OSERR, "%s", path);

    saveBuf = malloc(size);
    if (!saveBuf) err(EX_OSERR, "malloc");
    memcpy(saveBuf, buf, size);
    atexit(restoreBuf);

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr");
    atexit(restoreTerm);

    struct termios raw;
    cfmakeraw(&raw);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &raw);
    if (error) err(EX_IOERR, "tcsetattr");

    for (;;) {
        draw(buf, var.xres, var.yres);

        char c;
        ssize_t len = read(STDIN_FILENO, &c, 1);
        if (len < 0) err(EX_IOERR, "read");
        if (!len || c == CTRL('C')) return EX_OK;

        input(c);
    }
}
