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

#include "proxy.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>

#define MAX_EVENTS 4096
#define BUFFER_SIZE 8192

struct engine_epoll {
	PROXY_ENGINE_STRUCT;
	char buffer[BUFFER_SIZE];
	struct proxy_pair deleted;
	struct epoll_event events[MAX_EVENTS];
};

static void *proxy_routine(void *arg)
{
	int i, ready;
	ssize_t bytes;
	struct proxy_node *node;
	struct proxy_pair *pair, pair_next;
	struct engine_epoll *object = (struct engine_epoll *)arg;

	while (object->thread_running) {
		pthread_mutex_lock(&object->mutex);
		if (object->stat.pairs > 0)
			ready = epoll_wait(object->pfd, object->events, MAX_EVENTS, 10);
		else {
			ready = 0;
			usleep(10 * 1000);
		}
		pthread_mutex_unlock(&object->mutex);
		object->deleted.next = NULL;
		for (i = 0; i < ready; i++) {
			node = (struct proxy_node *)object->events[i].data.ptr;
			if (node->pair->deleted)
				continue;
			if (object->events[i].events & EPOLLIN) {
				bytes = recv(node->sock, object->buffer, BUFFER_SIZE, 0);
				if (bytes <= 0 || send(node->link->sock, object->buffer, bytes, 0) != bytes)
					node->pair->deleted = 1;
				else {
					if (node == &node->pair->node[0]) {
						object->stat.in_packets++;
						object->stat.in_bytes += bytes;
					} else {
						object->stat.out_packets++;
						object->stat.out_bytes += bytes;
					}
				}
			}
			if (object->events[i].events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP))
				node->pair->deleted = 1;
			if (node->pair->deleted) {
				node->pair->next = object->deleted.next;
				object->deleted.next = node->pair;
			}
		}
		pthread_mutex_lock(&object->mutex);
		pair = object->deleted.next;
		while (pair != NULL) {
			close(pair->node[0].sock);
			close(pair->node[1].sock);
			object->stat.pairs--;
			pair_next = pair->next;
			pair->next = object->pool;
			object->pool = pair;
			pair = pair_next;
		}
		pthread_mutex_unlock(&object->mutex);
	}
	return (void *)0;
}

int proxy_create(struct proxy_engine **engine)
{
	int retval = -1;
	struct engine_epoll *object;

	object = (struct engine_epoll *)malloc(sizeof(struct engine_epoll));
	if (object) {
		memset(object, 0, sizeof(*object));
		object->pfd = epoll_create(1);
		if (object->pfd >= 0) {
			pthread_mutex_init(&object->mutex, NULL);
			object->thread_running = 1;
			if (pthread_create(&object->thread, NULL, proxy_routine, object) == 0) {
				*engine = (struct proxy_engine *)object;
				retval = 0;
			} else {
				pthread_mutex_destroy(&object->mutex);
				close(object->pfd);
			}
		}
		if (retval)
			free(object);
	}
	return retval;
}

int proxy_close(struct proxy_engine *engine)
{
	int retval = -1;
	struct proxy_pair *pair;
	struct engine_epoll *object = (struct engine_epoll *)engine;

	if (object) {
		object->thread_running = 0;
		pthread_join(object->thread, NULL);
		pthread_mutex_destroy(&object->mutex);
		close(object->pfd);
		for (pair = object->pool; pair != NULL; pair = pair->next)
			free(pair);
		free(object);
		retval = 0;
	}
	return retval;
}

int proxy_addpair(struct proxy_engine *engine, int sock1, int sock2)
{
	int retval = -1;
	struct proxy_pair *pair;
	struct epoll_event event;
	struct engine_epoll *object = (struct engine_epoll *)engine;

	if (object) {
		pthread_mutex_lock(&object->mutex);
		if (object->pool) {
			pair = object->pool;
			object->pool = pair->next;
		} else
			pair = (struct proxy_pair *)malloc(sizeof(struct proxy_pair));
		if (pair) {
			memset(pair, 0, sizeof(*pair));
			pair->deleted = 0;
			pair->node[0].sock = sock1;
			pair->node[0].link = &pair->node[1];
			pair->node[0].pair = pair;
			pair->node[1].sock = sock2;
			pair->node[1].link = &pair->node[0];
			pair->node[1].pair = pair;
			event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;
			event.data.ptr = &pair->node[0];
			if (epoll_ctl(object->pfd, EPOLL_CTL_ADD, pair->node[0].sock, &event) == 0) {
				event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP;
				event.data.ptr = &pair->node[1];
				if (epoll_ctl(object->pfd, EPOLL_CTL_ADD, pair->node[1].sock, &event) == 0) {
					object->stat.pairs++;
					retval = 0;
				} else
					epoll_ctl(object->pfd, EPOLL_CTL_DEL, pair->node[0].sock, &event);
			}
			if (retval) {
				pair->next = object->pool;
				object->pool = pair;
			}
		}
		pthread_mutex_unlock(&object->mutex);
	}
	return retval;
}

