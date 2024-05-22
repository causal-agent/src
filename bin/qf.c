/* Copyright (C) 2022  June McEnroe <june@causal.agency>
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

#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

enum Type {
	File,
	Match,
	Context,
	Text,
};

struct Line {
	enum Type type;
	char *path;
	unsigned nr;
	char *text;
	regmatch_t match;
};

static struct {
	struct Line *ptr;
	size_t len, cap;
} lines;

static void push(struct Line line) {
	if (lines.len == lines.cap) {
		lines.cap = (lines.cap ? lines.cap * 2 : 256);
		lines.ptr = realloc(lines.ptr, sizeof(*lines.ptr) * lines.cap);
		if (!lines.ptr) err(1, "realloc");
	}
	lines.ptr[lines.len++] = line;
}

static const char *pattern;
static regex_t regex;

static void parse(struct Line line) {
	line.path = strsep(&line.text, ":");
	if (!line.text) {
		line.type = Text;
		line.text = line.path;
		if (lines.len) line.path = lines.ptr[lines.len-1].path;
		push(line);
		return;
	}
	char *rest;
	line.nr = strtoul(line.text, &rest, 10);
	struct Line prev = {0};
	if (lines.len) prev = lines.ptr[lines.len-1];
	if (!prev.path || strcmp(line.path, prev.path)) {
		if (lines.len) push((struct Line) { .type = Text, .text = " " });
		line.type = File;
		push(line);
	}
	if (rest > line.text && rest[0] == ':') {
		line.type = Match;
		line.text = &rest[1];
	} else if (rest > line.text && rest[0] == '-') {
		line.type = Context;
		line.text = &rest[1];
	} else {
		line.type = Text;
	}
	if (line.type == Match && pattern) {
		regexec(&regex, line.text, 1, &line.match, 0);
	}
	push(line);
}

enum {
	Path = 1,
	Number = 2,
	Highlight = 3,
};

static void curse(void) {
	set_term(newterm(NULL, stdout, stderr));
	cbreak();
	noecho();
	nodelay(stdscr, true);
	TABSIZE = 4;
	curs_set(0);
	start_color();
	use_default_colors();
	init_pair(Path, COLOR_GREEN, -1);
	init_pair(Number, COLOR_YELLOW, -1);
	init_pair(Highlight, COLOR_MAGENTA, -1);
}

static size_t top;
static size_t cur;
static bool reading = true;

static void draw(void) {
	int y = 0, x = 0;
	for (int i = 0; i < LINES; ++i) {
		move(i, 0);
		clrtoeol();
		if (top + i >= lines.len) {
			addstr(reading ? "..." : !lines.len ? "No results" : "");
			break;
		}
		struct Line line = lines.ptr[top + i];
		if (top + i == cur) {
			getyx(stdscr, y, x);
			attron(A_REVERSE);
		} else {
			attroff(A_REVERSE);
		}
		switch (line.type) {
			break; case File: {
				color_set(Path, NULL);
				addstr(line.path);
				color_set(0, NULL);
			}
			break; case Match: {
				color_set(Number, NULL);
				printw("%u", line.nr);
				color_set(0, NULL);
				addch(':');
				if (line.match.rm_so == line.match.rm_eo) {
					addstr(line.text);
					break;
				}
				addnstr(line.text, line.match.rm_so);
				color_set(Highlight, NULL);
				addnstr(
					&line.text[line.match.rm_so],
					line.match.rm_eo - line.match.rm_so
				);
				color_set(0, NULL);
				addstr(&line.text[line.match.rm_eo]);
			}
			break; case Context: {
				color_set(Number, NULL);
				printw("%u", line.nr);
				color_set(0, NULL);
				addch('-');
				addstr(line.text);
			}
			break; case Text: addstr(line.text);
		}
	}
	move(y, x);
	refresh();
}

static void edit(struct Line line) {
	char cmd[32];
	snprintf(cmd, sizeof(cmd), "+%u", (line.nr ? line.nr : 1));
	const char *editor = getenv("EDITOR");
	if (!editor) editor = "vi";
	pid_t pid = fork();
	if (pid < 0) err(1, "fork");
	if (!pid) {
		dup2(STDERR_FILENO, STDIN_FILENO);
		execlp(editor, editor, cmd, line.path, NULL);
		err(127, "%s", editor);
	}
	int status;
	pid = waitpid(pid, &status, 0);
	if (pid < 0) err(1, "waitpid");
}

static void toPrev(enum Type type) {
	if (!cur) return;
	size_t prev = cur - 1;
	while (prev && lines.ptr[prev].type != type) {
		prev--;
	}
	if (lines.ptr[prev].type == type) {
		cur = prev;
	}
}

static void toNext(enum Type type) {
	size_t next = cur + 1;
	while (next < lines.len && lines.ptr[next].type != type) {
		next++;
	}
	if (next < lines.len && lines.ptr[next].type == type) {
		cur = next;
	}
}

static void input(void) {
	char ch;
	while (ERR != (ch = getch())) {
		switch (ch) {
			break; case '\n': {
				if (lines.ptr[cur].type == Text) break;
				endwin();
				edit(lines.ptr[cur]);
				refresh();
			}
			break; case '{': toPrev(File);
			break; case '}': toNext(File);
			break; case 'G': cur = lines.len - 1;
			break; case 'N': toPrev(Match);
			break; case 'g': cur = 0;
			break; case 'j': if (cur + 1 < lines.len) cur++;
			break; case 'k': if (cur) cur--;
			break; case 'n': toNext(Match);
			break; case 'q': {
				endwin();
				exit(0);
			}
			break; case 'r': clearok(stdscr, true);
		}
	}
	if (cur < top) top = cur;
	if (cur >= top + LINES) top = cur - LINES + 1;
}

int main(int argc, char *argv[]) {
	if (isatty(STDIN_FILENO)) errx(1, "no input");
	if (argc > 1) {
		pattern = argv[1];
		int flags = REG_EXTENDED | REG_ICASE;
		for (const char *ch = pattern; *ch; ++ch) {
			if (isupper(*ch)) {
				flags &= ~REG_ICASE;
				break;
			}
		}
		int error = regcomp(&regex, pattern, flags);
		if (error) errx(1, "invalid pattern");
	}
	curse();
	draw();
	struct pollfd fds[2] = {
		{ .fd = STDERR_FILENO, .events = POLLIN },
		{ .fd = STDIN_FILENO, .events = POLLIN },
	};
	size_t len = 0;
	size_t cap = 4096;
	char *buf = malloc(cap);
	if (!buf) err(1, "malloc");
	while (poll(fds, (reading ? 2 : 1), -1)) {
		if (fds[0].revents) {
			input();
		}
		if (reading && fds[1].revents) {
			ssize_t n = read(fds[1].fd, &buf[len], cap - len);
			if (n < 0) err(1, "read");
			if (!n) reading = false;
			len += n;
			char *ptr = buf;
			for (
				char *nl;
				(nl = memchr(ptr, '\n', &buf[len] - ptr));
				ptr = &nl[1]
			) {
				struct Line line = { .text = strndup(ptr, nl - ptr) };
				if (!line.text) err(1, "strndup");
				parse(line);
			}
			len -= ptr - buf;
			memmove(buf, ptr, len);
			if (len == cap) {
				cap *= 2;
				buf = realloc(buf, cap);
				if (!buf) err(1, "realloc");
			}
		}
		draw();
	}
	err(1, "poll");
}
