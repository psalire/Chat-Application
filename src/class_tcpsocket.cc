/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/

#include "tcpsocket.h"
#include "helper.h"

/* Constructors */
TCPSocket::TCPSocket() {
    clearSocket();
}
TCPSocket::TCPSocket(int port, int options, std::string ipAddr) {
    setSocket(AF_INET, SOCK_STREAM, options);
    setSockaddr_in(AF_INET, port, ipAddr);
}

/* Setters */
int TCPSocket::setSocket(int type, int protocol, int option) {
    sockfd = socket(type, protocol, option);
    if (sockfd < 0) {
        std::cerr << "stderr: Error: socket() fail: " << strerror(errno) << "\n";
    }
    return sockfd;
}
struct sockaddr_in TCPSocket::setSockaddr_in(int family, int port, std::string server_ip) {
    sockaddr_server.sin_family = family;
    sockaddr_server.sin_port = htons(port);
    if (server_ip.compare("INADDR_ANY")) {
        if (server_ip.compare("localhost") == 0) {
            sockaddr_server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        }
        else {
            inet_aton(server_ip.c_str(), &sockaddr_server.sin_addr);
        }
    }
    else {
        sockaddr_server.sin_addr.s_addr = INADDR_ANY;
    }
    return sockaddr_server;
}

void TCPSocket::setSocket(const int &user_socket) {
    sockfd = user_socket;
}
void TCPSocket::setSockaddr_in(const struct sockaddr_in &user_sockaddr) {
    sockaddr_server = user_sockaddr;
}

void TCPSocket::closeSocket() {
    if (sockfd < 0) {
        return;
    }
    if (shutdown(sockfd, SHUT_WR)) {
        verbose("stderr: Error: shutdown() fail: " + std::string(strerror(errno)) + "\n");
    }
    if (close(sockfd) < 0) {
        verbose("stderr: Error: close() fail: " + std::string(strerror(errno)) + "\n");
    }
    clearSocket();
    usleep(20000);
}
void TCPSocket::clearSocket() {
    sockfd = -1;
    memset(&sockaddr_server, 0, sizeof(sockaddr_server));
}

bool TCPSocket::operator==(const TCPSocket &sock) {
    if (sock.getSockfd() == getSockfd()) {
        return true;
    }
    return false;
}
