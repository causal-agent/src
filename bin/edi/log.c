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
	assert(cap);
	struct State *states = malloc(sizeof(*states) * cap);
	if (!states) err(EX_OSERR, "malloc");
	states[0] = (struct State) {
		.table = NULL,
		.prev = 0,
		.next = 0,
	};
	return (struct Log) {
		.cap = cap,
		.len = 1,
		.index = 0,
		.states = states,
	};
}

void logFree(struct Log *log) {
	for (size_t i = 0; i < log->len; ++i) {
		free(log->states[i].table);
	}
	free(log->states);
}

void logPush(struct Log *log, struct Table *table) {
	if (log->len == log->cap) {
		log->cap *= 2;
		log->states = realloc(log->states, sizeof(*log->states) * log->cap);
		if (!log->states) err(EX_OSERR, "realloc");
	}
	size_t next = log->len++;
	log->states[next] = (struct State) {
		.table = table,
		.prev = log->index,
		.next = next,
	};
	log->states[log->index].next = next;
	log->index = next;
}
