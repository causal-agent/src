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

#import <Cocoa/Cocoa.h>
#import <err.h>
#import <stdbool.h>
#import <stdint.h>
#import <stdlib.h>
#import <sysexits.h>

#import "gfx.h"

#define UNUSED __attribute__((unused))

@interface BufferView : NSView {
    size_t bufSize;
    uint32_t *buf;
    CGColorSpaceRef colorSpace;
    CGDataProviderRef dataProvider;
}
@end

@implementation BufferView
- (instancetype) initWithFrame: (NSRect) frameRect {
    colorSpace = CGColorSpaceCreateDeviceRGB();
    return [super initWithFrame: frameRect];
}

- (void) setWindowTitle {
    [[self window] setTitle: [NSString stringWithUTF8String: status()]];
}

- (void) draw {
    draw(buf, [self frame].size.width, [self frame].size.height);
    [self setNeedsDisplay: YES];
}

- (void) setFrameSize: (NSSize) newSize {
    [super setFrameSize: newSize];
    size_t newBufSize = 4 * newSize.width * newSize.height;
    if (newBufSize > bufSize) {
        bufSize = newBufSize;
        buf = malloc(bufSize);
        if (!buf) err(EX_OSERR, "malloc(%zu)", bufSize);
        CGDataProviderRelease(dataProvider);
        dataProvider = CGDataProviderCreateWithData(NULL, buf, bufSize, NULL);
    }
    [self draw];
}

- (void) drawRect: (NSRect) UNUSED dirtyRect {
    NSSize size = [self frame].size;
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    CGImageRef image = CGImageCreate(
        size.width, size.height,
        8, 32, 4 * size.width,
        colorSpace, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
        dataProvider,
        NULL, false, kCGRenderingIntentDefault
    );
    CGContextDrawImage(ctx, [self frame], image);
    CGImageRelease(image);
}

- (BOOL) acceptsFirstResponder {
    return YES;
}

- (void) keyDown: (NSEvent *) event {
    char in;
    BOOL converted = [
        [event characters]
        getBytes: &in
        maxLength: 1
        usedLength: NULL
        encoding: NSASCIIStringEncoding
        options: 0
        range: NSMakeRange(0, 1)
        remainingRange: NULL
    ];
    if (converted) {
        if (!input(in)) {
            [NSApp terminate: self];
        }
        [self setWindowTitle];
        [self draw];
    }
}
@end

@interface Delegate : NSObject <NSApplicationDelegate>
@end

@implementation Delegate
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *) UNUSED sender {
    return YES;
}
@end

int main(int argc, char *argv[]) {
    int error = init(argc, argv);
    if (error) return error;

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    [NSApp setDelegate: [Delegate new]];

    NSString *name = [[NSProcessInfo processInfo] processName];
    NSMenu *menu = [NSMenu new];
    NSMenuItem *quit = [
        [NSMenuItem alloc]
        initWithTitle: [@"Quit " stringByAppendingString: name]
        action: @selector(terminate:)
        keyEquivalent: @"q"
    ];
    [menu addItem: quit];
    NSMenuItem *menuItem = [NSMenuItem new];
    [menuItem setSubmenu: menu];
    [NSApp setMainMenu: [NSMenu new]];
    [[NSApp mainMenu] addItem: menuItem];

    NSUInteger style = NSTitledWindowMask
        | NSClosableWindowMask
        | NSMiniaturizableWindowMask
        | NSResizableWindowMask;
    NSWindow *window = [
        [NSWindow alloc]
        initWithContentRect: NSMakeRect(0, 0, 800, 600)
        styleMask: style
        backing: NSBackingStoreBuffered
        defer: YES
    ];
    [window center];

    BufferView *view = [[BufferView alloc] initWithFrame: [window frame]];
    [window setContentView: view];
    [view setWindowTitle];

    [window makeKeyAndOrderFront: nil];
    [NSApp activateIgnoringOtherApps: YES];
    [NSApp run];
}
