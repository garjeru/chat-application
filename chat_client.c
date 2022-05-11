#define _POSIX_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>

#define SEPARATORS " \t\n"

int sockfd;
char msg[500];

void sigint_handler(int sig){
    close(sockfd);
    exit(0);
}

void *process_connection() {
	char buff[500];	
	int n;
	pthread_detach(pthread_self());
        
	while(1){
	    n = read(sockfd, buff, 500);
	    if(n <= 0){
	        close(sockfd);
		if(n == 0){
		    printf("Bye.\n");
		}else {
		    printf("Something went wrong.\n");
		    close(sockfd);
		    exit(0);
		}
		break;
	    }else if(strcmp(buff, "exit\n") == 0){
                    close(sockfd);
                    exit(0);
            }
	    buff[n] = '\0';
	    printf("%s\n", buff);

	}
	return 0;
}

int main(int argc, char * argv[]){
	int rv;
	int loginflag;
	int logoutflag;
	
	FILE *fptr;
	char *colon = ":";
	char *token;
        char fcontent[128];
        char servhost[128];
	char servport[128];
	
	char * argp[1000];
	struct addrinfo hints, *res, *ressave;
	
	char tempbuf[1000];	
	pthread_t tid;
		
	char * args[1000];
	char **arg;
	char line[1000];
	
	struct sigaction sigact;
	
	/* Ctrl-C handling */

        sigact.sa_handler = sigint_handler;
        sigact.sa_flags = 0;
        sigemptyset(&sigact.sa_mask);

        sigaction(SIGINT, &sigact, NULL);

	if(argc < 2){
		printf("Usage: a.out client_config file.\n");
		exit(0);
	}

	fptr = fopen(argv[1], "r");

	/* read client_config file */

	if(NULL == fptr){
	    printf("client_config file can not be opened\n");
	    exit(0);
	}else {
	    while(fgets(fcontent, sizeof fcontent, fptr) != NULL){
	        
		token = strtok(fcontent, colon);
		
		if(strcmp(token, "servhost") == 0){
		    token = strtok(NULL, colon);
		    strcpy(servhost, token);
		    servhost[strcspn(servhost, "\r\n")] = 0;
		}
		else if(strcmp(token, "servport") == 0){
		    token = strtok(NULL, colon);
		    strcpy(servport, token);
		    servport[strcspn(servport, "\r\n")] = 0;
		}
		
	    }
	}
	fclose(fptr);
	
	/* user input processing */
        loginflag = 0;
        logoutflag = 0;
	
	while(fgets(line, 1000, stdin) != NULL){
	    strcpy(tempbuf, line);
            arg = args;        
            *arg++ = strtok(line, SEPARATORS);
            while((*arg++ = strtok(NULL, SEPARATORS)));

	    /* login command process */

        if(strcmp(args[0], "login") == 0 && loginflag == 0){
         
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            argp[0] = &servhost[0];
            argp[1] = &servport[0];
            
            if((rv = getaddrinfo(argp[0], argp[1], &hints, &res)) != 0){
                fprintf(stderr, "%s\n", gai_strerror(rv));
                exit(1);
            }
        
            ressave = res;
            do {
                if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
                continue;
                }
                if(!connect(sockfd, res->ai_addr, res->ai_addrlen)){
                break;
                }
                close(sockfd);
            } while((res = res->ai_next) != NULL);

            freeaddrinfo(ressave);
            loginflag = 1;
            pthread_create(&tid, NULL, process_connection, NULL);
        
                write(sockfd, tempbuf, 1000);
	    }
	    else {
		if(strcmp(tempbuf,"logout\n") == 0 && loginflag == 1){  /* logout command process */
		    logoutflag = 1;
		    loginflag = 0;
		    write(sockfd, tempbuf, 1000);
	            close(sockfd);
		    continue;
	        }
	        if(strcmp(tempbuf, "exit\n") == 0 && (logoutflag == 1 || loginflag == 0)){   /* exit command process */
                    close(sockfd);
                    exit(0);
                }
            
                if(strcmp(args[0], "chat") == 0 && loginflag == 1){    /* chat command process */
		    write(sockfd, tempbuf, 1000);   
		}
	    }
	}
	
	exit(0);

}



