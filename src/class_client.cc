/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/

/* TODO: Create function for receiving connection upon WAIT,
         saves chatbuddy_socket & username,
         this goes on its own thread,
         Move command handling to its own function,
         CHAT handling has its own loop, run getUserInput(),
         checks for commands, then sends its message if not a command to connection
*/

#include "client.h"
#define BREAK -1
#define CONTINUE 0

Client::Client() {
    current_state = INFO;
    active_socket = TCPSocket();
    listening_socket = TCPSocket();
    chatbuddy_socket = TCPSocket();
    self_publicIp = getPublicIPAddr();
    closing = 0x0;
    mutex = new std::mutex();
}
Client::Client(std::string user_name) {
    current_state = INFO;
    active_socket = TCPSocket();
    listening_socket = TCPSocket();
    chatbuddy_socket = TCPSocket();
    mutex = new std::mutex();
    self_publicIp = getPublicIPAddr();
    setName(user_name);
    closing = 0x0;
    verbose("Created client, Public IP: " + self_publicIp + ", Username: " + user_name);
}

int Client::connectSocket(TCPSocket tcpsocket) {
    struct sockaddr_in sockaddr_server = tcpsocket.getSockaddr();
    int ret = connect(tcpsocket.getSockfd(), (struct sockaddr *) &sockaddr_server, sizeof(sockaddr_server));
    if (ret < 0) {
        std::cerr << "stderr: Error: connect() fail: " << strerror(errno) << "\n";
        return ret;
    }
    // active_socket = tcpsocket;
    usleep(2500);
    return ret;
}
TCPSocket Client::connectToServer(int port, int options, std::string server_ipaddr) {
    TCPSocket client_socket = TCPSocket(port, options, server_ipaddr);
    if (connectSocket(client_socket) < 0) {
        client_socket.clearSocket();
    }
    server_socket = client_socket;
    return client_socket;
}

std::string Client::getUserInput() {
    std::string user_input;
    std::cin.clear();
    std::cout << getName() << "> ";
    getline(std::cin, user_input);
    return user_input;
}
ClientState Client::getCurrentState() const {
    mutex->lock();
    ClientState ret = current_state;
    mutex->unlock();
    return ret;
}
int Client::checkFlag() {
    mutex->lock();
    int ret = flag_signal;
    mutex->unlock();
    return ret;
}

Message Client::buildMessage(MessageType type, std::string content) {
    return buildMsg(type, getName(), content);
}
int Client::sendLISTRequest() {
    Message msg = buildMessage(LIST, "LIST");
    int ret = sendMessage(getServerSocket(), &msg);
    return ret;
}
int Client::sendADD_USERRequest() {
    int port = ntohs(getServerSocket().getSockaddr().sin_port);
    int retval;
    uint8_t fail = 0x0;
    TCPSocket new_socket;
    verbose("Finding open port...");
    do {
        new_socket = TCPSocket(++port, 0, "INADDR_ANY");
        retval = bindSocket(new_socket);
        if (retval < 0) {
            if (errno != EADDRINUSE) {
                fail = 0x1;
                verbose("Find new port fail: " + std::string(strerror(errno)));
                break;
            }
            new_socket.closeSocket();
        }
    } while (retval < 0);
    if (fail) {
        return -1;
    }
    verbose("Got new port: " + std::to_string(port));
    listening_socket = new_socket;
    /* Send binded port number & username */
    Message msg = buildMessage(ADD_USER, std::to_string(port) + "," + self_publicIp);
    int ret = sendMessage(getServerSocket(), &msg);
    /* TODO: Create new thread that waits for connection, changes state to CHAT when connected, plus saves info on who connected\ */
    /* main loop checks for CHAT state, which has its own loop ::: waitForConnection(), do in enterState() */
    return ret;
}

