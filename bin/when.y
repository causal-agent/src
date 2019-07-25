/* Copyright (C) 2019  June McEnroe <june@causal.agency>
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
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};

static const char *Months[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
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
	time_t atime = timegm(&a), btime = timegm(&b);
	diff.tm_yday = (atime - btime) / 24 / 60 / 60;
	return diff;
}

static void printDate(struct tm date) {
	printf(
		"%s %s %d %d\n",
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
	if (scalar.tm_yday && scalar.tm_mon) printf("(%dd) ", scalar.tm_yday);
	printf("\n");
}

%}

%token Number Month Day
%left '+' '-'
%right '<' '>'

%%

expr:
	date { printDate($1); }
	| scalar { printScalar($1); }
	;

date:
	dateLit
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

	for (int i = 0; i < 7; ++i) {
		if (strncasecmp(input, Days[i], 3)) continue;
		while (isalpha(*input)) input++;
		yylval.tm_wday = i;
		return Day;
	}

	for (int i = 0; i < 12; ++i) {
		if (strncasecmp(input, Months[i], 3)) continue;
		while (isalpha(*input)) input++;
		yylval.tm_mon = i;
		return Month;
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
