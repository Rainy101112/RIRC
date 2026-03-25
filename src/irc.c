#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <irc.h>

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
            msg->sender = strndup(ptr, next_space - ptr);
            ptr = next_space + 1;
        }
    }

    /* Parse mode */
    next_space = strchr(ptr, ' ');
    if (next_space) {
        char *cmd = strndup(ptr, next_space - ptr);
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
        msg->channel = strndup(ptr, next_space - ptr);
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
