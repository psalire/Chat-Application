#include "server.h"

Server global_server;
std::vector<TCPSocket> global_client_socks;

Server::Server(TCPSocket tcpsocket) {
    /* Reuse server socket */
    int reuse = 1;
    if (setsockopt(tcpsocket.getSockfd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        error("setsockopt(SO_REUSEADDR) fail: " + std::string(strerror(errno)));
    }
    /* #ifdef SO_REUSEPORT
    if (setsockopt(tcpsocket.getSockfd(), SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        error("setsockopt(SO_REUSEPORT) fail: " + std::string(strerror(errno)));
    }
    #endif */
    mutex = new std::mutex();
    bindServer(tcpsocket);
}

int Server::addUser(const char user_name[128], std::string ip_addr, int port) {
    mutex->lock();
    if (active_users.find(std::string(user_name)) != active_users.end()) {
        return -1;
    }
    active_users.insert(std::pair<std::string,std::pair<std::string,int>>(user_name, std::make_pair(ip_addr, port)));
    mutex->unlock();
    verbose("Added user: " + std::string(user_name) + "," + std::to_string(port) + "," + ip_addr);
    return 1;
}
void Server::removeUser(const char user_name[128]) {
    mutex->lock();
    std::map<std::string,std::pair<std::string,int>>::iterator it;
    if ((it = active_users.find(std::string(user_name))) != active_users.end()) {
        active_users.erase(it);
        verbose("Removed user: " + std::string(user_name));
    }
    mutex->unlock();
}

Message Server::buildMessage(MessageType type, std::string content) {
    return buildMsg(type, "_server_", content);
}

int Server::bindServer(TCPSocket tcpsocket) {
    int ret;
    if ((ret = bindSocket(tcpsocket)) < 0) {
        server_tcpsocket.setSocket(-1);
    }
    else {
        server_tcpsocket = tcpsocket;
    }
    return ret;
}
int Server::listenServer(int num) {
    return listenSocket(server_tcpsocket, num);
}
TCPSocket Server::acceptClient() {
    return acceptSocket(server_tcpsocket);
}
void Server::sendUserInfo(TCPSocket sock, const char user_name[128]) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    std::map<std::string,std::pair<std::string,int>>::iterator it;
    std::string ipaddr;
    int port = -1;
    std::pair<std::string,int> elem;
    mutex->lock();
    try {
        elem = active_users.at(std::string(user_name));
        ipaddr = elem.first;
        port = elem.second;
    }
    catch (const std::out_of_range &e) {
        verbose("Error: failed to find " + std::string(user_name));
    }
    mutex->unlock();
    msg = buildMessage(USER_INFO, ipaddr + "," + std::to_string(port));
    sendMessage(sock, &msg);
}

std::string Server::getUserList() {
    std::string response;
    uint16_t i = 1;
    mutex->lock();
    for (auto user : active_users) {
        response += std::to_string(i++) + ") ";
        response += user.first + "\n";
    }
    mutex->unlock();
    return response;
}

void Server::requestHandler(TCPSocket client_socket) {
    Message msg;
    std::string recvd;
    int ret_value;
    // while (!flag_signal) {
    try {
        // for (;;) {
            /* verbose("Accept...");
            TCPSocket client_socket = acceptClient();
            if (client_socket.getSockfd() < 0) {
                break;
            }
            global_client_socks.push_back(client_socket);
            verbose("Accepted"); */
            do {
                if ((ret_value = recvMessage(client_socket, std::ref(recvd), std::ref(msg))) < 0) {
                    error("recvMessage() error");
                }
                verbose("retvalue: " + std::to_string(ret_value));

                verbose("Recvd: \"" + recvd + "\" from \"" + msg.username + "\"");
                bool ret;
                std::smatch regex_results;
                switch (ret_value) {
                    case ADD_USER:
                        ret = std::regex_match(recvd, regex_results, std::regex("^(\\d+),(\\d+\\.\\d+\\.\\d+\\.\\d+)$", std::regex::icase));
                        if (!ret || addUser(msg.username, regex_results[2], std::stoi(regex_results[1])) < 0) {
                            verbose("Failed to add user.");
                            msg = buildMessage(FAIL, "FAIL");
                        }
                        else {
                            msg = buildMessage(SUCCESS, "SUCCESS");
                        }
                        sendMessage(client_socket, &msg);
                        break;
                    case RM_USER:
                        removeUser(msg.username);
                        break;
                    case USER_INFO:
                        verbose("Received USER_INFO from " + std::string(msg.username));
                        sendUserInfo(client_socket, recvd.c_str());
                        break;
                    case LIST:
                        msg = buildMessage(LIST, getUserList().c_str());
                        verbose("Received LIST");
                        sendMessage(client_socket, &msg);
                        break;
                    case CLOSED:
                        verbose("Connection closed.");
                        global_client_socks.erase(std::remove(global_client_socks.begin(),
                                                  global_client_socks.end(), client_socket), global_client_socks.end());
                        client_socket.closeSocket();
                        break;
                }
                // std::clog << "flag: " << std::to_string(flag_signal) << "\n";
            // } while (recvd.length() && !flag_signal);
            } while (recvd.length());
        // }
    }
    catch (std::system_error &e) {
        std::cout << "Server crash " << e.what() << ". Exiting...\n";
        for (TCPSocket &client : global_client_socks) {
            client.closeSocket();
        }
        global_server.getServerTCPSocket().closeSocket();
        exit(1);
    }
    catch (int e) {
        std::cout << "Server crash " << ". Exiting...\n";
        for (TCPSocket &client : global_client_socks) {
            client.closeSocket();
        }
        global_server.getServerTCPSocket().closeSocket();
        exit(1);
    }
}

/* uint8_t Server::checkFlagSignal() {
    return sflag_signal;
}
void Server::setFlagSignal() {
    sflag_signal = 0x1;
}
void Server::clearFlagSignal() {
    sflag_signal = 0x0;
} */

static void sigHandler(int signal) {
    verbose("Caught signal: " + std::to_string(signal));
    flag_signal = 0x1;
    for (TCPSocket &client : global_client_socks) {
        client.closeSocket();
    }
    global_server.getServerTCPSocket().closeSocket();
    exit(0);
}
/* Setup catch user interrupts */
void initializeSigHandler() {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}
