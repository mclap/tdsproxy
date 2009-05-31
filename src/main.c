#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <event.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include "protocol.h"
#include "conn.h"

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
	srv_cn_state_t state;
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
void print_header(struct tds_header *hdr)
{
	printf("type: 0x%02x\n", (unsigned int)hdr->type);
	printf("len : %d\n", ntohs(hdr->length));
	printf("eof : %d\n", hdr->eof);
	printf("seq : %d\n", hdr->packet_num);
	printf("win : %d\n", hdr->window);
}

void srv_conn_close(srv_cn_t *ctx)
{
	close(ctx->bufev->ev_read.ev_fd);
	bufferevent_free(ctx->bufev);
	free(ctx);
}

void copy_utf16(void *dst, const char *src, int len)
{
	char *p = (char *)dst;

	while (len > 0)
	{
		*p++ = '\0';
		*p++ = *src++;
		len--;
	}
}

void *tds_new_error_token(uint8_t type, uint32_t errorcode, uint8_t state, uint8_t severity, const char *msg, const char *srvname, uint16_t *p_len)
{
	uint8_t *buf = 0, *p;
	uint16_t msglen = msg ? strlen(msg) : 0;
	uint16_t srvnamelen = srvname ? strlen(srvname) : 0;
	uint16_t tokenlen = sizeof(type) + 2 /*length*/
		+ sizeof(errorcode) + sizeof(state) + sizeof(severity)
		+ 1 + msglen*2
		+ 1 + srvnamelen*2
		+ 1 + 0 /* process name */
		+ 1 /* line number */;

	buf = (uint8_t *)malloc(tokenlen);
	if (!buf)
		return buf;

	*p_len = tokenlen;

#define RAW_CP(v)					\
	{ memcpy(p, &v, sizeof(v)); p += sizeof(v); }

	p = buf;
	RAW_CP(type);
	tokenlen = htons(tokenlen);
	RAW_CP(tokenlen);
	RAW_CP(errorcode);
	RAW_CP(state);
	RAW_CP(severity);
	*p++ = msglen; copy_utf16(p, msg, msglen); p += msglen*2;
	*p++ = srvnamelen; copy_utf16(p, srvname, srvnamelen); p += srvnamelen*2;
	*p++ = 0x00;
	*p = 0x00; /* line */

#undef RAW_CP

	return buf;
}

static
void srv_quit_cb(struct bufferevent *ev, void *arg)
{
	srv_cn_t *ctx = (srv_cn_t *)arg;
	TP_DEBUG("connection close is requested (bytes left: %d)", (int)EVBUFFER_LENGTH(ev->output));
	//srv_conn_close(ctx);
}

