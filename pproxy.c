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

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct sockaddr_in remote_addr;

void *proxy_thread_routine(void *arg)
{
	fd_set rfds;
	ssize_t rlen;
	char buffer[4096];
	int ivalue, remote = -1;
	int local = (int)(long)arg;

	remote = socket(AF_INET, SOCK_STREAM, 0);
	if (remote == -1)
		goto _exit_point;
	if (connect(remote, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
		goto _exit_point;

	ivalue = 1;
	setsockopt(local, SOL_SOCKET, SO_KEEPALIVE, &ivalue, sizeof(ivalue));
	setsockopt(local, IPPROTO_TCP, TCP_NODELAY, &ivalue, sizeof(ivalue));
	setsockopt(remote, SOL_SOCKET, SO_KEEPALIVE, &ivalue, sizeof(ivalue));
	setsockopt(remote, IPPROTO_TCP, TCP_NODELAY, &ivalue, sizeof(ivalue));

	if (local > remote)
		ivalue = local;
	else
		ivalue = remote;
	ivalue += 1;

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(local, &rfds);
		FD_SET(remote, &rfds);
		if (select(ivalue, &rfds, NULL, NULL, NULL) < 1)
			break;
		if (FD_ISSET(local, &rfds)) {
			rlen = recv(local, buffer, sizeof(buffer), 0);
			if (rlen > 0)
				send(remote, buffer, rlen, 0);
			else
				break;
		}
		if (FD_ISSET(remote, &rfds)) {
			rlen = recv(remote, buffer, sizeof(buffer), 0);
			if (rlen > 0)
				send(local, buffer, rlen, 0);
			else
				break;
		}
	}

 _exit_point:
	if (remote != -1)
		close(remote);
	close(local);
	return (void *)0;
}

int main(int argc, char *argv[])
{
	int sock, child, ivalue;
	pthread_t thread;
	struct sockaddr_in bind_addr;

	if (argc < 3 || argc > 4) {
		printf("Usage: <port> <remote_addr> [remote_port]\n");
		return -1;
	}

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(atoi(argv[1]));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = inet_addr(argv[2]);
	if (argc == 3)
		remote_addr.sin_port = htons(atoi(argv[1]));
	else
		remote_addr.sin_port = htons(atoi(argv[3]));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("network ");
		return -1;
	}
	ivalue = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ivalue, sizeof(ivalue));
	if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0 || listen(sock, 5) < 0) {
		perror("network ");
		return -1;
	}

	while (1) {
		child = accept(sock, NULL, NULL);
		if (child != -1) {
			if (pthread_create(&thread, NULL, proxy_thread_routine, (void *)(long)child) != 0)
				close(child);
		}
	}
	return 0;
}

