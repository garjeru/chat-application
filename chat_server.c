#define _POSIX_SOURCE
#define _BSD_SOURCE
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/select.h>
#include<netdb.h>
#include<signal.h>

#define MAXCONN 25
#define SEPARATORS " \t\n"

int sockfd;
int client[MAXCONN];

struct user_sock {
    char username[20];
    int sockfd;
};

void shutdown_server(){
    int k;
    close(sockfd);
    for(k = 0; k < MAXCONN; k++){
	if(client[k] >= 0){
	    write(client[k], "exit\n", 10);
	    close(client[k]);
	    client[k] = -1;
	}	
    }
}

void sigint_handler(int sig){
    shutdown_server();
    exit(0);
}

int main(int argc, char * argv[]){
	int rec_sock, len, i, j;
	struct sockaddr_in addr, recaddr;

	char buf[1000];
	char tempbuf[1000];
	char chatbuff[1000];
	char res[1500];
	char result[1500];

	FILE *fp;
	char *colon = ":";
	char *token;
	char fcontent[128];
	char servport[128];
	
	fd_set allset, rset;
	int maxfd;

	int count = 0;
	char * args[1024];
	char ** arg;
	struct user_sock user_sockfd[100];
	int pmflag;
	char user[100];

	int rv;
	struct addrinfo hints, *info, *ressave;
	char hostname[1024];
	
	struct sigaction sigact;

	/* Ctrl-C handling */
	sigact.sa_handler = sigint_handler;
	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	
	sigaction(SIGINT, &sigact, NULL);
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror(": Can't get socket");
		exit(1);
	}
	
	/* read server_config file */
	fp = fopen(argv[1], "r");

	if(NULL == fp){
	    printf("server_config file can not be opened\n");
	    exit(0);
	}	
	else {
	    while(fgets(fcontent, sizeof fcontent, fp) != NULL){
		token = strtok(fcontent, colon);
	        if(strcmp(token, "port") == 0){
	            token = strtok(NULL, colon);
	            strcpy(servport, token);
		    servport[strcspn(servport, "\r\n")] = 0;
		}
	    }
	}
	fclose(fp);

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;

	if(argc == 2){
	    addr.sin_port = htons((short)atoi(&servport[0]));
	}else {
	    printf("usage: a.out config_file\n");
	}

	if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror(": bind");
		exit(1);
	}

	len = sizeof(addr);
      
	if(getsockname(sockfd, (struct sockaddr *)&addr, (socklen_t *)&len) < 0){
	    perror(": can't get the socket name");
	    exit(1);
	}
	
	/* hostname and port */
        gethostname(hostname, 1023);
	
	memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if((rv = getaddrinfo(hostname,&servport[0], &hints, &info)) != 0){
            fprintf(stderr, "%s\n", gai_strerror(rv));
            exit(1);
        }
	
	/* Display server hostname and port */
	for(ressave = info; ressave != NULL; ressave = ressave->ai_next){
	    printf("domain name : %s, port : %d\n", ressave->ai_canonname, ntohs(addr.sin_port));
	    break;
	}
	freeaddrinfo(info);
	
	if(listen(sockfd, 5) < 0){
	    perror(": bind");
	    exit(1);
	}		

	for(i = 0; i < MAXCONN; i++) client[i] = -1;
	
	FD_ZERO(&allset);
	FD_SET(sockfd, &allset);
	maxfd = sockfd;

	while(1){
		rset = allset;
		select(maxfd+1, &rset, NULL, NULL, NULL);
		if(FD_ISSET(sockfd, &rset)){
			/* Some client tries to connect */
			if((rec_sock = accept(sockfd, (struct sockaddr *)(&recaddr), (socklen_t *)&len)) < 0){
				if(errno == EINTR)
				    continue;
				else {
				    perror(":accept error");
				    exit(1);
				}
			}
			
			printf("Client remote machine = %s, port = %d.\n", 
				inet_ntoa(recaddr.sin_addr), ntohs(recaddr.sin_port));
			
			for(i = 0; i < MAXCONN; i++){
				if(client[i] < 0){
				    client[i] = rec_sock;
				    FD_SET(client[i], &allset);
				    break;
				}
			}
			if(i == MAXCONN){
			    printf("Too many connections.\n");
			    close(rec_sock);
			}
			if(rec_sock > maxfd) maxfd = rec_sock;

		}	
		for(j = 0; j < MAXCONN; j++){
	            
		    if(client[j] < 0) continue;
		    if(FD_ISSET(client[j], &rset)){
		        int num;
			num = read(client[j], buf, 1000);
			    if(num == 0){
			        /* client is exited */
			        close(client[j]);
				printf("Client remote machine %d exited.\n", client[j]);
			        FD_CLR(client[j], &allset);
			        client[j] = -1;
			    } else {
				
				strcpy(tempbuf, buf);
				arg = args;
				
				*arg++ = strtok(tempbuf, SEPARATORS);
				while((*arg++ = strtok(NULL, SEPARATORS)));
				
				/* login command issued */
				if(strcmp(args[0], "login") == 0){
			            
				    strcpy(user_sockfd[count].username, args[1]);
				    
				    user_sockfd[count].sockfd = client[j];
				      
				    count++;
				}		
				
				/* chat command issued */	
				if(strcmp(args[0], "chat") == 0){  
				    pmflag = 0;
		
				    for(i = 0; i < count; i++){
                                            if(user_sockfd[i].sockfd == client[j]){
						
                                                memset(res, 0, sizeof res);
						strcpy(res, user_sockfd[i].username);
                                                strcat(res, " >> ");	
                                                break;
                                            }
                                    }
	
				    strncpy(chatbuff, &buf[5], strlen(buf));
			            memset(user, 0, sizeof user);
				    strncpy(user, args[1], strlen(args[1]));
			            strncpy(user, &user[1], strlen(user));

				    /* send msg to a user : chat @username message */
				    for(i = 0; i < count; i++){
					
                        if((strncmp(args[1], "@", 1) == 0) && (strcmp(user_sockfd[i].username, user) == 0)) {
                            memset(result, 0, sizeof result);
                            strcat(result, res);
                            strncat(result, &chatbuff[strlen(args[1])+1], strlen(chatbuff));
                            write(user_sockfd[i].sockfd, result, strlen(result));
                            pmflag = 1;				    /* send msg to specfic user flag set */
                            break;
                        }
				    }

				    /* send msg to all user : chat message */
				    if(pmflag == 0 && (strncmp(args[1], "@", 1) != 0)) {
				
				        for(i = 0; i < count; i++){	
			            	    
                            if(user_sockfd[i].sockfd != client[j] && user_sockfd[i].sockfd != -2){
                            memset(result, 0, sizeof result);
                            strcat(result, res);
                            strcat(result, chatbuff);
                                        write(user_sockfd[i].sockfd, result, strlen(result));
                            }
				        }
				    }
				    
				}
				
				/* logout command issued */
				if(strcmp(args[0], "logout") == 0){		   
		                    for(i = 0; i < count; i++){
                                    if(user_sockfd[i].sockfd == client[j]){
                                            strcpy(user_sockfd[i].username, "");   /* set logged out user details to "" username and -2 sockfd in user_sockfd struct */
                                            user_sockfd[i].sockfd = -2;
                                            break;
                                        }
                                }
                                close(client[j]);
                                printf("Client remote machine logged out.\n");
                                FD_CLR(client[j], &allset);
                                client[j] = -1;
                }
                }
	          }       
	     }
	}
	exit(0);
}
