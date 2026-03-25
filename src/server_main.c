#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <irc.h>

#define MAX_EVENTS 64
#define MAX_CLIENTS 4096
#define PORT 6667

struct irc_user *clients[MAX_CLIENTS];

void broadcast_message(const char *message, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->registered && i != sender_fd) {
            send(i, message, strlen(message), 0);
        }
    }
}

/* Set socket to nonblocking mode to support epoll */
void set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, opts | O_NONBLOCK);
}

int main() {
    int listen_sock, epoll_fd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];

    /* Create listening socket */
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_sock, 10);
    set_nonblocking(listen_sock);

    /* Create epoll */
    epoll_fd = epoll_create1(0);
    ev.events = EPOLLIN; /* Read event */
    ev.data.fd = listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev);

    printf("IRC Server started on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_sock) {
                if (events[n].data.fd == listen_sock) {
                    /* Accept new client */
                    int conn_sock = accept(listen_sock, NULL, NULL);
                    set_nonblocking(conn_sock);

                    struct irc_user *new_user = calloc(1, sizeof(struct irc_user));
                    new_user->fd = conn_sock;
                    clients[conn_sock] = new_user;

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = conn_sock;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev);
                    printf("New connection (fd: %d)\n", conn_sock);
                }
            } else {
                int fd = events[n].data.fd;
                char buf[512];
                int len = recv(fd, buf, sizeof(buf) - 1, 0);

                if (len <= 0) {
                    /* Clean disconnected clients */
                    free(clients[fd]);
                    clients[fd] = NULL;
                    close(fd);
                    printf("Disconnected (fd: %d)\n", fd);
                } else {
                    buf[len] = '\0';
                    struct irc_user *current_user = clients[fd];

                    if (strncmp(buf, "NICK ", 5) == 0) {
                        sscanf(buf + 5, "%s", current_user->nick);
                    }
                    else if (strncmp(buf, "USER ", 5) == 0) {
                        if (strlen(current_user->nick) != 0) {
                            sscanf(buf + 5, "%s", current_user->user);
                            printf("Registered NICK %s USER %s as new a user (fd: %d)\n", current_user->nick, current_user->user, fd);
                        }
                        else {
                            send(fd, "Error: Illegal register!!!", 27, 0);
                            free(clients[fd]);
                            clients[fd] = NULL;
                            close(fd);
                            printf("Disconnected (fd: %d) caused by illegal register\n", fd);
                        }
                    }
                    else if (strncmp(buf, "PRIVMSG ", 8) == 0 && current_user->registered) {
                        char out_msg[1024];
                        snprintf(out_msg, sizeof(out_msg), ":%s PRIVMSG #global :%s\r\n", 
                                 current_user->nick, strchr(buf + 8, ':') + 1);

                        broadcast_message(out_msg, fd);
                    }
                }
            }
        }
    }
    return 0;
}
