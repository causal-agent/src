#include <stdint.h>

static uint32_t color;

void init() {
}

void draw(uint32_t *buf, uint32_t xres, uint32_t yres) {
    for (uint32_t i = 0; i < xres * yres; ++i) {
        buf[i] = color;
    }
}

void input(char c) {
    color = color << 4 | (c & 0xF);
}
