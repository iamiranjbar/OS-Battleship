#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h> 
#include <math.h>
#include <fcntl.h>
#include <signal.h>

#define USERNAME_MAX 10
#define WELCOME_MSG_SIZE 31
#define IP "127.0.0.1"
#define SERVER_MSG_SIZE 40
#define RIVAL_MSG_SIZE 40
#define PORT_LENGTH 5
#define TRUE 1
#define FALSE 0
#define NULLPTR '\0'
#define PAIR_MSG_LENGTH 40

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

char* get_rival_username(){
    char* rival_username = (char *)malloc(USERNAME_MAX * sizeof(char));
    char* msg = "Please enter your rival username: (if you don't care, press ENTER)\n";
    int rsz;
    write(1, msg, 70);
    rsz = read(0, rival_username, 70);
    rival_username[rsz-1] = '\0';
    return rival_username;
}

void make_server_msg_ready(char* username, int port, char* msg, char* rival_username){
    char* port_str = (char *)malloc(PORT_LENGTH*sizeof(char));
    msg = strcat(msg, username);
    msg = strcat(msg, " ");
    msg = strcat(msg, IP);
    msg = strcat(msg," ");
    port_str = toArray(port);
    msg = strcat(msg, port_str);
    msg = strcat(msg, " ");
    msg = strcat(msg, rival_username);
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

int connect_to_server(int* listening_port, char* server_ip, int server_port){
    int sockfd, numbytes;  
	struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in servaddr;
    char *msg = (char *)malloc(SERVER_MSG_SIZE * sizeof(char));
    char *username, *rival_username;
    username = get_username();
    *listening_port = generate_random_port();
    rival_username = get_rival_username();
    printf("%d\n",*listening_port);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("client: socket");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(server_port);

    if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) {
        perror("client: connect");
        close(sockfd);
        exit(-1);
    }
    make_server_msg_ready(username, *listening_port , msg, rival_username);
    if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
        perror("send");
        exit(1);
    }
    return sockfd;
}

int connect_to_rival(int role,struct rival riv, int listening_port){
    int sockfd, numbytes, listen_socket, addrlen;  
	struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in rivaddr,address;
    char* msg=(char*)malloc(RIVAL_MSG_SIZE*sizeof(char));;
    char buf[1025];
    if (role){
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("client: socket");
            exit(1);
        }

        rivaddr.sin_family = AF_INET;
        rivaddr.sin_addr.s_addr = inet_addr(riv.ip);
        rivaddr.sin_port = htons(riv.port);
        if (connect(sockfd, (struct sockaddr*) &rivaddr, sizeof(rivaddr)) == -1) {
            perror("client: connect");
            close(sockfd);
            exit(-1);
        }
        msg = "salam! let's start :D";
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

int game_is_finished(char* map){
    for(int i=0; i < 199; i++){
        if (i % 2 == 1)
            continue;
        if (map[i] == '1')
            return FALSE;
    }
    return TRUE;
}

int get_cord(int sockfd){
    int read_num,numbytes;
    char cord[4], buf[1024];
    puts("Cordinates:");
    read_num = read(0, cord , 4);
    cord[read_num] = NULLPTR;
    if ((numbytes = send(sockfd, cord, strlen(cord), 0)) == -1) {
        perror("send");
        exit(1);
    }
    if ((numbytes = recv(sockfd, buf, 1025, 0)) == -1){
        perror("recv");
        exit(1);
    }  
    buf[numbytes] = NULLPTR;
    printf("%s\n",buf);
    if(strcmp(buf,"Good Shot :}") == 0){
        return get_cord(sockfd);
    } else if(strcmp(buf,"You Win :(") == 0){
        return TRUE;
    } else if(strcmp(buf,"Failed :D") == 0){
        return FALSE;
    }
    return TRUE;
}

int recv_cord(int sockfd,char* map){
    int numbytes, x, y;
    char buf[1024];
    char* msg= (char*)malloc(RIVAL_MSG_SIZE*sizeof(char));
    if ((numbytes = recv(sockfd, buf, 1025, 0)) == -1){
        perror("recv");
        exit(1);
    }  
    buf[numbytes] = NULLPTR;
    printf("%s\n",buf);
    x = atoi(&buf[0]);
    y = atoi(&buf[2]);
    if (map[y*20 + 2*x] == '1'){
        map[y*20 + 2*x] = '0';
        if(game_is_finished(map)){
            msg = strcpy(msg,"You Win :(");
            if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
                perror("send");
                exit(1);
            }
            return TRUE;
        }
        else{
            msg= strcpy(msg,"Good Shot :}");
            if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
                perror("send");
                exit(1);
            }
            return recv_cord(sockfd, map);
        }
    }else{
        msg = strcpy(msg,"Failed :D");
        if ((numbytes = send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("send");
            exit(1);
        }
        return FALSE;
    }
}

