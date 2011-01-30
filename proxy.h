/*
 * This file is part of pproxy.
 *
 * Copyright (C) 2011  Vietor Liu, <vietor@vxwo.org>
 * 
 * pproxy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pproxy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pproxy. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROXY_H
#define PROXY_H

#include <sys/socket.h>
#include <pthread.h>

struct proxy_node {
	int sock;
	struct proxy_node *link;
	struct proxy_pair *pair;
};

struct proxy_pair {
	int deleted;
	struct proxy_node node[2];
	struct proxy_pair *next;
};

struct proxy_stat {
	unsigned long pairs;
	unsigned long long in_packets;
	unsigned long long in_bytes;
	unsigned long long out_packets;
	unsigned long long out_bytes;
};

#define PROXY_ENGINE_STRUCT \
	int pfd; \
	pthread_t thread; \
	int thread_running; \
	pthread_mutex_t mutex; \
	struct proxy_stat stat; \
	struct proxy_pair *pool

struct proxy_engine {
	PROXY_ENGINE_STRUCT;
};

int proxy_create(struct proxy_engine **engine);
int proxy_close(struct proxy_engine *engine);
int proxy_addpair(struct proxy_engine *engine, int sock1, int sock2);
int proxy_status(struct proxy_engine *engine, struct proxy_stat *stat);

#endif

