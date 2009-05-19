#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <event.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define TP_DEBUG(fmt, ...) do {					\
		struct timeval tv;				\
		gettimeofday(&tv, 0);				\
		fprintf(stderr, "[%d.%09d] ", (int)tv.tv_sec, (int)tv.tv_usec);	\
		fprintf(stderr, fmt, ## __VA_ARGS__);	\
		fprintf(stderr, "\n");				\
	} while (0)

#define TP_DEBUG_DUMP(p, s, fmt, ...) do {			\
		const void *ptr = (p); int sz = (s);		\
		struct timeval tv;				\
		gettimeofday(&tv, 0);				\
		fprintf(stderr, "[%d.%09d] ", (int)tv.tv_sec, (int)tv.tv_usec);	\
		fprintf(stderr, fmt, ## __VA_ARGS__);	\
		fprintf(stderr, "\n");				\
		dump_mem(ptr,sz);	\
	} while (0)

inline
unsigned char toprint(unsigned char c)
{
	if (c < ' ' || c >= 0x7f)
		return '.';

	return c;
}

void dump_mem(const void *ptr, int size)
{
	const unsigned char *p = (const unsigned char *)ptr;
	int i;

	for (i = 0; i < size; )
	{
		char buf[0x81];
		char *writep = buf;

		int cell = 0;
		writep += sprintf(writep, "\t%04x: ", i);
		for (cell = 0; cell < 16; cell++)
		{
			if (i + cell < size)
				writep += sprintf(writep, "%02x", p[cell]);
			else
				writep += sprintf(writep, "  ");
				
			if (cell > 0 && (cell & 3)==0)
			{
				*writep++ = ' '; *writep = 0;
			}
		}

		*writep++ = ' '; *writep = 0;

		for (cell = 0; cell < 16; cell++)
		{
			if (i + cell < size)
				writep += sprintf(writep, "%c", toprint(p[cell]));
			else
				writep += sprintf(writep, "  ");
		}
		fprintf(stderr, "%s\n", buf);
		i+= 16;
		p+= 16;
	}
}

typedef struct
{
	struct bufferevent *bufev;
} srv_cn_t;

typedef struct
{
	struct bufferevent bufev;
} cli_cn_t;

//int socket(int domain, int type, int protocol);

static
int net_socket(in_addr_t iaIP, in_port_t ipPort, struct sockaddr_in *srv)
{
	int on = 1;

	memset(srv, 0, sizeof(*srv));
	srv->sin_family = AF_INET;
	srv->sin_port = htons(ipPort);
	srv->sin_addr.s_addr = htonl(iaIP);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock >= 0)
	{
		evutil_make_socket_nonblocking(sock);
	}

#ifdef SO_REUSEPORT
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
#ifdef SO_REUSEADDR
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
#endif

       int setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen);


	return sock;
}

static
int net_listen_at(in_addr_t iaIP, in_port_t ipPort, int backlog)
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
void srv_read_cb(struct bufferevent *ev, void *arg)
{
	srv_cn_t *ctx = (srv_cn_t *)arg;
	TP_DEBUG("[%d] in %d bytes", ev->ev_read.ev_fd,
		(int)EVBUFFER_LENGTH(ev->input));

	TP_DEBUG_DUMP(EVBUFFER_DATA(ev->input), EVBUFFER_LENGTH(ev->input),
		"[%d] incoming data of %d bytes", ev->ev_read.ev_fd,
		(int)EVBUFFER_LENGTH(ev->input));
}

static
void srv_error_cb(struct bufferevent *ev, short what, void *arg)
{
	TP_DEBUG("[%d] error 0x%04x", ev->ev_read.ev_fd, what);
}

static
void srv_accept_cb(int fd, short event, void *arg)
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

	int newsock = accept(fd, (struct sockaddr *)&sa, &sa_len);
	if (newsock < 0)
	{
		TP_DEBUG("accept failed: %d", errno);
		return;
	}
	evutil_make_socket_nonblocking(newsock);
	TP_DEBUG("new connection: %d", newsock);

	ctx->bufev = bufferevent_new(newsock, srv_read_cb, 0, srv_error_cb, ctx);
	assert(ctx->bufev);
	bufferevent_enable(ctx->bufev, EV_READ);
}

int main()
{
	struct event evmain;
	int listenfd = -1;

	setvbuf(stderr, 0, _IONBF, 0);

	signal(SIGPIPE, SIG_IGN);
	event_init();

	listenfd = net_listen_at(INADDR_ANY, 1433, 4096);
	if (listenfd < 0)
	{
		TP_DEBUG("listen socket creation error: %d", errno);
		return -1;
	}

	event_set(&evmain, listenfd, EV_READ | EV_PERSIST, srv_accept_cb, 0);
	event_add(&evmain, NULL);

	event_dispatch();

	return 0;
}
