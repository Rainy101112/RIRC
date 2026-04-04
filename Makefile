CC		:= gcc
C_FLAGS	:= -Wall -Wextra -O2 -ggdb -I include -o

all: build

build:
	$(CC) src/client_main.c src/irc.c src/sasl.c $(C_FLAGS) rirc_client -lssl -lcrypto
	$(CC) src/server_main.c src/irc.c src/sasl.c $(C_FLAGS) rirc_server -lssl -lcrypto

.PHONY: clean

clean:
	rm rirc_client rirc_server
