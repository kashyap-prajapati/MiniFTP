#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


#define PORT 9001
#define MIRROR_PORT_1 9002
#define MIRROR_PORT_2 9003 

void error(const char *msg) {
    perror(msg);
    exit(0);
}


int init_socket(char *hostname, int p_port){
        int sockfd, n;
        struct hostent *server;
        struct sockaddr_in serv_addr;
        char buffer[256];
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("Error opening socket");

        // Get server information
        server = gethostbyname(hostname);
        if (server == NULL) {
            error("Error, no such host\n");
        }

        // Initialize server address struct
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
        serv_addr.sin_port = htons(p_port);

        if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
            error("Error connecting");
        
        return sockfd;
}

int argument_count(char *str){
    int argc = 0;
    char *token = strtok(str, " ");
    while (token != NULL)
    {
        token = strtok(NULL, " ");
        argc++;
    }
    return argc;
}

int validate_command(char *buffer){
    printf("valid buffer %s\n",buffer);
    if (strstr(buffer, "w24fn") != NULL ||
        strstr(buffer, "sgetfiles") != NULL ||
        strstr(buffer, "dgetfiles") != NULL ||
        strstr(buffer, "getfiles") != NULL ||
        strstr(buffer, "gettargz") != NULL ||
        strstr(buffer, "quit") != NULL){
        if (strstr(buffer, "w24fn") != NULL){
            if (argument_count(buffer) != 2){
                printf("\n<usage>: w24fn <filename>\n\n");
                return 0;
            }
            return 1;
        }
        // validated
        return 1;
    }
    else
    {
        // Invalid command
        printf("Invalid sfdfs command\n");
        return 0;
    }
}

int main() {
    printf("Enter server hostname: ");
    char hostname[256];
    fgets(hostname, 255, stdin);
    strtok(hostname, "\n");

  //  int p_socket = init_socket(hostname,PORT);
    // int sockfd = p_socket;
    // write(p_socket,"c", 1);

    int p_port = PORT;
    // write(sockfd, "client", strlen("client"));
    
    // char read_buffer[2];
    // int n = read(p_socket, read_buffer, 1);
    // read_buffer[1] = '\0';
    // printf("read_buffer%s\n", read_buffer);
    // if (strcmp(read_buffer, "1") == 0){
    //     printf("[+]Connected to PRIMARY SERVER.\n");
    //     p_port=PORT;
    // }
    // else if (strcmp(read_buffer, "2") == 0){
    //     close(p_socket);
    //     printf("[+]Connected to MIRROR SERVER 1.\n");
    //     p_port=MIRROR_PORT_1;
    // }
    // else if(strcmp(read_buffer,"3")==0){
    //     close(p_socket); 
    //     printf("[+]Connected to MIRROR SERVER 2.\n");
    //     p_port=MIRROR_PORT_2;  
    // }
    // else{
    //     // if the server responded with anything else, then exit
    //     printf("[-]PRIMARY SERVER malfunction: Responded with unknown command\n");
    //     exit(1);
    // }
    int sockfd = init_socket(hostname,p_port);

    while (1) {
        char buffer[1024];
        int n; 
        memset(buffer, 0, 1024);
  
        printf("\nEnter command: ");
        fgets(buffer, 1023, stdin);
        strtok(buffer, "\n");
        char command[1024];
        //strcpy(command, buffer);
        //int valid = validate_command(command);
        //printf("while %s %d\n",buffer,valid);
        //if(valid==0){
        //   continue;
        //}
        // printf("p _port %d \n", p_port);
        // sockfd = init_socket(hostname, p_port);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) 
            error("Error writing to socket");

    	if(strncmp(buffer,"w24fz",5)==0 || strncmp(buffer,"w24ft",5)==0 
                || strncmp(buffer,"w24fdb",6)==0 || strncmp(buffer,"w24fda",6)==0){
    		
            char file_size_str[20] = {0};
            off_t byte_read = 0;
            off_t buffer_size;
            recv(sockfd, &buffer_size, sizeof(buffer_size), 0);
            char read_buffer[1024];
           // printf("%ld\n", buffer_size);
            if(buffer_size==0){
                printf("File not found \n"); 
            }else{
                FILE *tar_file = fopen("temp.tar.gz", "wb");
                while (byte_read < buffer_size) {
                    ssize_t bytes_received = recv(sockfd, read_buffer, sizeof(read_buffer), 0);
                    fwrite(read_buffer, 1, bytes_received, tar_file);
                    byte_read = byte_read + bytes_received;
                    printf("%d\n", byte_read);       
                }
                fclose(tar_file);
            }
    	}else{
            char read_buffer[4096];
            memset(read_buffer, 0, 4096);
            ssize_t bytes_received = recv(sockfd, read_buffer, sizeof(read_buffer), 0);
            fwrite(read_buffer, 1, bytes_received, stdout);
            if (bytes_received == -1) {
                error("Error receiving directory list from server");
            }
            fflush(stdout);
        }
        printf("I am here at the end \n");
        //close(sockfd);
    }
    close(sockfd);
        
    return 0;

}
