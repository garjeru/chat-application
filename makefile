#Makefile

CC=gcc

chat_server: chat_server.c
	$(CC) -Wall -ansi -pedantic -o chat_server.x chat_server.c

chat_client: chat_client.c
	$(CC) -Wall -ansi -pedantic -o chat_client.x chat_client.c -lpthread
