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
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	int ready;
	fd_set fds;
	struct timeval timeout;
	int sock, ivalue = 1;
	int local, remote;
	struct sockaddr_in bind_addr;
	struct sockaddr_in remote_addr;
	struct proxy_stat status;
	struct proxy_engine *engine;

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

	sock = socket(bind_addr.sin_family, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("network ");
		return -1;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ivalue, sizeof(ivalue));
	if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0 || listen(sock, 5) < 0) {
		perror("network ");
		return -1;
	}

	if (proxy_create(&engine) < 0) {
		perror("proxy");
		return -1;
	}
	
	signal(SIGPIPE, SIG_IGN);

	while (1) {
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		ready = select(sock + 1, &fds, NULL, NULL, &timeout);
		if (ready < 0) {
			perror("select");
			return -1;
		}
		if (ready > 0 && FD_ISSET(sock, &fds)) {
			local = accept(sock, NULL, NULL);
			if (local != -1) {
				remote = socket(remote_addr.sin_family, SOCK_STREAM, 0);
				if (remote != -1
				    && connect(remote, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == 0) {
					setsockopt(local, SOL_SOCKET, SO_KEEPALIVE, &ivalue, sizeof(ivalue));
					setsockopt(local, IPPROTO_TCP, TCP_NODELAY, &ivalue, sizeof(ivalue));
					setsockopt(remote, SOL_SOCKET, SO_KEEPALIVE, &ivalue, sizeof(ivalue));
					setsockopt(remote, IPPROTO_TCP, TCP_NODELAY, &ivalue, sizeof(ivalue));
					if (proxy_addpair(engine, local, remote) == 0) {
						local = -1;
						remote = -1;
					}
				}
				if (remote != -1)
					close(remote);
				if (local != -1)
					close(local);
			}
		}
		if (proxy_status(engine, &status) == 0)
			printf("pair=%lu, in=%llu/%llu, out=%llu/%llu\n",
			       status.pairs, status.in_packets, status.in_bytes, status.out_packets, status.out_bytes);
	}
	return 0;
}

