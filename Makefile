# Makefile

CC = g++
CFLAGS = -Wall -Wextra -Werror -std=c++11
SRC_DIR = src

all: server client

server: $(SRC_DIR)/server.cpp
	$(CC) $(CFLAGS) -o server $(SRC_DIR)/server.cpp

client: $(SRC_DIR)/client.cpp
	$(CC) $(CFLAGS) -o client $(SRC_DIR)/client.cpp -pthread

clean:
	rm -f server client
