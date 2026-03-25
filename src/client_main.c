#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "irc.h"

int main(int argc, char *argv[]) {
    int sock = 0;
    struct addrinfo hints, *res;
    char buffer[MAX_BUF];

    const char *server = argv[1];
    const char *port = "6667";
    const char *nick = "nickname_001";
    const char *user = "user_001";
    const char *realname = "User the user";

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server, port, &hints, &res) != 0){
        perror("getaddrinfo");
        return 1;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        perror("connect");
        return 1;
    }
    freeaddrinfo(res);
    printf("Connected to %s:%s\n", server, port);

    // Register
    irc_send(sock, "NICK %s", nick);
    irc_send(sock, "USER %s 0 * :%s", user, realname);

    while (1) {
        ssize_t len = recv(sock, buffer, MAX_BUF - 1, 0);
        if (len <= 0) break;

        buffer[len] = '\0';
        printf("<< %s", buffer);

        /* Response server PING
           Format: PING :servername */
        if (strncmp(buffer, "PING", 4) == 0) {
            buffer[1] = 'O';
            irc_send(sock, "PONG%s", buffer + 4); 
        }
    }

    close(sock);
    return 0;
}
