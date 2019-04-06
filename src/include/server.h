#ifndef _SERVER_H_
#define _SERVER_H_

#include <map>
#include "tcpsocket.h"
#include "user.h"
#include "helper.h"

class Server : public Messaging {
    TCPSocket server_tcpsocket;
    std::map<std::string,std::pair<std::string,int>> active_users;
    std::mutex *mutex;
    uint8_t sflag_signal;

    public:
        Server() {};
        Server(TCPSocket tcpsocket);

        TCPSocket &getServerTCPSocket() {return server_tcpsocket;}

        Message buildMessage(MessageType type, std::string content);

        int addUser(const char user_name[128], std::string ip_addr, int port);
        void removeUser(const char user_name[128]);
        void sendUserInfo(TCPSocket sock, const char user_name[128]);
        std::string getUserList();

        int bindServer(TCPSocket tcpsocket);
        int listenServer(int num);
        TCPSocket acceptClient();
        
        void requestHandler(TCPSocket client_socket);
        /* uint8_t checkFlagSignal();
        void setFlagSignal();
        void clearFlagSignal(); */
};
void initializeSigHandler();

extern Server global_server; /* Used in signal handling */
extern std::vector<TCPSocket> global_client_socks;

#endif
