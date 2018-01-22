#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASK_RANK   (0x0F)
#define MASK_SUIT   (0x30)
#define MASK_COLOR  (0x10)
#define MASK_UP     (0x40)
#define MASK_SELECT (0x80)

#define SUIT_CLUB    (0x00)
#define SUIT_DIAMOND (0x10)
#define SUIT_SPADE   (0x20)
#define SUIT_HEART   (0x30)

struct Stack {
    uint8_t data[52];
    uint8_t index;
};
#define EMPTY { .data = {0}, .index = 52 }

static void push(struct Stack *stack, uint8_t value) {
    assert(stack->index > 0);
    stack->data[--stack->index] = value;
}

static uint8_t pop(struct Stack *stack) {
    assert(stack->index < 52);
    return stack->data[stack->index++];
}

static uint8_t get(const struct Stack *stack, uint8_t i) {
    if (stack->index + i > 51) return 0;
    return stack->data[stack->index + i];
}

static uint8_t len(const struct Stack *stack) {
    return 52 - stack->index;
}

static struct {
    struct Stack stock;
    struct Stack waste;
    struct Stack found[4];
    struct Stack table[7];
} snap, g = {
    .stock = {
        .data = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
            0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
            0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
            0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
            0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
            0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
        },
        .index = 0,
    },
    .waste = EMPTY,
    .found = { EMPTY, EMPTY, EMPTY },
    .table = {
        EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
    },
};

static void snapshot(void) {
    memcpy(&snap, &g, sizeof(g));
}

static void undo(void) {
    memcpy(&g, &snap, sizeof(g));
}

static void shuffle(void) {
    for (int i = 51; i > 0; --i) {
        int j = arc4random_uniform(i + 1);
        uint8_t x = g.stock.data[i];
        g.stock.data[i] = g.stock.data[j];
        g.stock.data[j] = x;
    }
}

static void deal(void) {
    for (int i = 0; i < 7; ++i) {
        for (int j = i; j < 7; ++j) {
            push(&g.table[j], pop(&g.stock));
        }
    }
}

static void reveal(void) {
    for (int i = 0; i < 7; ++i) {
        if (get(&g.table[i], 0)) {
            push(&g.table[i], pop(&g.table[i]) | MASK_UP);
        }
    }
}

static void draw(void) {
    if (get(&g.stock, 0)) push(&g.waste, pop(&g.stock) | MASK_UP);
    if (get(&g.stock, 0)) push(&g.waste, pop(&g.stock) | MASK_UP);
    if (get(&g.stock, 0)) push(&g.waste, pop(&g.stock) | MASK_UP);
}

static void wasted(void) {
    for (int i = g.waste.index; i < 52; ++i) {
        push(&g.stock, pop(&g.waste) & ~MASK_UP);
    }
}

static bool validFound(const struct Stack *found, uint8_t card) {
    if (!get(found, 0)) return true;
    if ((card & MASK_SUIT) != (get(found, 0) & MASK_SUIT)) return false;
    return (card & MASK_RANK) == (get(found, 0) & MASK_RANK) + 1;
}

static bool validTable(const struct Stack *table, uint8_t card) {
    if (!get(table, 0)) return (card & MASK_RANK) == 13;
    if ((card & MASK_COLOR) != (get(table, 0) & MASK_COLOR)) return false;
    return (card & MASK_RANK) == (get(table, 0) & MASK_RANK) - 1;
}

enum {
    PAIR_EMPTY = 1,
    PAIR_BACK,
    PAIR_BLACK,
    PAIR_RED,
};

static void curse(void) {
    setlocale(LC_CTYPE, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    set_escdelay(100);
    curs_set(0);

    start_color();
    assume_default_colors(-1, -1);
    init_pair(PAIR_EMPTY, COLOR_WHITE, COLOR_BLACK);
    init_pair(PAIR_BACK, COLOR_WHITE, COLOR_BLUE);
    init_pair(PAIR_BLACK, COLOR_BLACK, COLOR_WHITE);
    init_pair(PAIR_RED, COLOR_RED, COLOR_WHITE);
}

static const char rank[] = "\0A23456789TJQK";
static const char *suit[] = {
    [SUIT_HEART]   = "♥",
    [SUIT_CLUB]    = "♣",
    [SUIT_DIAMOND] = "♦",
    [SUIT_SPADE]   = "♠",
};

static void renderCard(int y, int x, uint8_t card) {
    if (card & MASK_UP) {
        bkgdset(
            COLOR_PAIR(card & MASK_COLOR ? PAIR_RED : PAIR_BLACK)
            | (card & MASK_SELECT ? A_REVERSE : A_NORMAL)
        );

        move(y, x);
        addch(rank[card & MASK_RANK]);
        addstr(suit[card & MASK_SUIT]);
        addch(' ');

        move(y + 1, x);
        addstr(suit[card & MASK_SUIT]);
        addch(' ');
        addstr(suit[card & MASK_SUIT]);

        move(y + 2, x);
        addch(' ');
        addstr(suit[card & MASK_SUIT]);
        addch(rank[card & MASK_RANK]);

    } else {
        bkgdset(COLOR_PAIR(PAIR_EMPTY));
        if (card) {
            bkgdset(COLOR_PAIR(PAIR_BACK));
        }

        mvaddstr(y, x, "   ");
        mvaddstr(y + 1, x, "   ");
        mvaddstr(y + 2, x, "   ");
    }
}

static void render(void) {
    bkgdset(COLOR_PAIR(0));
    erase();

    int x = 2;
    int y = 1;

    renderCard(y, x, get(&g.stock, 0));

    x += 5;
    renderCard(y, x++, get(&g.waste, 2));
    renderCard(y, x++, get(&g.waste, 1));
    renderCard(y, x, get(&g.waste, 0));

    x += 5;
    for (int i = 0; i < 4; ++i) {
        renderCard(y, x, get(&g.found[i], 0));
        x += 4;
    }

    x = 2;
    for (int i = 0; i < 7; ++i) {
        y = 5;
        for (int j = len(&g.table[i]); j > 0; --j) {
            renderCard(y, x, get(&g.table[i], j - 1));
            y++;
        }
        x += 4;
    }
}

int main() {
    curse();

    shuffle();
    deal();
    snapshot();

    for (;;) {
        reveal();
        render();

        switch (getch()) {
            case 's':
            case ' ':
                snapshot();
                if (get(&g.stock, 0)) {
                    draw();
                } else {
                    wasted();
                }
                break;

            case 'w':
                if (get(&g.waste, 0)) {
                    push(&g.waste, pop(&g.waste) ^ MASK_SELECT);
                }
                break;

            case 'u':
                undo();
                break;

            case 'q':
                endwin();
                return 0;
        }
    }
}