static
int srv_handle_pkt(srv_cn_t *ctx, const void *ptr)
{
	struct tds_header *hdr = (struct tds_header *)ptr;
	print_header(hdr);

	if (ctx->state == cn_LOGIN)
	{
		if (hdr->type != pkt_LOGIN7)
		{
			/* Cannot handle. Close connection gracefuly
			 * with message
			 */
#if 0
			struct tds_header resp = { 0 };
			uint16_t errbuflen;
			void *errbuf = tds_new_error_token(
				0xAA, 0x10000, 0, 20, "Not supported packet sequence",
				"tdsproxy", &errbuflen);

			uint8_t token_end[] = {
				0xfd, 0x02, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x00, 0x00
			};

			resp.packet_num = 0;
			resp.eof = 1;
			resp.type = pkt_RESPONSE;
			resp.spid = hdr->spid;
			resp.window = 0;
			resp.length = htons(sizeof(resp) + errbuflen + sizeof(token_end));

			bufferevent_disable(ctx->bufev, EV_READ);
			bufferevent_write(ctx->bufev, &resp, sizeof(resp));
			bufferevent_write(ctx->bufev, errbuf, errbuflen);
			bufferevent_write(ctx->bufev, token_end, sizeof(token_end));

			free(errbuf);

			TP_DEBUG_DUMP(EVBUFFER_DATA(ctx->bufev->output), EVBUFFER_LENGTH(ctx->bufev->output),
				      "[%d] error pkt+token of %d bytes", ctx->bufev->ev_read.ev_fd,
				      (int)EVBUFFER_LENGTH(ctx->bufev->output));


			ctx->bufev->writecb = srv_quit_cb;
			ctx->state = cn_QUIT;
#endif
			/* FIXME: close without any message */
			srv_conn_close(ctx);

			return -1;
		}
		/* parse auth data */
	}

	const unsigned char sample_resp[] = {
		0x04,0x01,0x01,0xbe,0x00,0x35,0x01,0x00,0xe3,0x1b,0x00,0x01,
		0x06,0x6d,0x00,0x61,0x00,0x73,0x00,0x74,0x00,0x65,0x00,0x72,
		0x00,0x06,0x6d,0x00,0x61,0x00,0x73,0x00,0x74,0x00,0x65,0x00,
		0x72,0x00,0xab,0x74,0x00,0x45,0x16,0x00,0x00,0x02,0x00,0x25,
		0x00,0x43,0x00,0x68,0x00,0x61,0x00,0x6e,0x00,0x67,0x00,0x65,
		0x00,0x64,0x00,0x20,0x00,0x64,0x00,0x61,0x00,0x74,0x00,0x61,
		0x00,0x62,0x00,0x61,0x00,0x73,0x00,0x65,0x00,0x20,0x00,0x63,
		0x00,0x6f,0x00,0x6e,0x00,0x74,0x00,0x65,0x00,0x78,0x00,0x74,
		0x00,0x20,0x00,0x74,0x00,0x6f,0x00,0x20,0x00,0x27,0x00,0x6d,
		0x00,0x61,0x00,0x73,0x00,0x74,0x00,0x65,0x00,0x72,0x00,0x27,
		0x00,0x2e,0x00,0x0f,0x4f,0x00,0x52,0x00,0x47,0x00,0x41,0x00,
		0x4e,0x00,0x49,0x00,0x5a,0x00,0x41,0x00,0x2d,0x00,0x41,0x00,
		0x34,0x00,0x43,0x00,0x31,0x00,0x37,0x00,0x31,0x00,0x00,0x00,
		0x00,0xe3,0x17,0x00,0x02,0x0a,0x75,0x00,0x73,0x00,0x5f,0x00,
		0x65,0x00,0x6e,0x00,0x67,0x00,0x6c,0x00,0x69,0x00,0x73,0x00,
		0x68,0x00,0x00,0xab,0x78,0x00,0x47,0x16,0x00,0x00,0x01,0x00,
		0x27,0x00,0x43,0x00,0x68,0x00,0x61,0x00,0x6e,0x00,0x67,0x00,
		0x65,0x00,0x64,0x00,0x20,0x00,0x6c,0x00,0x61,0x00,0x6e,0x00,
		0x67,0x00,0x75,0x00,0x61,0x00,0x67,0x00,0x65,0x00,0x20,0x00,
		0x73,0x00,0x65,0x00,0x74,0x00,0x74,0x00,0x69,0x00,0x6e,0x00,
		0x67,0x00,0x20,0x00,0x74,0x00,0x6f,0x00,0x20,0x00,0x75,0x00,
		0x73,0x00,0x5f,0x00,0x65,0x00,0x6e,0x00,0x67,0x00,0x6c,0x00,
		0x69,0x00,0x73,0x00,0x68,0x00,0x2e,0x00,0x0f,0x4f,0x00,0x52,
		0x00,0x47,0x00,0x41,0x00,0x4e,0x00,0x49,0x00,0x5a,0x00,0x41,
		0x00,0x2d,0x00,0x41,0x00,0x34,0x00,0x43,0x00,0x31,0x00,0x37,
		0x00,0x31,0x00,0x00,0x00,0x00,0xe3,0x11,0x00,0x03,0x06,0x63,
		0x00,0x70,0x00,0x31,0x00,0x32,0x00,0x35,0x00,0x31,0x00,0x01,
		0x00,0x00,0xe3,0x0b,0x00,0x05,0x04,0x31,0x00,0x30,0x00,0x34,
		0x00,0x39,0x00,0x00,0xe3,0x0f,0x00,0x06,0x06,0x31,0x00,0x39,
		0x00,0x36,0x00,0x36,0x00,0x30,0x00,0x39,0x00,0x00,0xad,0x36,
		0x00,0x01,0x07,0x00,0x00,0x00,0x16,0x4d,0x00,0x69,0x00,0x63,
		0x00,0x72,0x00,0x6f,0x00,0x73,0x00,0x6f,0x00,0x66,0x00,0x74,
		0x00,0x20,0x00,0x53,0x00,0x51,0x00,0x4c,0x00,0x20,0x00,0x53,
		0x00,0x65,0x00,0x72,0x00,0x76,0x00,0x65,0x00,0x72,0x00,0x00,
		0x00,0x00,0x00,0x08,0x00,0x00,0xc2,0xe3,0x13,0x00,0x04,0x04,
		0x34,0x00,0x30,0x00,0x39,0x00,0x36,0x00,0x04,0x34,0x00,0x30,
		0x00,0x39,0x00,0x36,0x00,0xfd,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00};

	bufferevent_write(ctx->bufev, sample_resp, sizeof(sample_resp));

	return 0;
}

static
void srv_read_cb(struct bufferevent *ev, void *arg)
{
	srv_cn_t *ctx = (srv_cn_t *)arg;
	int len = EVBUFFER_LENGTH(ev->input);

	TP_DEBUG("[%d] in %d bytes", ev->ev_read.ev_fd, len);

	TP_DEBUG_DUMP(EVBUFFER_DATA(ev->input), len,
		"[%d] incoming data of %d bytes", ev->ev_read.ev_fd,
		len);

	char *ptr_orig, *ptr;

	ptr_orig = ptr = EVBUFFER_DATA(ev->input);

	while (len > sizeof(struct tds_header))
	{
		struct tds_header *hdr = (struct tds_header *)ptr;

		if (len < ntohs(hdr->length))
		{
			/* not enough data */
			break;
		}

		if (srv_handle_pkt(ctx, ptr) < 0)
			break;

		len -= ntohs(hdr->length);
		ptr += ntohs(hdr->length);
	}

	if (ptr != ptr_orig)
	{
		TP_DEBUG("draining %d bytes", (int)(ptr-ptr_orig));
		evbuffer_drain(ev->input, ptr-ptr_orig);
	}
}

static
void srv_error_cb(struct bufferevent *ev, short what, void *arg)
{
	srv_cn_t *ctx = (srv_cn_t *)arg;
	TP_DEBUG("[%d] error 0x%04x", ev->ev_read.ev_fd, what);
	srv_conn_close(ctx);
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
	ctx->state = cn_LOGIN;
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
