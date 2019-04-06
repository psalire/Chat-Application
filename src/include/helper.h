/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/


#ifndef _HELPER_H_
#define _HELPER_H_

#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <regex>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include "tcpsocket.h"

enum MessageType {
    CLOSED = 0, /* Server closed connection */
    MSG, /* MSG for CHAT */
    MSG_MULTIPLE, /* MSGs length >4096 will be chunked (TODO) */
    ADD_USER, /* Request to add user to server list */
    RM_USER, /* Request to remove user from server list */
    USER_INFO, /* Request for connection info on a user */
    LIST, /* Request for list of waiting users */
    SUCCESS,
    FAIL
};

/* Chat message structure */
typedef struct message {
    char content[4096], /* Message content */
         username[128]; /* Who the message is from */
    uint8_t type; /* Uses MessageType */
    uint16_t seq; /* Used for MSG_MULTIPLE */
} Message;

extern uint8_t option_verbose;
extern uint8_t volatile flag_signal;

class Messaging {
    public:
        /* int sendBytes(TCPSocket sock, const char *msg, uint32_t tot_bytes);
        int recvBytes(TCPSocket sock, uint32_t tot_bytes, std::string &str);
        int recvBytes(TCPSocket sock, std::string &str); */
        Message buildMsg(MessageType type, std::string username, std::string content);
        int sendMessage(TCPSocket sock, Message *msg);
        int recvMessage(TCPSocket sock, std::string &str, Message &msg);

        int bindSocket(TCPSocket tcpsocket);
        int listenSocket(TCPSocket selfsock, int num);
        TCPSocket acceptSocket(TCPSocket selfsock);
};

void getOption_server(int, char **);
void getOption_client(int, char **);
void initializeSigHandler();
std::string getPublicIPAddr();

static inline void verbose(std::string msg) {
    if (option_verbose) {
        std::clog << "stdlog: " << msg << "\n";
    }
}
static inline void error(std::string msg) {
    std::cerr << "stderr: " << msg << "\n";
}

#endif
