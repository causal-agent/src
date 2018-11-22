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

#include <assert.h>
#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <sysexits.h>

#include "edi.h"

struct Log logAlloc(size_t cap) {
	struct State *states = malloc(sizeof(*states) * cap);
	if (!states) err(EX_OSERR, "malloc");
	return (struct Log) {
		.cap = cap,
		.len = 0,
		.state = 0,
		.states = states,
	};
}

void logFree(struct Log *log) {
	for (size_t i = 0; i < log->len; ++i) {
		tableFree(&log->states[i].table);
	}
	free(log->states);
}

void logPush(struct Log *log, struct Table table) {
	if (log->len == log->cap) {
		log->cap *= 2;
		log->states = realloc(log->states, sizeof(*log->states) * log->cap);
		if (!log->states) err(EX_OSERR, "realloc");
	}
	size_t next = log->len++;
	log->states[next] = (struct State) {
		.table = table,
		.prev = log->state,
		.next = next,
	};
	log->states[log->state].next = next;
	log->state = next;
}
