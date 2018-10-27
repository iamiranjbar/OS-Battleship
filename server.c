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
     
#define TRUE   1  
#define FALSE  0  
#define LISTEN_PORT 8888  
#define MAX_PENDING 5
#define MAX_CLIENTS 30
#define NULLPTR '\0'

struct client{
    char username[10];
    char ip[10];
    int port;
    char rival[10];
};

void clean_struct(struct client* new_client){
    memset(new_client -> username, NULLPTR, 10);
    memset(new_client -> ip, NULLPTR, 10);
    new_client -> port = 0;
    memset(new_client -> rival, NULLPTR, 10);
}

void parse_request(char* incoming_msg, struct client* new_client){
    int space_count = 0, prev_index;
    char temp[5];
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
    memcpy(new_client -> rival, incoming_msg+prev_index+1, strlen(incoming_msg) - prev_index-1);
}

int main(int argc , char *argv[]){   
    int master_socket , addrlen , new_socket , client_socket[30] , activity, i , valread , sd;   
    int max_sd;
    struct sockaddr_in address;   
    struct client clients[30];     
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
                    clean_struct(&clients[i]);
                    parse_request(buffer, &clients[i]);
                    printf("%s %s %d %s\n", clients[i].username, clients[i].ip, clients[i].port, clients[i].rival);   
                }   
            }
        }   
    }   
         
    return 0;   
}  