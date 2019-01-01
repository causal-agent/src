/* Copyright (C) 2018  June McEnroe <june@causal.agency>
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

#include "gfx.h"

static size_t width = 800;
static size_t height = 600;

static Display *display;
static Window window;
static Atom WM_DELETE_WINDOW;
static GC windowGc;
static XImage *image;

static size_t bufSize;
static uint32_t *buf;

static size_t pixmapWidth;
static size_t pixmapHeight;
static Pixmap pixmap;

static void createWindow(void) {
	display = XOpenDisplay(NULL);
	if (!display) errx(EX_UNAVAILABLE, "XOpenDisplay: %s", XDisplayName(NULL));

	Window root = DefaultRootWindow(display);
	window = XCreateSimpleWindow(display, root, 0, 0, width, height, 0, 0, 0);

	WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

	windowGc = XCreateGC(display, window, 0, NULL);

	image = XCreateImage(display, NULL, 24, ZPixmap, 0, NULL, width, height, 32, 0);
}

static void resizePixmap(void) {
	size_t newSize = 4 * width * height;
	if (newSize > bufSize) {
		bufSize = newSize;
		free(buf);
		buf = malloc(bufSize);
		if (!buf) err(EX_OSERR, "malloc(%zu)", bufSize);
	}

	image->data = (char *)buf;
	image->width = width;
	image->height = height;
	image->bytes_per_line = 4 * width;

	if (width > pixmapWidth || height > pixmapHeight) {
		pixmapWidth = width;
		pixmapHeight = height;
		if (pixmap) XFreePixmap(display, pixmap);
		pixmap = XCreatePixmap(display, window, pixmapWidth, pixmapHeight, 24);
	}
}

static void drawWindow(void) {
	draw(buf, width, height);
	XPutImage(display, pixmap, windowGc, image, 0, 0, 0, 0, width, height);
	XCopyArea(display, pixmap, window, windowGc, 0, 0, width, height, 0, 0);
}

int main(int argc, char *argv[]) {
	int error = init(argc, argv);
	if (error) return error;

	createWindow();
	resizePixmap();
	drawWindow();

	XStoreName(display, window, status());
	XMapWindow(display, window);

	XEvent event;
	XSelectInput(display, window, ExposureMask | StructureNotifyMask | KeyPressMask);
	for (;;) {
		XNextEvent(display, &event);

		switch (event.type) {
			case KeyPress: {
				XKeyEvent key = event.xkey;
				KeySym sym = XLookupKeysym(&key, key.state & ShiftMask);
				if (sym > 0x80) break;
				if (!input(sym)) return EX_OK;
				drawWindow();
				XStoreName(display, window, status());
			} break;

			case ConfigureNotify: {
				XConfigureEvent configure = event.xconfigure;
				width = configure.width;
				height = configure.height;
				resizePixmap();
				drawWindow();
			} break;

			case Expose: {
				XExposeEvent expose = event.xexpose;
				XCopyArea(
					display, pixmap, window, windowGc,
					expose.x, expose.y,
					expose.width, expose.height,
					expose.x, expose.y
				);
			} break;

			case ClientMessage: {
				XClientMessageEvent client = event.xclient;
				if ((Atom)client.data.l[0] == WM_DELETE_WINDOW) return EX_OK;
			} break;
		}
	}
}