void Client::setState(ClientState state) {
    mutex->lock();
    current_state = state;
    mutex->unlock();
}
void Client::enterState(ClientState state) {
    switch (state) {
        case WAIT:
            if (getCurrentState() != WAIT) {
                /* Get new port number, bind it, and save to listening_socket */
                if (sendADD_USERRequest() < 0) {
                    return;
                }
                verbose("Sent WAIT info & binded socket.");
                std::string str;
                verbose("Waiting response.");
                Message msg;
                memset(&msg, 0, sizeof(msg));
                int retval = recvMessage(getServerSocket(), std::ref(str), std::ref(msg));
                if (retval < 0) {
                    verbose("recvMessage() error");
                }
                else if (retval == CLOSED) {
                    verbose("Server closed connection. Exiting...");
                    closeActiveSocket();
                    closeServerSocket();
                    exit(0);
                }
                if (msg.type == SUCCESS) {
                    setState(WAIT);
                    /* TODO: Spawn accept() thread, on accept, changes state to chat, use mutex */
                    // std::thread thread_connection(&Client::waitForConnection, this);
                    connection_thread = std::thread(&Client::waitForConnection, this);
                    // thread_connection.detach();
                }
                else {
                    std::cout << "Error: /wait fail: Failed to wait for a connection; Your username already exists on the server.\n";
                    break;
                }
            }
            std::cout << "Waiting for connection.\n";
            break;
        case INFO:
            if (getCurrentState() == WAIT) {
                std::cout << "Stopped waiting.\n";
                chatbuddy_socket.closeSocket();
                listening_socket.closeSocket();
                pthread_kill(connection_thread.native_handle(), SIGUSR2);
                // pthread_cancel(connection_thread.native_handle());
                pthread_join(connection_thread.native_handle(), NULL);
                Message msg = buildMessage(RM_USER, getName().c_str());
                sendMessage(getServerSocket(), &msg);
                verbose("Sent RM_USER.");
                setState(INFO);
            }
            else if (getCurrentState() == CHAT) {
                verbose("CHAT closing...");
                chatbuddy_socket.closeSocket();
                listening_socket.closeSocket();
                if (!checkSigFlag()) {
                    try {
                        pthread_kill(recv_thread.native_handle(), SIGUSR2);
                        pthread_join(recv_thread.native_handle(), NULL);
                    }
                    catch (std::exception &e) {
                        verbose("thread not detachable (sigsegv)");
                    }
                }
                std::cout << "\nLeft conversation with " << chatbuddy_username << ".\n";
                if (!checkSigFlag()) {
                    std::cout << getName() << "> ";
                }
                std::cout << std::flush;
                clearSigFlag();
                setState(INFO);
            }
            break;
        case CHAT:
            Message msg = buildMessage(RM_USER, getName().c_str());
            sendMessage(getServerSocket(), &msg);
            verbose("Sent RM_USER.");
            try {
                if (recv_thread.joinable()) {
                    recv_thread.join();
                }
            }
            catch (std::system_error &e) {
                verbose("recv_thread not joinable (system_error)");
            }
            recv_thread = std::thread(&Client::recvFromBuddy, this);
            setState(CHAT);
            break;
    }
}

void Client::userSignal() {
    std::cout << "\n";
    switch (getCurrentState()) {
        case INFO:
            clearSigFlag();
            verbose("INFO state; Don't exit.");
            break;
        case WAIT:
            clearSigFlag();
            enterState(INFO);
            break;
        case CHAT:
            setSigFlag();
            enterState(INFO);
            break;
    }
}

void Client::waitForConnection() {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (listenSocket(listening_socket, 1) < 0) {
        chatbuddy_username = "_fail_";
        verbose("Connection fail.");
        listening_socket.closeSocket();
        // enterState(INFO);
        // return;
        connection_thread.detach();
        pthread_exit(0);
    }
    /* TODO?: Change accept to timeout that checks a flag on timeout */
    verbose("Accepting...");
    chatbuddy_socket = acceptSocket(listening_socket);
    if (chatbuddy_socket.getSockfd() < 0) {
        chatbuddy_username = "_fail_";
        verbose("Connection fail.");
        listening_socket.closeSocket();
        // enterState(INFO);
        // return;
        connection_thread.detach();
        pthread_exit(0);
    }
    verbose("Receive chatbuddy's username...");
    /* Receive chatbuddy's username */
    Message msg;
    if (recvMessage(chatbuddy_socket, std::ref(chatbuddy_username), std::ref(msg)) <= 0) {
        chatbuddy_username = "_fail_";
        std::cout << "Connection from " << msg.username << ".\n";
        std::cout << "Left conversation with " << msg.username << "\n";
        listening_socket.closeSocket();
        // enterState(INFO);
        connection_thread.detach();
        pthread_exit(0);
    }
    std::cout << "\nConnection from " << msg.username << ".\n" << getName() << "> ";
    std::cout << std::flush;
    
    enterState(CHAT);
    connection_thread.detach();
    pthread_exit(0);
}