void start_game(int role, int sockfd){
    int numbytes,read_num;
    char buf[1024];
    char map[200];
    char cord[4];
    int fd = open("map.txt",O_RDONLY);
    if (fd<0){
        perror("open");
        exit(EXIT_FAILURE);
    }
    read_num = read(fd , map, 200);
    map[read_num] = NULLPTR;
    puts("Map is loaded!");
    for (int j = 0; j < 10; j++){
        for (int i = 0; i < 20; i++){
            write(1,&map[j*20+i],1);
        }
    }
    write(1,"\n",1);
    if (role){
        while(!game_is_finished(map)){
            if (recv_cord(sockfd, map)){
                puts("You LOSE!");
                break;
            }
            if (get_cord(sockfd)){
                puts("You WIN:D");
                break;
            } 
        }
    }else{
        while(!game_is_finished(map)){
            if (get_cord(sockfd)){
                puts("You WIN:D");
                break;
            } 
            if (recv_cord(sockfd, map)){
                puts("You LOSE!");
                break;
            }
        }
    }
}

void parse_heartbeat(char* incoming_msg, char* ip, int* port){
    int space_count = 0, prev_index;
    char temp[6];
    for (int i = 0; i < strlen(incoming_msg); i++){
        if (incoming_msg[i] == ' '){
            space_count++;
            switch(space_count){
                case 1:
                    memcpy(ip, incoming_msg, i);
                    ip[i] = NULLPTR;
                    prev_index = i;
                break;
                case 2:
                    memcpy(temp, incoming_msg+prev_index+1, i - prev_index-1);
                    *port = atoi(temp);
                    prev_index = i;
                break;
            }
        }
    }
}

int server_is_up(char *argv[],char* server_ip,int* server_port){
    int sockfd, numbytes, broadcast = 1;
	struct sockaddr_in addr;
	char buf[100];
    struct timeval tout;

    tout.tv_sec = 2;
    tout.tv_usec = 0;

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("listener: socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2])); 
    addr.sin_addr.s_addr = inet_addr(IP);

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout)) < 0){
        close(sockfd);
        return FALSE;
    }

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        perror("listener: bind");
        exit(1);
    }

	printf("listener: waiting to recive hearbeat <3...\n");

    if ((numbytes = recvfrom(sockfd, buf, 99 , 0, NULL, 0)) == -1) {
        close(sockfd);
        return FALSE;
    }
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);
    parse_heartbeat(buf,server_ip,server_port);
    close(sockfd);
    return TRUE;
}

void broadcast_for_rival(char *argv[], char* username, int listening_port, char* rival_username){
    clock_t prev, now;
    double cpu_time_used;
    prev = clock();
    int sockfd, numbytes , broadcast = 1, one = 1;
    struct sockaddr_in broad_addr;
    char* msg= (char*)malloc(PAIR_MSG_LENGTH*sizeof(char));  

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port = htons(atoi(argv[4])); 
    broad_addr.sin_addr.s_addr = INADDR_ANY;

    make_server_msg_ready(username, listening_port, msg, rival_username);
    while(TRUE){
        now = clock();
        cpu_time_used = ((double) (now - prev)) / CLOCKS_PER_SEC;
        if (cpu_time_used > 1){
            prev = now;
            if ((numbytes=sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broad_addr, sizeof(broad_addr))) == -1) {
                perror("sendto");
                exit(1);
            }
            write(1,"Broadcasting for rival...\n",27);
        }
    }
}

