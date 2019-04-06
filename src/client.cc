#include "client.h"

int main(int argc, char **argv) {
    /* Initialize interrupt signal handler */
    initializeSigHandler();

    /* Get verbose option */
    getOption_client(argc, argv);

    /* Get arguments */
    int port                  = atoi(argv[2 + option_verbose]);
    std::string server_ipaddr = argv[1 + option_verbose],
                user_name     = argv[3 + option_verbose];

    /* Initialize client */
    Client main_client = Client(user_name);

    /* Connect to server */
    main_client.connectToServer(port, 0, server_ipaddr);
    if (main_client.getServerSocket().getSockfd() < 0) {
        exit(0);
    }

    main_client.messagingHandler();

    main_client.closeActiveSocket();
    main_client.closeServerSocket();
    return 0;
}
