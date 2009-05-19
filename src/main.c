#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <event.h>
#include <string.h>
#include <stdio.h>

#define TP_DEBUG(fmt,...) fprintf(stderr, fmt, ##__VA_ARGS__)

typedef struct
{
	struct bufferevent bufev;
} srv_cn_t;

typedef struct
{
	struct bufferevent bufev;
} cli_cn_t;

//int socket(int domain, int type, int protocol);

static
int net_socket(in_addr_t iaIP, in_port_t ipPort, struct sockaddr_in *srv)
{
	memset(srv, 0, sizeof(*srv));
	srv->sin_family = AF_INET;
	srv->sin_port = htons(ipPort);
	srv->sin_addr.s_addr = htonl(iaIP);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock >= 0)
	{
		evutil_make_socket_nonblocking(sock);
	}

	return sock;
}

static
int net_listen_at(in_addr_t iaIP, in_port ipPort, int backlog)
{
	struct sockaddr_in sa;
	int sock = net_socket(iaIP, ipPort, &sa);
	if (sock < 0)
		return sock;

	if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		close(sock);
		return -1;
	}
	if (listen(sock, backlog) < 0)
	{
		close(sock);
		return -1;
	}

	return sock;
}

static
void srv_accept_cb(int fd, short event, void *)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	socklen_t sa_len = sizeof(sa);

	srv_cn_t *ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
	{
		TP_DEBUG("calloc failed: %d", errno);
		return;
	}

	int newsock = accept(fd, &sa, &sa_len);
	if (newsock < 0)
	{
		TP_DEBUG("accept failed: %d", errno);
		return;
	}


}

int main()
{
	struct event evmain;
	int listenfd = net_listen_at(INADDR_ANY, 1433);

	event_init();
	event_set(&evmain, listenfd, EV_READ, srv_accept, 0, 0);

	event_dispatch();

	return 0;
}
