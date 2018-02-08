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

#include <X11/Xlib.h>
#include <err.h>
#include <sysexits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

extern int init(int argc, char *argv[]);
extern const char *status(void);
extern void draw(uint32_t *buf, size_t width, size_t height);
extern bool input(char in);

static size_t width;
static size_t height;

static size_t bufSize;
static uint32_t *buf;

static Display *display;
static Window window;
static GC gc;
static XImage *image;
static Pixmap pixmap;

static void resize(size_t newWidth, size_t newHeight) {
    size_t newSize = 4 * newWidth * newHeight;
    if (newSize > bufSize) {
        free(buf);
        buf = malloc(newSize);
        if (!buf) err(EX_OSERR, "malloc(%zu)", newSize);
        bufSize = newSize;
    }

    image->data = (char *)buf;
    image->width = newWidth;
    image->height = newHeight;
    image->bytes_per_line = 4 * newWidth;

    if (pixmap) XFreePixmap(display, pixmap);
    pixmap = XCreatePixmap(display, window, newWidth, newHeight, 24);

    width = newWidth;
    height = newHeight;
}

static void drawWindow(void) {
    draw(buf, width, height);
    XPutImage(display, pixmap, gc, image, 0, 0, 0, 0, width, height);
    XCopyArea(display, pixmap, window, gc, 0, 0, width, height, 0, 0);
}

int main(int argc, char *argv[]) {
    int error = init(argc, argv);
    if (error) return error;

    display = XOpenDisplay(NULL);
    if (!display) errx(EX_UNAVAILABLE, "XOpenDisplay: %s", XDisplayName(NULL));

    Window root = DefaultRootWindow(display);
    window = XCreateSimpleWindow(display, root, 0, 0, 800, 600, 0, 0, 0);
    gc = XCreateGC(display, window, 0, NULL);
    image = XCreateImage(display, NULL, 24, ZPixmap, 0, NULL, 0, 0, 32, 0);

    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

    XStoreName(display, window, status());
    XMapWindow(display, window);

    XEvent event;
    XSelectInput(display, window, ExposureMask | StructureNotifyMask | KeyPressMask);
    for (;;) {
        XNextEvent(display, &event);
        switch (event.type) {
            case KeyPress: {
                XKeyEvent key = event.xkey;
                KeySym sym = XLookupKeysym(&key, key.state);
                if (sym > 128) break;
                if (!input(sym)) return EX_OK;
                drawWindow();
            } break;

            case ConfigureNotify: {
                XConfigureEvent configure = event.xconfigure;
                resize(configure.width, configure.height);
                drawWindow();
            } break;

            case Expose: {
                XExposeEvent expose = event.xexpose;
                XCopyArea(
                    display,
                    pixmap, window, gc,
                    expose.x, expose.y,
                    expose.width, expose.height,
                    expose.x, expose.y
                );
            } break;

            case ClientMessage: {
                XClientMessageEvent message = event.xclient;
                if ((Atom)message.data.l[0] == WM_DELETE_WINDOW) {
                    return EX_OK;
                }
            } break;
        }
    }
}
