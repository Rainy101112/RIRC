CC		:= gcc
C_FLAGS	:= -Wall -Wextra -O2 -o

all: build

build:
	$(CC) client_main.c $(C_FLAGS) rirc_client
	$(CC) server_main.c $(C_FLAGS) rirc_server
