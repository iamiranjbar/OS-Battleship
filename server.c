#include <stdio.h>  
#include <string.h> 
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   
#include <arpa/inet.h>   
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> 
#include <math.h>

#define TRUE   1  
#define FALSE  0  
#define LISTEN_PORT 7276 
#define MAX_PENDING 5
#define MAX_CLIENTS 30
#define NULLPTR '\0'
#define PORT_LENGTH 5

struct client{
    char username[10];
    char ip[10];
    int port;
    char rival[10];
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

void clean_struct(struct client* new_client){
    memset(new_client -> username, NULLPTR, 10);
    memset(new_client -> ip, NULLPTR, 10);
    new_client -> port = 0;
    memset(new_client -> rival, NULLPTR, 10);
}

void parse_request(char* incoming_msg, struct client* new_client){
    int space_count = 0, prev_index;
    char temp[6];
    for (int i = 0; i < strlen(incoming_msg); i++){
        if (incoming_msg[i] == ' '){
            space_count++;
            switch(space_count){
                case 1:
                    memcpy(new_client -> username, incoming_msg, i); 
                    prev_index = i;
                break;
                case 2:
                    memcpy(new_client -> ip, incoming_msg+prev_index+1, i - prev_index-1);
                    prev_index = i;
                break;
                case 3:
                    memcpy(temp, incoming_msg+prev_index+1, i - prev_index-1);
                    new_client -> port = atoi(temp);
                    prev_index = i;
                break;
            }
        }
    }
    memcpy(new_client -> rival, incoming_msg+prev_index+1, strlen(incoming_msg) - prev_index-2);
}

void make_msg_ready(struct client first_client, char* msg){
        char* port_str = (char *)malloc(PORT_LENGTH*sizeof(char));
        msg = strcat(msg, first_client.username);
        msg = strcat(msg, " ");
        msg = strcat(msg, first_client.ip);
        msg = strcat(msg," ");
        port_str = toArray(first_client.port);
        msg = strcat(msg, port_str);
        msg = strcat(msg, " ");      
}

void check_for_pariring(int* client_socket, struct client* waiting_clients, int own_id){
    int first_sd, second_sd, numbytes;
    char* msg_to_first = "You're Paired!";
    char msg_to_second[40];
    second_sd = client_socket[own_id];
    for (int i = 0;i < MAX_CLIENTS; i++){
        first_sd = client_socket[i];
        if (first_sd > 0 && i != own_id && 
            (
                (waiting_clients[i].rival[0] == NULLPTR && waiting_clients[own_id].rival[0] == NULLPTR) ||
                (strcmp(waiting_clients[own_id].username,waiting_clients[i].rival) == 0)
            ) 
            ){
            make_msg_ready(waiting_clients[i], msg_to_second);          
            if ((numbytes = send(first_sd, msg_to_first, strlen(msg_to_first), 0)) == -1) {
                perror("send");
                exit(1);
            }
            if ((numbytes = send(second_sd, msg_to_second, strlen(msg_to_second), 0)) == -1) {
                perror("send");
                exit(1);
            }
            close(first_sd);
            close(second_sd);
            client_socket[i] = 0;
            client_socket[own_id] = 0;
            clean_struct(&waiting_clients[i]);
            clean_struct(&waiting_clients[own_id]);
            break;
        }
    }
}

void send_heartbeat_message(char *argv[]){
    int sockfd, numbytes , broadcast = 1;
    struct sockaddr_in broad_addr;
    char msg[6]= "hello";

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port = htons(atoi(argv[2])); 
    broad_addr.sin_addr.s_addr = INADDR_ANY;

    while(TRUE){
        if ((numbytes=sendto(sockfd, msg, strlen(msg), 0,
             (struct sockaddr *)&broad_addr, sizeof(broad_addr))) == -1) {
            perror("sendto");
            exit(1);
        }
    }
}

int main(int argc , char *argv[]){   
    int master_socket , addrlen , new_socket , client_socket[30] , activity, i , valread , sd , max_sd;
    struct sockaddr_in address;   
    struct client waiting_clients[30];     
    char buffer[1025];          
    fd_set readfds;   

    for (i = 0; i < MAX_CLIENTS; i++){   
        client_socket[i] = 0;   
    }   
         
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   {   
        perror("socket failed"); 
        exit(EXIT_FAILURE);
    }   

    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons(LISTEN_PORT);   
         
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0){   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", LISTEN_PORT);  

    if (listen(master_socket, MAX_PENDING) < 0){   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    addrlen = sizeof(address);   
    printf("Waiting for connections ...");   
    if (fork() == 0){
        close(master_socket);
        send_heartbeat_message(argv);
    }else{   
        while(TRUE){   
            FD_ZERO(&readfds);   
            FD_SET(master_socket, &readfds);   
            max_sd = master_socket;        
            for ( i = 0 ; i < MAX_CLIENTS ; i++){   
                sd = client_socket[i];   
                if(sd > 0)   
                    FD_SET( sd , &readfds);   
                if(sd > max_sd)   
                    max_sd = sd;   
            } 

            activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
            if ((activity < 0) && (errno!=EINTR)){   
                printf("select error");
                continue;   
            }   
                
            if (FD_ISSET(master_socket, &readfds)){   
                if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   {   
                    perror("accept");   
                    exit(EXIT_FAILURE);   
                }               
                printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs (address.sin_port));   
                    
                for (i = 0; i < MAX_CLIENTS; i++){   
                    if( client_socket[i] == 0 ){   
                        client_socket[i] = new_socket;   
                        printf("Adding to list of sockets as %d \n" , i);      
                        break;   
                    }   
                }   
            }   
                
            for (i = 0; i < MAX_CLIENTS; i++){
                // printf("%s--%s--%d\n",waiting_clients[i].username,waiting_clients[i].ip,waiting_clients[i].port);   
                sd = client_socket[i];     
                if (FD_ISSET( sd , &readfds)){   
                    if ((valread = read( sd , buffer, 1024)) == 0){  
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);   
                        printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                        close( sd );   
                        client_socket[i] = 0;   
                    }   
                    else{   
                        buffer[valread] = '\0';  
                        printf("%s\n",buffer);
                        clean_struct(&waiting_clients[i]);
                        parse_request(buffer, &waiting_clients[i]);
                        check_for_pariring(client_socket, waiting_clients, i);
                    }   
                }
            }   
        }   
    }     
    return 0;   
}  