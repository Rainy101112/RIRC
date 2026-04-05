#ifndef INCLUDE_IRC_H_
#define INCLUDE_IRC_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#define MAX_BUF 512
#define MAX_CLIENTS 4096

#define FEATURE_SASL

enum irc_msg_modes {
    MODE_UNKNOWN, MODE_TAGMSG, MODE_PRIVMSG, MODE_NOTICE, MODE_STATUSMSG
};

struct irc_msg {
    char *sender;
    int mode;
    char *channel;
    char *content;
};

struct irc_user {
    int fd;

    char nick[32];
    char user[32];
    int registered;

    int cap_negotiating;
    int sasl_active;
    int authenticated;

    char read_buffer[MAX_BUF * 2];
    int read_pos;
};

struct irc_channel {
    char name[32];
    char password[32];

    struct irc_user *users[MAX_CLIENTS];
};

struct irc_msg *irc_msg_parser(const char *raw);
void free_irc_msg(struct irc_msg *msg);
void irc_send(int sock, const char *format, ...);

#endif // INCLUDE_IRC_H_
