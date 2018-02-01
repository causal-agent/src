/* Copyright (c) 2017, June McEnroe <programble@gmail.com>
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

#include <curses.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>

static const char *WORDS[] = {
#include "words.txt"
};
#define WORDS_LEN (sizeof(WORDS) / sizeof(WORDS[0]))

static const char **targets;
static size_t targetsLen;

static void setTarget(size_t i) {
    const char *target;
    size_t len;
    do {
        target = WORDS[arc4random_uniform(WORDS_LEN)];
        len = strlen(target);
    } while (len < 3 || len > 9);
    targets[i] = target;
    move(i, 0);
    clrtoeol();
    mvaddstr(i, arc4random_uniform(COLS - len), target);
}

int main() {
    initscr();
    cbreak();
    echo();

    targetsLen = LINES - 2;
    targets = calloc(targetsLen, sizeof(*targets));
    for (size_t i = 0; i < targetsLen; ++i) {
        setTarget(i);
    }
    mvhline(LINES - 2, 0, ACS_HLINE, COLS);

    time_t start = time(NULL);
    if (start < 0) err(EX_OSERR, "time");

    int letterCount = 0;
    for (;;) {
        time_t now = time(NULL);
        if (now < 0) err(EX_OSERR, "time");

        double wpm = (double)letterCount / 5.0 / difftime(now, start) * 60.0;

        char *wpmDisplay;
        int len = asprintf(&wpmDisplay, "%.2f WPM ", wpm);
        if (len < 0) err(EX_OSERR, "asprintf");

        move(LINES - 1, 0);
        clrtoeol();
        mvaddstr(LINES - 1, COLS - len, wpmDisplay);
        free(wpmDisplay);

        move(LINES - 1, 0);
        char word[16];
        getnstr(word, sizeof(word));

        for (size_t i = 0; i < targetsLen; ++i) {
            if (strcmp(word, targets[i])) continue;
            setTarget(i);
            letterCount += strlen(word);
            break;
        }
    }
}
