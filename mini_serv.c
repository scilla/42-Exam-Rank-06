#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

void fatal() {
	write(2, "Fatal error\n", 12);
	exit(1);
}

int sockfd = -1;
fd_set cur, wr, rd;
int gid = 0;

typedef struct s_cl {
	int id;
	int fd;
	int newmess;
	struct s_cl* next;
} t_cl;

t_cl* cls = 0;
char buff[64];

int max_fd() {
	int res = sockfd;
	for (t_cl* x = cls; x; x = x->next)
		res = x->fd > res ? x->fd : res;
	return (res);
}

void send_all(char* str, int len, int skip_fd) {
	for (t_cl* x = cls; x; x = x->next) {
		if (x->fd != skip_fd && FD_ISSET(x->fd, &wr)) {
			if (send(x->fd, str, len, 0) < 0) fatal();
		}
	}
}


void new_cl(int fd) {
	t_cl* nc = calloc(sizeof(t_cl), 1);
	t_cl* tmp = cls;

	nc->id = gid++;
	nc->fd = fd;
	nc->newmess = 1;

	if (!tmp) {
		cls = nc;
		return (nc->id);
	}
	while (tmp->next) tmp = tmp->next;
	tmp->next = nc;
	return (nc->id);
}

void accept_client() {
	int fd = accept(sockfd, 0, 0);
	if (fd < 0) fatal();
	FD_SET(fd, &cur);
	sprintf(buff, "server: client %d just arrived\n", new_cl(fd));
	send_all(buff, strlen(buff), fd);
}

void close_client(int fd) {
	t_cl* tmp = cls;
	for (t_cl* x = cls; x; x = x->next) {
		if (x->fd != fd) {
			tmp = x;
			continue;
		}
		if (close(fd) < 0) fatal();
		FD_CLR(fd, &cur);
		sprintf(buff, "server: client %d just left\n", x->id);
		send_all(buff, strlen(buff), x->fd);
		if (cls == x)
			cls = cls->next;
		else
			tmp->next = x->next;
		free(x);
		break;
	}
}