void Client::recvFromBuddy() {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    Message msg;
    std::string recv;
    for (;;) {
        int retval = recvMessage(chatbuddy_socket, std::ref(recv), std::ref(msg));
        if (retval <= 0) {
            if (!closing) {
                if (retval != EINTR && !checkSigFlag()) {
                    verbose("Error: failed to receive.\n");
                    enterState(INFO);
                    // recv_thread.detach();
                }
                try {
                    recv_thread.detach();
                }
                catch (std::system_error &e) {
                    verbose("thread not detachable (system_error)");
                }
                catch (std::exception &e) {
                    verbose("thread not detachable (sigsegv)");
                }
            }
            pthread_exit(0);
        }
        std::cout << "\n" << chatbuddy_username << ": " << recv << std::endl << getName() << "> ";
        std::cout << std::flush;
    }
}

void Client::connectToBuddy(std::string buddy_name) {
    Message msg = buildMessage(USER_INFO, buddy_name.c_str());
    sendMessage(getServerSocket(), &msg);
    verbose("Recv USER_INFO...");
    std::string recv;
    int retval = recvMessage(getServerSocket(), std::ref(recv), std::ref(msg));
    if (retval < 0) {
        verbose("recv() error: " + std::string(strerror(errno)));
        std::cout << "Connect to " << buddy_name << " fail.\n";
    }
    else if (!retval) {
        verbose("Server closed connection. Exiting...");
        chatbuddy_socket.closeSocket();
        listening_socket.closeSocket();
        closeActiveSocket();
        closeServerSocket();
        exit(0);
    }
    std::smatch regex_results;
    retval = std::regex_match(recv, regex_results, std::regex("^(\\d+\\.\\d+\\.\\d+\\.\\d+),(\\d+)$", std::regex::icase));
    verbose("Recvd: \"" + recv + "\" from " + std::string(msg.username));
    if (!retval) {
        std::cout << "Failed to connect to " << buddy_name << ". Enter \"/list\" to view list of users.\n";
        return;
    }
    int port = std::stoi(regex_results[2]);
    std::string ipaddr = regex_results[1];
    verbose("IP: " + ipaddr);
    verbose("Port: " + std::to_string(port));
    TCPSocket sock = TCPSocket(port, 0, ipaddr);
    if (connectSocket(sock) < 0) {
        sock.clearSocket();
    }
    chatbuddy_socket = sock;
    
    msg = buildMessage(USER_INFO, getName());
    if ((retval = sendMessage(chatbuddy_socket, &msg)) < 0) {
        verbose("send() error");
        return;
    }
    else if (retval == CLOSED) {
        std::cout << "Server has closed the connection. Exiting...\n";
        return;
    }
    chatbuddy_username = buddy_name;
    std::cout << "Connected to " << chatbuddy_username << ".\n";
    verbose("Username sent");
    enterState(CHAT);
}

