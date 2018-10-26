#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "8888"

#define MAXDATASIZE 100
#define PORT 1111

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]){
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
    struct sockaddr_in servaddr;

    //if (server_is_up()){
        if ((sockfd = socket(AF_UNSPEC, SOCK_STREAM, 0)) == -1) {
            perror("client: socket");
            exit(1);
        }
        
        memset(servaddr, '0', sizeof (&servaddr));
        servaddr.sin_family = AF_INET;
        servadrr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(PORT);

        if (connect(sockfd, (), servinfo->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        if (p == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            return 2;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);
        printf("client: connecting to %s\n", s);

        if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("recv");
            exit(1);
        }

        close(sockfd);
    //}

	return 0;
}