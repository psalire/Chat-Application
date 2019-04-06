Chat

Files:
/
  Makefile            : Used to compile program
  /bin                : All .o and executables are placed here
  /src                : All .cc files
    /include          : All .h files
    server.cc         : Main server file
    client.cc         : Main client file
    class_client.cc   : Source for Client class
    class_server.cc   : Source for Server class
    class_user.cc     : Source for User class
    communication.cc  : Source for communication functions

Limitations:
  - Very long messages will be cut.
  - Very long usernames will be cut.

Tested for:
  - Many clients over different networks
  - All rubric critera

