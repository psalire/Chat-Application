# Philip Salire
# CMPE 156/L
# Final Project: Chat

SRC_DIR = src
BIN_DIR = bin
FLAGS = -Wall -Wextra -pedantic -Isrc/include -pthread
OBJECTS = $(BIN_DIR)/class_user.o $(BIN_DIR)/server.o $(BIN_DIR)/client.o\
			$(BIN_DIR)/class_tcpsocket.o $(BIN_DIR)/class_server.o\
			$(BIN_DIR)/class_client.o $(BIN_DIR)/helper.o
LINK_OBJECTS = $(BIN_DIR)/class_user.o $(BIN_DIR)/class_tcpsocket.o $(BIN_DIR)/helper.o
EXECS = $(BIN_DIR)/server $(BIN_DIR)/client
CC = g++ -std=c++17

.PHONY: all, clean
all: $(EXECS)

### Executables ###
$(BIN_DIR)/server: $(LINK_OBJECTS) $(BIN_DIR)/server.o $(BIN_DIR)/class_server.o
	$(CC) -o server $^ $(FLAGS)
	@mv server $(BIN_DIR)

$(BIN_DIR)/client: $(LINK_OBJECTS) $(BIN_DIR)/client.o $(BIN_DIR)/class_client.o
	$(CC) -o client $^ $(FLAGS)
	@mv client $(BIN_DIR)

### Objects ###
$(BIN_DIR)/server.o: $(SRC_DIR)/server.cc
	$(CC) -c $< $(FLAGS)
	@mv server.o $(BIN_DIR)

$(BIN_DIR)/client.o: $(SRC_DIR)/client.cc
	$(CC) -c $< $(FLAGS)
	@mv client.o $(BIN_DIR)

$(BIN_DIR)/class_server.o: $(SRC_DIR)/class_server.cc
	$(CC) -c $< $(FLAGS)
	@mv class_server.o $(BIN_DIR)

$(BIN_DIR)/class_client.o: $(SRC_DIR)/class_client.cc
	$(CC) -c $< $(FLAGS)
	@mv class_client.o $(BIN_DIR)

$(BIN_DIR)/class_tcpsocket.o: $(SRC_DIR)/class_tcpsocket.cc
	$(CC) -c $< $(FLAGS)
	@mv class_tcpsocket.o $(BIN_DIR)

$(BIN_DIR)/class_user.o: $(SRC_DIR)/class_user.cc
	$(CC) -c $< $(FLAGS)
	@mv class_user.o $(BIN_DIR)
    
$(BIN_DIR)/helper.o: $(SRC_DIR)/helper.cc
	$(CC) -c $< $(FLAGS)
	@mv helper.o $(BIN_DIR)

clean:
	rm -f $(EXECS) $(OBJECTS)