int Client::commandHandler(std::string command) {
    int retval;
    std::smatch regex_results;
    if (command.compare("quit") == 0) {
        closing = 0x1;
        Message msg = buildMessage(RM_USER, getName().c_str());
        sendMessage(getServerSocket(), &msg);
        verbose("Sent RM_USER.");
        // break;
        try {
            connection_thread.detach();
            verbose("Thread closed");
        }
        catch (std::system_error &e) {
            verbose("No thread to close (system_error).");
        }
        try {
            recv_thread.detach();
            verbose("Thread closed");
        }
        catch (std::system_error &e) {
            verbose("No thread to close (system_error).");
        }
        chatbuddy_socket.closeSocket();
        listening_socket.closeSocket();
        return BREAK;
    }
    else if (command.compare("wait") == 0) {
        if (getCurrentState() != CHAT) {
            enterState(WAIT);
        }
        else {
            std::cout << "Error: Cannot wait while in CHAT. Ctrl+C to end CHAT.\n";
        }
    }
    else if (command.compare("list") == 0) {
        if ((retval = sendLISTRequest()) < 0) {
            /* sendLISTRequest() fail */
            std::cout << "Error: /list request failed";
            // continue;
            return CONTINUE;
        }
        else if (retval == CLOSED) {
            if (getCurrentState() == CHAT) {
                std::cout << "Error: /list request failed. Server has closed the connection. CHAT will continue.\n";
                return CONTINUE;
            }
            std::cout << "Server has closed the connection. Exiting...\n";
            // break;
            return BREAK;
        }
        verbose("Sent LIST request.");
        std::string recvd;
        Message msg;
        if ((retval = recvMessage(getServerSocket(), std::ref(recvd), std::ref(msg))) < 0) {
            verbose("recvMessage() error");
            // continue;
            return CONTINUE;
        }
        else if (retval == CLOSED) {
            if (getCurrentState() == CHAT) {
                std::cout << "Error: /list request failed. Server has closed the connection. CHAT will continue.\n";
                return CONTINUE;
            }
            std::cout << "Server has closed the connection. Exiting...\n";
            // break;
            return BREAK;
        }
        verbose("Recvd LIST response");
        std::cout << recvd;
    }
    else if (std::regex_search(command, regex_results, std::regex("^connect"))) {
        ClientState current_state = getCurrentState();
        if (current_state == WAIT) {
            std::cout << "Error: Cannot connect while in WAIT. Ctrl+C to end WAIT.\n";
        }
        else if (current_state == CHAT) {
            std::cout << "Error: Cannot connect while in CHAT. Ctrl+C to end CHAT.\n";
        }
        else if (std::regex_match(command, regex_results, std::regex("^connect\\s+([a-z0-9]+)\\s*$", std::regex::icase))) {
            verbose("Connect request to \"" + std::string(regex_results[1]) + "\"");
            connectToBuddy(regex_results[1]);
        }
        else {
            std::cout << "Invalid /connect command. Usage: /connect [username], [username] must be [a-zA-Z0-9]\n";
        }
    }
    else {
        std::cout << "Command /" << command << " not recognized.\n";
    }
    return 1;
}

void Client::messagingHandler() {
    std::string user_input;
    int retval;
    Message msg;
    for (;;) {
        user_input = getUserInput();
        if (flag_signal) {
            userSignal();
            continue;
        }

        /* Ignore blank user input */
        if (!user_input.length()) {
            continue;
        }

        // verbose("State: " + std::to_string(main_client.getCurrentState()));
        /* Command handling, indicated by leading '/' */
        if (user_input.at(0) == '/') {
            retval = commandHandler(user_input.substr(1));
            if (retval == BREAK) {
                break;
            }
            else if (retval == CONTINUE) {
                continue;
            }
        }
        /* If in wait and not a command, do nothing & print waiting message */
        else if (getCurrentState() == WAIT) {
            std::cout << "Waiting for connection.\n";
            continue;
        }
        /* CHAT has its own loop */
        else if (getCurrentState() == CHAT) {
            verbose("CHAT");
            msg = buildMessage(MSG, user_input);
            if ((retval = sendMessage(chatbuddy_socket, &msg)) < 0) {
                std::cerr << "Error: Message failed to send.\n";
                continue;
            }
            else if (retval == CLOSED) {
                enterState(INFO);
                continue;
            }
            verbose("MSG sent");
        }
        else {
            /* Send message, check if server is still live */
            msg = buildMessage(MSG, user_input);
            if ((retval = sendMessage(getServerSocket(), &msg)) < 0) {
                verbose("send() error");
                continue;
            }
            else if (retval == CLOSED) {
                std::cout << "Server has closed the connection. Exiting...\n";
                enterState(INFO);
                break;
            }
            verbose("MSG sent");
        }
    }
}

void Client::closeActiveSocket() {
    if (active_socket.getSockfd() == server_socket.getSockfd()) {
        server_socket.clearSocket();
    }
    getActiveSocket().closeSocket();
}
void Client::closeServerSocket() {
    getServerSocket().closeSocket();
}

void Client::setSigFlag() {
    mutex->lock();
    flag_signal = 0x1;
    mutex->unlock();
}
void Client::clearSigFlag() {
    mutex->lock();
    flag_signal = 0x0;
    mutex->unlock();
}
uint8_t Client::checkSigFlag() {
    mutex->lock();
    uint8_t ret = flag_signal;
    mutex->unlock();
    return ret;
}

static void sigHandler(int signal) {
    if (signal == SIGINT) {
        flag_signal = 0x1;
    }
    verbose("Signal: " + std::to_string(signal));
}
/* Setup catch user interrupts */
void initializeSigHandler() {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	sigaction(SIGUSR2, &sigIntHandler, NULL);
}
