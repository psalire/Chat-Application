/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/

#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H_

#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Class for handling necessary initializations for TCP connections e.g. sockets and sockaddrs */
class TCPSocket {
    int sockfd;
    struct sockaddr_in sockaddr_server;

    public:
        TCPSocket();
        TCPSocket(int port, int options, std::string ipAddr);
        TCPSocket(const int &user_sockfd, const struct sockaddr_in &user_sockaddr) :
                    sockfd(user_sockfd), sockaddr_server(user_sockaddr) {};
                    
        int getSockfd() const {return sockfd;}
        const struct sockaddr_in &getSockaddr() {return sockaddr_server;}

        int setSocket(int type, int protocol, int option);
        void setSocket(const int &user_sockfd);

        struct sockaddr_in setSockaddr_in(int family, int port, std::string server_ip);
        void setSockaddr_in(const struct sockaddr_in &sockaddr_server);

        void closeSocket();
        void clearSocket();
        
        bool operator==(const TCPSocket &sock);
};

#endif
