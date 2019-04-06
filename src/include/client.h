/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <unistd.h>
#include <signal.h>
#include "tcpsocket.h"
#include "user.h"
#include "helper.h"

enum ClientState {
    INFO,
    WAIT,
    CHAT
};

class Client : public Messaging, public User {
    TCPSocket server_socket, active_socket, listening_socket, chatbuddy_socket;
    ClientState current_state;
    std::string self_publicIp, chatbuddy_username;
    std::thread connection_thread, recv_thread;
    std::mutex *mutex;
    uint8_t closing;

    public:
        Client();
        Client(std::string user_name);

        TCPSocket getActiveSocket() const {return active_socket;}
        TCPSocket getServerSocket() const {return server_socket;}
        int getActiveSockfd() const {return active_socket.getSockfd();}
        ClientState getCurrentState() const;
        std::string getUserInput();
        int checkFlag();
        void setState(ClientState state);

        Message buildMessage(MessageType type, std::string content);
        int sendLISTRequest();
        int sendADD_USERRequest();

        void enterState(ClientState state);
        void userSignal();

        int connectSocket(TCPSocket tcpsocket);
        TCPSocket connectToServer(int port, int options, std::string server_ipaddr);

        void waitForConnection();
        void recvFromBuddy();
        void connectToBuddy(std::string buddy_name);

        int commandHandler(std::string command);
        void messagingHandler();

        void closeActiveSocket();
        void closeServerSocket();
        
        void setSigFlag();
        void clearSigFlag();
        uint8_t checkSigFlag();
};
void initializeSigHandler();

#endif
