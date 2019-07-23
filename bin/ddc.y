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

static struct tm normalize(struct tm date) {
	time_t time = mktime(&date);
	struct tm *norm = localtime(&time);
	if (!norm) err(EX_OSERR, "localtime");
	return *norm;
}

static struct tm today(void) {
	time_t now = time(NULL);
	struct tm *date = localtime(&now);
	if (!date) err(EX_OSERR, "localtime");
	date->tm_hour = date->tm_min = date->tm_sec = 0;
	return *date;
}

static struct tm monthDay(int month, int day) {
	struct tm date = today();
	date.tm_mon = month;
	date.tm_mday = day;
	return normalize(date);
}

static struct tm monthDayYear(int month, int day, int year) {
	struct tm date = {
		.tm_mon = month,
		.tm_mday = day,
		.tm_year = year - 1900,
	};
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
	struct tm diff = { .tm_year = a.tm_year - b.tm_year };
	if (a.tm_mday < b.tm_mday) {
		diff.tm_mon = a.tm_mon - b.tm_mon - 1;
		while (dateAdd(b, diff).tm_mday != a.tm_mday) diff.tm_mday++;
	} else {
		diff.tm_mon = a.tm_mon - b.tm_mon;
		diff.tm_mday = a.tm_mday - b.tm_mday;
	}
	return diff;
}

static const char *Days[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};

static const char *Months[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

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
		printf("%dd\n", scalar.tm_mday);
	} else {
		printf("%dw\n", scalar.tm_mday / 7);
	}
}

%}

%token Number Month Day
%left '+' '-'

%%

expr:
	date { printDate($1); }
	| scalar { printScalar($1); }
	;

date:
	dateSpec
	| date '+' scalar { $$ = dateAdd($1, $3); }
	| date '-' scalar { $$ = dateSub($1, $3); }
	;

scalar:
	scalarSpec
	| scalar '+' scalar { $$ = scalarAdd($1, $3); }
	| scalar '-' scalar { $$ = scalarSub($1, $3); }
	| date '-' date { $$ = dateDiff($1, $3); }
	;

dateSpec:
	'.' { $$ = today(); }
	| Month Number { $$ = monthDay($1.tm_mon, $2.tm_sec); }
	| Month Number Number { $$ = monthDayYear($1.tm_mon, $2.tm_sec, $3.tm_sec); }
	| Day { $$ = weekDay($1.tm_wday); }
	;

scalarSpec:
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

int main(void) {
	char *buf = NULL;
	size_t cap = 0;
	while (0 < getline(&buf, &cap, stdin)) {
		input = buf;
		yyparse();
		printf("\n");
	}
}
