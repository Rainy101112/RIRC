CC		:= gcc
C_FLAGS	:= -Wall -Wextra -O2 -I include -o

all: build

build:
	$(CC) src/client_main.c src/irc.c $(C_FLAGS) rirc_client
	$(CC) src/server_main.c src/irc.c $(C_FLAGS) rirc_server

.PHONY: clean

clean:
	rm rirc_client rirc_server
