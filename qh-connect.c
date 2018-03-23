#define _GNU_SOURCE 1
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>

#define NSOCK_EBIND    (-1)     /**< failed to bind() */
#define NSOCK_ELISTEN  (-2)     /**< failed to listen() */
#define NSOCK_ESOCKET  (-3)     /**< failed to socket() */
#define NSOCK_EUNLINK  (-4)     /**< failed to unlink() */
#define NSOCK_ECONNECT (-5)     /**< failed to connect() */
#define NSOCK_EFCNTL   (-6)     /**< failed to fcntl() */
#define NSOCK_EINVAL (-EINVAL) /**< -22, normally */

/* flags for the various create calls */
#define NSOCK_TCP     (1 << 0)  /**< use tcp mode */
#define NSOCK_UDP     (1 << 1)  /**< use udp mode */
#define NSOCK_UNLINK  (1 << 2)  /**< unlink existing path (only nsock_unix) */
#define NSOCK_REUSE   (1 << 2)  /**< reuse existing address */
#define NSOCK_CONNECT (1 << 3)  /**< connect rather than create */
#define NSOCK_BLOCK   (1 << 4)  /**< socket should be in blocking mode */

#ifndef offsetof
# define offsetof(t, f) ((unsigned long)&((t *)0)->f)
#endif

static int nsock_unix(const char *path, unsigned int flags)
{
	struct sockaddr *sa;
	int sock = 0, mode;
	socklen_t slen;
	struct sockaddr_un saun;;

	if (!path)
		return NSOCK_EINVAL;

	if (flags & NSOCK_TCP)
		mode = SOCK_STREAM;
	else if (flags & NSOCK_UDP)
		mode = SOCK_DGRAM;
	else
		return NSOCK_EINVAL;

	if ((sock = socket(AF_UNIX, mode, 0)) < 0) {
		return NSOCK_ESOCKET;
	}

	/* set up the sockaddr_un struct and the socklen_t */
	sa = (struct sockaddr *)&saun;
	memset(&saun, 0, sizeof(saun));
	saun.sun_family = AF_UNIX;
	slen = strlen(path);
	memcpy(&saun.sun_path, path, slen);
	slen += offsetof(struct sockaddr_un, sun_path);

	/* unlink if we're supposed to, but not if we're connecting */
	if (flags & NSOCK_UNLINK && !(flags & NSOCK_CONNECT)) {
		if (unlink(path) < 0 && errno != ENOENT)
			return NSOCK_EUNLINK;
	}

	if (flags & NSOCK_CONNECT) {
		if (connect(sock, sa, slen) < 0) {
			close(sock);
			return NSOCK_ECONNECT;
		}
		return sock;
	} else {
		if (bind(sock, sa, slen) < 0) {
			close(sock);
			return NSOCK_EBIND;
		}
	}

	if (!(flags & NSOCK_BLOCK) && fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
		close(sock);
		return NSOCK_EFCNTL;
	}

	if (flags & NSOCK_UDP)
		return sock;

	if (listen(sock, 3) < 0) {
		close(sock);
		return NSOCK_ELISTEN;
	}

	return sock;
}
int main(int argc, char **argv)
{
	const char *spath;
	char *buf;
	int sock;
	int pos = 0, i, buflen = 0;
	int ret;

	if (argc < 3) {
		printf("Need path to socket to connect to as first argument (and stuff to send as the rest)\n");
		exit(EXIT_FAILURE);
	}
	spath = argv[1];
	for (i = 2; i < argc; i++) {
		buflen += strlen(argv[i]) + 1;
	}
	buf = calloc(1, buflen + 1);
	for (pos = 0, i = 2; i < argc; i++) {
		int arglen = strlen(argv[i]);
		memcpy(buf + pos, argv[i], arglen);
		pos += arglen;
		if (i < (argc - 1)) {
			buf[pos++] = ' ';
		}
	}

	printf("Buf to send: %s\n", buf);

	sock = nsock_unix(spath, NSOCK_CONNECT | NSOCK_TCP);
	if (sock < 0) {
		printf("Failed to connect to %s: %s\n", spath, strerror(errno));
	}

	ret = write(sock, buf, pos + 1);
	for (;;) {
		char inbuf[4096];
		ret = read(sock, inbuf, sizeof(inbuf));
		if (ret < 0)
			break;
		if (write(fileno(stdout), inbuf, ret) < 0)
			break;
	}

	close(sock);
	return 0;
}
