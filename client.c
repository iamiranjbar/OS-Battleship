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
#include <time.h> 
#include <math.h>

#define SERVER_PORT 7275
#define USERNAME_MAX 10
#define WELCOME_MSG_SIZE 31
#define IP "127.0.0.1"
#define SERVER_MSG_SIZE 40
#define RIVAL_MSG_SIZE 40
#define PORT_LENGTH 5
#define TRUE 1
#define FALSE 0
#define NULLPTR '\0'

struct rival{
    char username[10];
    char ip[10];
    int port;
};

char* toArray(int number){
    int n = log10(number) + 1;
    char* numberArray = malloc((n+1) * sizeof(char));
    for (int i = n-1; i >= 0; i--){
        numberArray[i] = (number % 10) + '0';
        number /= 10;
    }
    return numberArray;
}

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int generate_random_port(){
    srand(time(NULL)); 
    return (rand() % 50535) + 1024;
}

char* get_username(){
    char* username = (char *)malloc(USERNAME_MAX * sizeof(char));
    char* welcome_msg = "Welcome!Please enter username:\n";
    int rsz;
    write(1, welcome_msg, WELCOME_MSG_SIZE);
    rsz = read(0, username, USERNAME_MAX);
    username[rsz-1] = '\0';
    return username;
}

void make_server_msg_ready(char* username, int port, char* msg){
        char* port_str = (char *)malloc(PORT_LENGTH*sizeof(char));
        msg = strcat(msg, username);
        msg = strcat(msg, " ");
        msg = strcat(msg, IP);
        msg = strcat(msg," ");
        port_str = toArray(port);
        msg = strcat(msg, port_str);
        msg = strcat(msg, " ");
}

void parse_request(char* incoming_msg, struct rival* riv){
    int space_count = 0, prev_index;
    char temp[6];
    for (int i = 0; i < strlen(incoming_msg); i++){
        if (incoming_msg[i] == ' '){
            space_count++;
            switch(space_count){
                case 1:
                    memcpy(riv -> username, incoming_msg, i); 
                    riv->username[i] = NULLPTR;
                    prev_index = i;
                break;
                case 2:
                    memcpy(riv -> ip, incoming_msg+prev_index+1, i - prev_index-1);
                    riv->ip[i-prev_index-1] = NULLPTR;
                    prev_index = i;
                break;
                case 3:
                    memcpy(temp, incoming_msg+prev_index+1, i - prev_index-1);
                    riv -> port = atoi(temp);
                    prev_index = i;
                break;
            }
        }
    }
}

int wait_for_rival(int socketfd, struct rival* riv){
    int numbytes;
    char buf[1024];
    if ((numbytes = recv(socketfd, buf, 1025, 0)) == -1){
        perror("recv");
        exit(1);
    }
    buf[numbytes] = NULLPTR;
    puts("You're Paired!");
    if(strcmp(buf, "You're Paired!") == 0){
        return 0;
    }
    parse_request(buf, riv);
    return 1;
}

int connect_to_server(int* listening_port, struct rival* riv){
    int sockfd, numbytes;  
	struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in servaddr;
    char *msg = (char *)malloc(SERVER_MSG_SIZE * sizeof(char));
    char *username;
    username = get_username();
    *listening_port = generate_random_port();
    printf("%d\n",*listening_port);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("client: socket");
            exit(1);
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(SERVER_PORT);

        if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) {
            fprintf(stderr, "client: failed to connect\n");
            perror("client: connect");
            close(sockfd);
            exit(-1);
        }
        make_server_msg_ready(username, *listening_port , msg);
        if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("send");
            exit(1);
        }
        return sockfd;
}

void connect_to_rival(int role,struct rival riv, int listening_port){
    int sockfd, numbytes, listen_socket, new_socket, addrlen;  
	struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in rivaddr,address;
    char *msg = (char *)malloc(RIVAL_MSG_SIZE * sizeof(char));
    char buf[1024];
    if (role){
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("client: socket");
            exit(1);
        }

        rivaddr.sin_family = AF_INET;
        rivaddr.sin_addr.s_addr = inet_addr(riv.ip);
        rivaddr.sin_port = htons(riv.port);
        if (connect(sockfd, (struct sockaddr*) &rivaddr, sizeof(rivaddr)) == -1) {
            fprintf(stderr, "client: failed to connect\n");
            perror("client: connect");
            close(sockfd);
            exit(-1);
        }
        msg = "salam:D";
        if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("send");
            exit(1);
        }
        return sockfd;
    }
    else{
        if( (listen_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   {   
            perror("socket failed"); 
            exit(EXIT_FAILURE);
        }   

        address.sin_family = AF_INET;   
        address.sin_addr.s_addr = inet_addr(IP) ;   
        address.sin_port = htons(listening_port);   
            
        if (bind(listen_socket, (struct sockaddr *)&address, sizeof(address))<0){   
            perror("bind failed");   
            exit(EXIT_FAILURE);   
        }   
        printf("Listener on port %d \n", listening_port);  
        addrlen = sizeof(address);
        if (listen(listen_socket, 1) < 0){   
            perror("listen");   
            exit(EXIT_FAILURE);   
        }   
        if ((listen_socket = accept(listen_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   {   
                perror("accept");   
                exit(EXIT_FAILURE);   
        }
        if ((numbytes = recv(listen_socket, buf, 1025, 0)) == -1){
            perror("recv");
            exit(1);
        }    
        buf[numbytes] = NULLPTR;
        printf("%s\n",buf);
        return listen_socket;
    }
}

void start_game(){
    
}

// 
int main(int argc, char *argv[]){
	int listening_port, sockfd, role;
    struct rival riv;
    // while(true){
    //if (server_is_up()){
        sockfd = connect_to_server(&listening_port , &riv);
        role = wait_for_rival(sockfd,&riv);
        close(sockfd);
        connect_to_rival(role,riv,listening_port);
        // start_game();
    //}
    // }
	return 0;
}