/* Copyright (C) 2019, 2022  June McEnroe <june@causal.agency>
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

%{

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sysexits.h>
#include <time.h>

static void yyerror(const char *str);
static int yylex(void);

#define YYSTYPE struct tm

static const char *Days[7] = {
	"Sunday", "Monday", "Tuesday", "Wednesday",
	"Thursday", "Friday", "Saturday",
};

static const char *Months[12] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December",
};

static const struct tm Week = { .tm_mday = 7 };

static struct tm normalize(struct tm date) {
	time_t time = timegm(&date);
	struct tm *norm = gmtime(&time);
	if (!norm) err(EX_OSERR, "gmtime");
	return *norm;
}

static struct tm today(void) {
	time_t now = time(NULL);
	struct tm *local = localtime(&now);
	if (!local) err(EX_OSERR, "localtime");
	struct tm date = {
		.tm_year = local->tm_year,
		.tm_mon = local->tm_mon,
		.tm_mday = local->tm_mday,
	};
	return normalize(date);
}

static struct tm monthDay(int month, int day) {
	struct tm date = today();
	date.tm_mon = month;
	date.tm_mday = day;
	return normalize(date);
}

static struct tm monthDayYear(int month, int day, int year) {
	struct tm date = today();
	date.tm_mon = month;
	date.tm_mday = day;
	date.tm_year = year - 1900;
	return normalize(date);
}

static struct tm weekDay(int day) {
	struct tm date = today();
	date.tm_mday += day - date.tm_wday;
	return normalize(date);
}

static struct tm scalarAdd(struct tm a, struct tm b) {
	a.tm_mday += b.tm_mday;
	a.tm_mon += b.tm_mon;
	a.tm_year += b.tm_year;
	return a;
}

static struct tm scalarSub(struct tm a, struct tm b) {
	a.tm_mday -= b.tm_mday;
	a.tm_mon -= b.tm_mon;
	a.tm_year -= b.tm_year;
	return a;
}

static struct tm dateAdd(struct tm date, struct tm scalar) {
	return normalize(scalarAdd(date, scalar));
}

static struct tm dateSub(struct tm date, struct tm scalar) {
	return normalize(scalarSub(date, scalar));
}

static struct tm dateDiff(struct tm a, struct tm b) {
	time_t atime = timegm(&a), btime = timegm(&b);
	if (atime < btime) {
		struct tm x = a;
		a = b;
		b = x;
		time_t xtime = atime;
		atime = btime;
		btime = xtime;
	}
	struct tm diff = {
		.tm_year = a.tm_year - b.tm_year,
		.tm_mon = a.tm_mon - b.tm_mon,
		.tm_mday = a.tm_mday - b.tm_mday,
	};
	if (a.tm_mon < b.tm_mon) {
		diff.tm_year--;
		diff.tm_mon += 12;
	}
	if (a.tm_mday < b.tm_mday) {
		diff.tm_mon--;
		diff.tm_mday = 0;
		while (dateAdd(b, diff).tm_mday != a.tm_mday) diff.tm_mday++;
	}
	diff.tm_yday = (atime - btime) / 24 / 60 / 60;
	return diff;
}

static struct {
	size_t cap, len;
	struct tm *ptr;
} dates;

static struct tm getDate(const char *name) {
	for (size_t i = 0; i < dates.len; ++i) {
		if (!strcmp(dates.ptr[i].tm_zone, name)) return dates.ptr[i];
	}
	return (struct tm) {0};
}

static void setDate(const char *name, struct tm date) {
	for (size_t i = 0; i < dates.len; ++i) {
		if (strcmp(dates.ptr[i].tm_zone, name)) continue;
		char *tm_zone = dates.ptr[i].tm_zone;
		dates.ptr[i] = date;
		dates.ptr[i].tm_zone = tm_zone;
		return;
	}
	if (dates.len == dates.cap) {
		dates.cap = (dates.cap ? dates.cap * 2 : 8);
		dates.ptr = realloc(dates.ptr, sizeof(*dates.ptr) * dates.cap);
		if (!dates.ptr) err(EX_OSERR, "realloc");
	}
	dates.ptr[dates.len] = date;
	dates.ptr[dates.len].tm_zone = strdup(name);
	if (!dates.ptr[dates.len].tm_zone) err(EX_OSERR, "strdup");
	dates.len++;
}

static void printDate(struct tm date) {
	printf(
		"%.3s %.3s %d %d\n",
		Days[date.tm_wday], Months[date.tm_mon],
		date.tm_mday, 1900 + date.tm_year
	);
}

static void printScalar(struct tm scalar) {
	if (scalar.tm_year) printf("%dy ", scalar.tm_year);
	if (scalar.tm_mon) printf("%dm ", scalar.tm_mon);
	if (scalar.tm_mday % 7) {
		printf("%dd ", scalar.tm_mday);
	} else if (scalar.tm_mday) {
		printf("%dw ", scalar.tm_mday / 7);
	}
	if (scalar.tm_yday && scalar.tm_mon) {
		if (scalar.tm_yday >= 7) {
			printf("(%dw", scalar.tm_yday / 7);
			if (scalar.tm_yday % 7) {
				printf(" %dd", scalar.tm_yday % 7);
			}
			printf(") ");
		}
		printf("(%dd) ", scalar.tm_yday);
	}
	printf("\n");
}

%}

%token Name Number Month Day
%left '+' '-'
%right '=' '<' '>'

%%

expr:
	date { printDate($1); }
	| scalar { printScalar($1); }
	;

date:
	dateLit
	| Name { $$ = getDate($1.tm_zone); free($1.tm_zone); }
	| Name '=' date { setDate($1.tm_zone, $3); free($1.tm_zone); $$ = $3; }
	| '(' date ')' { $$ = $2; }
	| '<' date { $$ = dateSub($2, Week); }
	| '>' date { $$ = dateAdd($2, Week); }
	| date '+' scalar { $$ = dateAdd($1, $3); }
	| date '-' scalar { $$ = dateSub($1, $3); }
	;

scalar:
	scalarLit
	| '(' scalar ')' { $$ = $2; }
	| scalar '+' scalar { $$ = scalarAdd($1, $3); }
	| scalar '-' scalar { $$ = scalarSub($1, $3); }
	| date '-' date { $$ = dateDiff($1, $3); }
	;

dateLit:
	{ $$ = today(); }
	| '.' { $$ = today(); }
	| Month Number { $$ = monthDay($1.tm_mon, $2.tm_sec); }
	| Month Number Number { $$ = monthDayYear($1.tm_mon, $2.tm_sec, $3.tm_sec); }
	| Day { $$ = weekDay($1.tm_wday); }
	;

scalarLit:
	Number 'd' { $$ = (struct tm) { .tm_mday = $1.tm_sec }; }
	| Number 'w' { $$ = (struct tm) { .tm_mday = 7 * $1.tm_sec }; }
	| Number 'm' { $$ = (struct tm) { .tm_mon = $1.tm_sec }; }
	| Number 'y' { $$ = (struct tm) { .tm_year = $1.tm_sec }; }
	;

%%

static void yyerror(const char *str) {
	warnx("%s", str);
}

static const char *input;

static int yylex(void) {
	while (isspace(*input)) input++;
	if (!*input) return EOF;

	if (isdigit(*input)) {
		char *rest;
		yylval.tm_sec = strtol(input, &rest, 10);
		input = rest;
		return Number;
	}

	size_t len;
	for (len = 0; isalnum(input[len]) || input[len] == '_'; ++len);

	if (len >= 3) {
		for (int i = 0; i < 7; ++i) {
			if (strncasecmp(input, Days[i], len)) continue;
			yylval.tm_wday = i;
			input += len;
			return Day;
		}

		for (int i = 0; i < 12; ++i) {
			if (strncasecmp(input, Months[i], len)) continue;
			yylval.tm_mon = i;
			input += len;
			return Month;
		}
	}

	if (len && (len != 1 || !strchr("dwmy", *input))) {
		yylval.tm_zone = strndup(input, len);
		if (!yylval.tm_zone) err(EX_OSERR, "strndup");
		input += len;
		return Name;
	}

	return *input++;
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
		input = argv[1];
		return yyparse();
	}

	struct tm date = today();
	printDate(date);
	printf("\n");

	char *line = NULL;
	size_t cap = 0;
	while (0 < getline(&line, &cap, stdin)) {
		if (line[0] == '\n') continue;

		if (today().tm_mday != date.tm_mday) {
			warnx("the date has changed");
			date = today();
		}

		input = line;
		yyparse();
		printf("\n");
	}
}
