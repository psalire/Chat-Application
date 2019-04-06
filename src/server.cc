#include "server.h"
#define NUM_THREADS 15

int main(int argc, char **argv) {
    /* Initialize interrupt signal handler */
    initializeSigHandler();

    /* Get verbose option */
    getOption_server(argc, argv);

    /* Initialize server socket */
    TCPSocket server_socket = TCPSocket(atoi(argv[1 + option_verbose]), 0, "INADDR_ANY");

    /* Initialize server to listen state */
    Server main_server = Server(server_socket);
    if (main_server.getServerTCPSocket().getSockfd() < 0 || main_server.listenServer(SOMAXCONN) < 0) {
        server_socket.closeSocket();
        std::cout << "Exiting...\n";
        exit(1);
    }
    global_server = std::ref(main_server); /* Used in signal handling */

    // main_server.requestHandler();
    /* TODO: make requestHandler take TCPSocket as argument, main waits for connections, maintains a queue of connections,
             spawn thread with accepted conenction */
             
    for (;;) {
            verbose("Accept...");
            TCPSocket client_socket = main_server.acceptClient();
            if (client_socket.getSockfd() < 0) {
                break;
            }
            global_client_socks.push_back(client_socket);
            std::clog << "stdlog: Accepted connection.\n";
            std::thread thread_client = std::thread(&Server::requestHandler, std::ref(main_server), client_socket);
            thread_client.detach();
    }
    /* std::vector<std::thread> threads;
    for (int i = 0; i < 2; i++) {
        threads.push_back(std::thread(&Server::requestHandler, std::ref(main_server)));
    }
    for (std::thread &t : threads) {
        t.join();
    } */

    main_server.getServerTCPSocket().closeSocket();
    return 0;
}
