/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/

#include "helper.h"

uint8_t option_verbose = 0x0;
uint8_t volatile flag_signal = 0x0;

Message Messaging::buildMsg(MessageType type, std::string username, std::string content) {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    strncpy(msg.content, content.c_str(), 4096);
    strncpy(msg.username, username.c_str(), 128);
    msg.type = type;
    return msg;
}

int Messaging::sendMessage(TCPSocket sock, Message *msg) {
    uint32_t sentBytes = 0;
    int sent;
    do {
        sent = send(sock.getSockfd(), &((uint8_t *) msg)[sentBytes], sizeof(Message) - sentBytes, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EPIPE) {
                return CLOSED;
            }
            verbose("send() fail: " + std::string(strerror(errno)));
            return sent;
        }
        sentBytes += sent;
    } while (sentBytes < sizeof(Message));
    usleep(2500);
    return sent;
}

int Messaging::recvMessage(TCPSocket sock, std::string &str, Message &msg) {
    memset(&msg, 0, sizeof(msg));
    str.clear();
    int read;
    uint32_t readBytes = 0;
    do {
        if ((read = recv(sock.getSockfd(), &(((uint8_t *)&msg)[readBytes]), sizeof(Message) - readBytes, 0)) < 0) {
            error("recv() fail: " + std::string(strerror(errno)));
            return read;
        }
        if (read == 0) {
            verbose("Connection is closed, read 0 bytes.");
            return CLOSED;
        }
        readBytes += read;
    } while (readBytes < sizeof(Message));
    str = msg.content;
    return msg.type;
}

int Messaging::bindSocket(TCPSocket tcpsocket) {
    struct sockaddr_in sockaddr_client = tcpsocket.getSockaddr();
    int ret = bind(tcpsocket.getSockfd(), (struct sockaddr *) &sockaddr_client, sizeof(sockaddr_client));
    if (ret < 0) {
        verbose("bind() fail: " + std::string(strerror(errno)));
	}
    // server_tcpsocket = tcpsocket;
    usleep(2500);
    return ret;
}

int Messaging::listenSocket(TCPSocket selfsock, int num) {
    int ret = listen(selfsock.getSockfd(), num);
    if (ret < 0) {
        error("listen() fail: " + std::string(strerror(errno)));
        return ret;
	}
    usleep(2500);
    return ret;
}

TCPSocket Messaging::acceptSocket(TCPSocket selfsock) {
    struct sockaddr_in client_addr;
    socklen_t len;
    memset(&client_addr, 0, sizeof(client_addr));
    memset(&len, 0, sizeof(len));
    usleep(2500);
    int ret = accept(selfsock.getSockfd(), (struct sockaddr *) &client_addr, &len);
	if (ret < 0) {
        if (errno != EINTR) {
            error("accept() fail: " + std::string(strerror(errno)));
        }
        return TCPSocket(-1, client_addr);
	}
    return TCPSocket(ret, client_addr);
}

/* Get verbose option */
void getOption_server(int argc, char **argv) {
    const char *usage_err = "stderr: Error: usage: server [port] [-v (optional verbose)]\n";
    if (argc < 2 || argc > 3) {
        std::cerr << usage_err;
        exit(0);
    }
    int c;
    while ((c = getopt(argc, argv, "v")) != -1) {
        if (c == 'v') {
            option_verbose = 0x1;
            verbose("Verbose on.");
        }
        else {
            error("Invalid option, exiting");
            exit(0);
        }
    }
    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        std::cerr << usage_err;
        exit(0);
    }
}
void getOption_client(int argc, char **argv) {
    const char *usage_err = "stderr: Error: usage: client [ip address] [port] [username] [-v (optional verbose)]\n";
    if (argc < 4 || argc > 5) {
        std::cerr << usage_err;
        exit(0);
    }
    int c;
    while ((c = getopt(argc, argv, "v")) != -1) {
        if (c == 'v') {
            option_verbose = 0x1;
            verbose("Verbose on.");
        }
        else {
            error("Invalid option, exiting");
            exit(0);
        }
    }
    if (argc == 4 && strcmp(argv[1], "-v") == 0) {
        std::cerr << usage_err;
        exit(0);
    }
}

std::string getPublicIPAddr() {
    struct ifaddrs *addrs;
    getifaddrs(&addrs);

    char host[NI_MAXHOST];
    memset(host, 0, NI_MAXHOST);

    const std::regex regex_local_ip("^((127)|(10)|(192)|(172))\\.");
    std::cmatch regex_result;

    std::string ret = "127.0.0.1";
    for (struct ifaddrs *temp = addrs; temp != NULL; temp = temp->ifa_next) {
        /* Only want ipv4 interfaces */
        if (temp->ifa_addr == NULL || temp->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        /* Get IP addr to string */
        int s;
        if ((s = getnameinfo(temp->ifa_addr, sizeof(struct sockaddr_in),
             host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST)) != 0) {
            verbose("stdlog: getnameinfo() failed: " + std::string(gai_strerror(s)));
            break;
        }

        /* Check if local ip addr or not */
        if (std::regex_search(host, regex_result, regex_local_ip)) {
            verbose("stdlog: This ip is local: " + std::string(host));
        }
        else {
            ret = host;
            break;
        }
    }
    freeifaddrs(addrs);

    verbose("stdlog: Got public IP addr: " + ret);
    return ret;
}
