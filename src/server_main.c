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

#define MAX_EVENTS 64
#define PORT 6667

// 将 socket 设置为非阻塞模式（epoll 的标配）
void set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, opts | O_NONBLOCK);
}

int main() {
    int listen_sock, epoll_fd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];

    // 1. 创建监听 Socket
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_sock, 10);
    set_nonblocking(listen_sock);

    // 2. 初始化 epoll
    epoll_fd = epoll_create1(0);
    ev.events = EPOLLIN; // 监听读事件
    ev.data.fd = listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev);

    printf("IRC Server started on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_sock) {
                // 有新客户端连接
                int conn_sock = accept(listen_sock, NULL, NULL);
                set_nonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
                ev.data.fd = conn_sock;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev);
                printf("New connection (fd: %d)\n", conn_sock);
            } else {
                // 现有客户端发来了数据
                char buf[512];
                int fd = events[n].data.fd;
                int len = recv(fd, buf, sizeof(buf) - 1, 0);
                if (len <= 0) {
                    printf("Client fd:%d disconnected.\n", fd);
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    buf[len] = '\0';
                    printf("Recv from fd:%d: %s", fd, buf);
                    
                    // 这里编写你的解析逻辑：
                    // if (strncmp(buf, "NICK", 4) == 0) { 保存昵称到结构体... }
                    // if (strncmp(buf, "PRIVMSG", 7) == 0) { 转发给其他人... }
                }
            }
        }
    }
    return 0;
}
