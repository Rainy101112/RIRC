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

#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <irc.h>

#define MAX_EVENTS 64
#define PORT 6667

struct irc_user *clients[MAX_CLIENTS];

void broadcast_message(const char *message, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->registered && i != sender_fd) {
            send(i, message, strlen(message), 0);
        }
    }
}

int is_valid_nick(const char *nick) {
    if (strlen(nick) < 1 || strlen(nick) > 31) return 0;
    if (nick[0] >= '0' && nick[0] <= '9') return 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->registered && strcmp(clients[i]->nick, nick) == 0) {
            return -1;
        }
    }
    return 1;
}

void cleanup_client(int epoll_fd, int fd) {
    if (clients[fd] != NULL) {
        free(clients[fd]);
        clients[fd] = NULL;
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    printf("[*] Disconnected (fd: %d)\n", fd);
}

/* Set socket to nonblocking mode to support epoll */
void set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, opts | O_NONBLOCK);
}

void handle_client_data(int epoll_fd, int fd) {
    struct irc_user *current_user = clients[fd];
    char raw_buf[MAX_BUF];

    int len = recv(fd, raw_buf, sizeof(raw_buf) - 1, 0);
    if (len <= 0) {
        cleanup_client(epoll_fd, fd);
        return;
    }

    strncat(current_user->read_buffer, raw_buf, sizeof(current_user->read_buffer) - current_user->read_pos - 1);

    char *line;
    while ((line = strchr(current_user->read_buffer, '\n')) != NULL) {
        *line = '\0';
        char *cmd = current_user->read_buffer;

        if (strncmp(cmd, "NICK ", 5) == 0) {
            sscanf(cmd + 5, "%31s", current_user->nick);
            if (is_valid_nick(current_user->nick) <= 0) {
                const char *err_msg = "ERROR :Illegal Nickname\r\n";
                send(fd, err_msg, strlen(err_msg), 0);
                printf("[*] Illegal register attempt (fd: %d). Closing.\n", fd);

                cleanup_client(epoll_fd, fd);
            }
        }
        else if (strncmp(cmd, "USER ", 5) == 0) {
            if (current_user->nick[0] != '\0') {
                sscanf(cmd + 5, "%31s", current_user->user);
                current_user->registered = 1;
                printf("[*] Registered NICK %s USER %s as new a user (fd: %d)\n", current_user->nick, current_user->user, fd);

                char welcome[256];
                snprintf(welcome, sizeof(welcome), ":server 001 %s :Welcome to the IRC Network!\r\n", current_user->nick);
                send(fd, welcome, strlen(welcome), 0);
            }
            else {
                const char *err_msg = "ERROR :Nickname must be set before USER\r\n";
                send(fd, err_msg, strlen(err_msg), 0);

                printf("[*] Illegal register attempt (fd: %d). Closing.\n", fd);

                cleanup_client(epoll_fd, fd);
            }
        }
        else if (strncmp(cmd, "PRIVMSG ", 8) == 0 && current_user->registered) {
            char out_msg[1024];
            snprintf(out_msg, sizeof(out_msg), ":%s PRIVMSG #global :%s\r\n", 
                                 current_user->nick, strchr(cmd + 8, ':') + 1);

            broadcast_message(out_msg, fd);
        }

        int remaining = strlen(line + 1);
        memmove(current_user->read_buffer, line + 1, remaining + 1);
    }
}

int main() {
    memset(clients, 0, sizeof(clients));

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

    printf("[*] IRC Server started on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
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
                printf("[*] New connection (fd: %d)\n", conn_sock);
            } else {
                int fd = events[n].data.fd;

                handle_client_data(epoll_fd, fd);
            }
        }
    }
    return 0;
}
