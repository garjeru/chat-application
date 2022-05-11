# chat-application
A Chat Application

Design and Implementation details :
1. Pthread and select() are used in the project to support I/O multiplexing.
2. Server is implemented using select() system call.
3. Client is implemented using POSIX threads.
4. Server maintains fd_set of active client connections. Select() function indicates file descriptor which are ready for reading. select() is also a blocking call until any of the file descriptor is ready.
5. For each new client connection a separate thread is created and processed.
6. client_config maintains the server hostname(or dotted-decimal IP address) and port.
7. server_config maintains server port detail. If server port is 0, at the time of connection establishment, server is assigned with any unused port.
8. Once client starts, a user can issue exit or login command. After login command, user can issue chat and logout commands. 
9. On logout command, user can login again with same/different username on the same the exiting terminal. Also user can issue exit command after logout.
10. Server maintains the MAXCONN 25 in client array initialized with -1 at start. On every new connection this client array is populated with fd.
11. Server maintains the username and file descriptor mapping in user_sockfd struct.
12. On login command, server adds entry to user_sockfd struct.
13. On logout command, server update existing entry in user_sockfd struct. Username is set to "" and fd is set to -2. 
14. Signal SIGINT is handled, which is generated on Ctrl-C. Program closes the opened socket connection before they exit.
15. When user issues the exit command, all the opened socket connections are closed properly before exiting the program.


To run the program :

Note : update client_config with server hostname and port displayed on server start. 

Commands to use :

$ make chat_server
$ make chat_client
$ chat_server.x server_config
$ chat_client.x client_config
$ login sam
$ chat @sam hi
$ chat hello all
$ logout
$ exit 