void parse_broadcast_msg(char* incoming_msg, struct rival* riv, char* rival_name){
    int space_count = 0, prev_index;
    char temp[6];
    for (int i = 0; i < strlen(incoming_msg); i++){
        if (incoming_msg[i] == ' '){
            space_count++;
            switch(space_count){
                case 1:
                    memcpy(riv -> username, incoming_msg, i); 
                    riv -> username[i] = NULLPTR;
                    prev_index = i;
                break;
                case 2:
                    memcpy(riv -> ip, incoming_msg+prev_index+1, i - prev_index-1);
                    riv -> ip[i-prev_index-1] = NULLPTR;
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
    memcpy(rival_name, incoming_msg+prev_index+1, strlen(incoming_msg) - prev_index-2);
}

int search_for_rival(char* argv[], struct rival* riv, char* username){
    int sockfd, numbytes, broadcast = 1, one = 1;
	struct sockaddr_in addr, broad_addr;
	char buf[100];
    char rival_name[10] = "\0";
    char *final_pair_msg = (char*)malloc(sizeof(char)*SERVER_MSG_SIZE);
    char *pair_msg="You're paired 1 ";

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("listener: socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[4])); 
    addr.sin_addr.s_addr = inet_addr(IP);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) < 0){
        close(sockfd);
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        perror("listener: bind");
        exit(1);
    }

	printf("listener: waiting for rival...\n");
    while(TRUE){
        if ((numbytes = recvfrom(sockfd, buf, 99 , 0, NULL, 0)) == -1) {
            close(sockfd);
            exit(1);
        }
        buf[numbytes] = '\0';
        printf("listener: packet contains \"%s\"\n", buf);
        parse_broadcast_msg(buf,riv,rival_name);
        if((rival_name[0] == '\0' || (strcmp(rival_name,username)==0))&&(strcmp(riv -> username,username)!=0)){
            if ((strcmp(riv->username,"You're") == 0) && (strcmp(riv->ip,"paired") == 0) && (riv->port == 1)){
                printf("Rival found!\n");
                close(sockfd);
                return 0;
            }else{
                printf("Rival found!\n");
                final_pair_msg = strcat(final_pair_msg, pair_msg);
                final_pair_msg = strcat(final_pair_msg, riv->username);
                close(sockfd);
                if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
                    perror("socket");
                    exit(1);
                }
                if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
                    perror("setsockopt (SO_BROADCAST)");
                    exit(1);
                }

                broad_addr.sin_family = AF_INET;
                broad_addr.sin_port = htons(atoi(argv[4])); 
                broad_addr.sin_addr.s_addr = INADDR_ANY;
                
                if ((numbytes=sendto(sockfd, final_pair_msg, strlen(final_pair_msg), 0, (struct sockaddr *)&broad_addr, sizeof(broad_addr))) == -1) {
                    perror("sendto");
                    exit(1);
                }
                close(sockfd);
                return 1;
            }
        }
    }
}

int main(int argc, char *argv[]){
	int listening_port, sockfd, role, server_port, rival_port;
    pid_t id;
    struct rival riv;
    char server_ip[10],rival_ip;
    char *username , *rival_username;
    if (server_is_up(argv,server_ip, &server_port)){
        sockfd = connect_to_server(&listening_port, server_ip, server_port);
        role = wait_for_rival(sockfd,&riv);
        close(sockfd);
        sockfd = connect_to_rival(role,riv,listening_port);
        start_game(role, sockfd);
        close(sockfd);
    }else{
        username = get_username();
        listening_port = generate_random_port();
        rival_username = get_rival_username();
        printf("%d\n",listening_port);
        id = fork();
        if (id == 0){
            sleep(5);
            broadcast_for_rival(argv, username, listening_port, rival_username);
        }
        else{
            role = search_for_rival(argv, &riv, username);
            kill(id, SIGKILL);
            sockfd = connect_to_rival(role,riv,listening_port);
            start_game(role, sockfd);
        }
    }
	return 0;
}