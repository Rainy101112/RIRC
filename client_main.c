#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUF 512

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum irc_msg_modes {
    MODE_UNKNOWN, MODE_TAGMSG, MODE_PRIVMSG, MODE_NOTICE, MODE_STATUSMSG
};

struct irc_msg {
    char *sender;
    int mode;
    char *channel;
    char *content;
};

char *strndup_safe(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (p) {
        memcpy(p, s, n);
        p[n] = '\0';
    }
    return p;
}

struct irc_msg *irc_msg_parser(const char *raw) {
    if (!raw || strlen(raw) == 0) return NULL;

    struct irc_msg *msg = calloc(1, sizeof(struct irc_msg));
    const char *ptr = raw;
    const char *next_space;

    /* Parse sender */
    if (*ptr == ':') {
        ptr++;
        next_space = strchr(ptr, ' ');
        if (next_space) {
            msg->sender = strndup_safe(ptr, next_space - ptr);
            ptr = next_space + 1;
        }
    }

    /* Parse mode */
    next_space = strchr(ptr, ' ');
    if (next_space) {
        char *cmd = strndup_safe(ptr, next_space - ptr);
        if (strcmp(cmd, "PRIVMSG") == 0) msg->mode = MODE_PRIVMSG;
        else if (strcmp(cmd, "NOTICE") == 0) msg->mode = MODE_NOTICE;
        else if (strcmp(cmd, "STATUSMSG") == 0) msg->mode = MODE_STATUSMSG;
        else if (strcmp(cmd, "TAGMSG") == 0) msg->mode = MODE_TAGMSG;
        else msg->mode = MODE_UNKNOWN;
        free(cmd);
        ptr = next_space + 1;
    }

    /* Parse channel */
    next_space = strchr(ptr, ' ');
    if (next_space) {
        msg->channel = strndup_safe(ptr, next_space - ptr);
        ptr = next_space + 1;
    }

    /* Parse content */
    if (*ptr == ':') ptr++;
    msg->content = strdup(ptr); 
    
    /* Remove \r\n */
    char *newline = strpbrk(msg->content, "\r\n");
    if (newline) *newline = '\0';

    return msg;
}

void free_irc_msg(struct irc_msg *msg) {
    if (!msg) return;
    free(msg->sender);
    free(msg->channel);
    free(msg->content);
    free(msg);
}

void irc_send(int sock, const char *format, ...) {
    char buf[MAX_BUF];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_BUF, format, args);
    va_end(args);

    strcat(buf, "\r\n");
    printf(">> %s", buf);
    send(sock, buf, strlen(buf), 0);

    return;
}

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
