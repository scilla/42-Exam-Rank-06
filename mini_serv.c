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


int new_cl(int fd) {
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

int main(int argc, char** argv) {
	int res;
	struct sockaddr_in servaddr;
	char c;

	if (argc != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) fatal();
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		fatal();
	}
	if (listen(sockfd, 0) < 0) {
		fatal();
	}
	FD_ZERO(&cur);
	FD_SET(sockfd, &cur);
	while (1) {
		wr = rd = cur;
		if (select(max_fd() + 1, &rd, &wr, 0, 0) < 0) fatal();
		if (FD_ISSET(sockfd, &rd)) {
			accept_client();
			continue;
		}
		for (t_cl* x = cls; x; x = x->next) {
			if (!(FD_ISSET(x->fd, &rd))) continue;
			res = recv(x->fd, &c, 1, 0);
			if (res < 0) fatal();
			if (!res) {
				close_client(x->fd);
				break;
			}
			if (x->newmess) {
				x->newmess = 0;
				sprintf(buff, "client %d: ", x->id);
				send_all(buff, strlen(buff), x->fd);
			}
			if (c == '\n') x->newmess = 1;
			send_all(&c, 1, x->fd);
			break;
		}
	}
	return (0);
}











