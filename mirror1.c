#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <sys/sendfile.h>

#define PORT 9002
#define MAX_CLIENTS 5
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64


char *home;
char *commands[MAX_ARGS];

#define MAX_PATH_LEN 256

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int compare_creation_time(const void *a, const void *b) {
    const struct dirent **dir_a = (const struct dirent **)a;
    const struct dirent **dir_b = (const struct dirent **)b;
    struct stat stat_a, stat_b;

    stat((*dir_a)->d_name, &stat_a);
    stat((*dir_b)->d_name, &stat_b);

    return stat_a.st_ctime - stat_b.st_ctime;
}

void execute_dirlist_t(const char *home,int client_socket) {
    // Open home directory
    DIR *dir = opendir(getenv("HOME"));
    if (dir == NULL) {
        perror("Error opening home directory");
        return;
    }

    // Read directory entries and sort by creation time
    struct dirent **dir_list;
    int num_dirs = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            num_dirs++;
        }
    }

    // Allocate memory for directory list
    dir_list = (struct dirent **)malloc(num_dirs * sizeof(struct dirent *));
    if (dir_list == NULL) {
        perror("Error allocating memory");
        closedir(dir);
        return;
    }

    // Reset directory stream
    rewinddir(dir);

    // Fill directory list
    int index = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            dir_list[index++] = entry;
        }
    }

    // Sort directory list by creation time
    qsort(dir_list, num_dirs, sizeof(struct dirent *), compare_creation_time);

    // Send directory names to client
    for (int i = 0; i < num_dirs; i++) {
        if (send(client_socket, dir_list[i]->d_name, strlen(dir_list[i]->d_name), 0) == -1) {
            perror("Error sending directory name to client");
            free(dir_list);
            closedir(dir);
            return;
        }
        if (send(client_socket, "\n", 1, 0) == -1) {
            perror("Error sending newline character to client");
            free(dir_list);
            closedir(dir);
            return;
        }
    }

    free(dir_list);
    closedir(dir);
}

void send_tarfile(int client_socket){

    int fd = open("temp.tar.gz", O_RDONLY);
    if (fd == -1)
    {
            printf("Error opening archive...\n");
            exit(0);
    }

        // send file size
    struct stat st;
    if (stat("temp.tar.gz", &st) >= 0)
    {
        off_t buffer_size = st.st_size;
        send(client_socket, &buffer_size, sizeof(buffer_size), 0);
    }
    else
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    // Read archive into buffer and send to client over socket
    char buffer[1024];

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, 1024)) > 0)
    {
        ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent < bytes_read)
        {
            printf("Error sending data...\n");
            exit(0);
        }
    }
    remove("temp.tar.gz");

    close(fd);
    printf("File sent successfully...\n");  
}

void execute_w24fz(const char *size1, const char *size2,int client_socket) {
    char validate_command[256];
    snprintf(validate_command, sizeof(validate_command), "find ~ -type f -size +%sc -size -%sc -print0", size1, size2);
    FILE *fp = popen(validate_command, "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    if (fgets(validate_command, sizeof(validate_command), fp) == NULL) {
        off_t buffer_size = 0;
        send(client_socket, &buffer_size, sizeof(buffer_size), 0);
        pclose(fp);
        return;
    }

    pclose(fp);
    
    char command[256];
    snprintf(command, sizeof(command), "find ~ -type f -size +%sc -size -%sc -print0 | tar -czf temp.tar.gz --null -T -", size1, size2);

    if (system(command) == -1) {
        perror("Error executing system command");
        return;
    }

    send_tarfile(client_socket);
}

void execute_dirlist_a(const char *home, int client_socket){
    DIR *dir = opendir(getenv("HOME"));
    
    if (dir == NULL) {
        perror("Error opening home directory");
        return;
    }

    // Read directory entries and send subdirectories to client
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (send(client_socket, entry->d_name, strlen(entry->d_name), 0) == -1) {
                perror("Error sending directory name to client");
                closedir(dir);
                return;
            }
            if (send(client_socket, "\n", 1, 0) == -1) {
                perror("Error sending newline character to client");
                closedir(dir);
                return;
            }
        }
    }
    send(client_socket,"END",strlen("END"),0);

    closedir(dir);
} 

void execute_w24fdb(const char *date, int client_socket){
    char validate_command[256];
    snprintf(validate_command, sizeof(validate_command), "find ~ -type f ! -newermt \"%s\" -print0", date);
    FILE *fp = popen(validate_command, "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    if (fgets(validate_command, sizeof(validate_command), fp) == NULL) {
        off_t buffer_size = 0;
        send(client_socket, &buffer_size, sizeof(buffer_size), 0);
        pclose(fp);
        return;
    }

    pclose(fp);

    char command[512];
    snprintf(command, sizeof(command), "find ~ -type f ! -newermt \"%s\" -print0 | tar -czf temp.tar.gz --null -T -", date);
    
    if (system(command) == -1) {
        perror("Error executing system command");
        return;
    }

    send_tarfile(client_socket);
    // Open and send the tar.gz file back to client
    // FILE *tar_file = fopen("temp.tar.gz", "rb");
    // if (tar_file == NULL) {
    //     perror("Error opening temp.tar.gz");
    //     return;
    // }

    // char buffer[1024];
    // size_t bytes_read;
    // while ((bytes_read = fread(buffer, 1, sizeof(buffer), tar_file)) > 0) {
    //     if (send(client_socket, buffer, bytes_read, 0) == -1) {
    //         perror("Error sending tar.gz file to client");
    //         fclose(tar_file);
    //         return;
    //     }
    // }

    // fclose(tar_file);

    // // Signal end of transmission
    // send(client_socket, "END", strlen("END"), 0);

    // // Remove the temporary tar.gz file
    // if (remove("temp.tar.gz") != 0) {
    //     perror("Error removing temporary tar.gz file");
    // }

}

void execute_w24fda(const char *date, int client_socket){
    char validate_command[256];
    snprintf(validate_command, sizeof(validate_command), "find ~ -type f -newermt \"%s\" -print0", date);
    FILE *fp = popen(validate_command, "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    if (fgets(validate_command, sizeof(validate_command), fp) == NULL) {
        off_t buffer_size = 0;
        send(client_socket, &buffer_size, sizeof(buffer_size), 0);
        pclose(fp);
        return;
    }

    pclose(fp);
    
    char command[512];
    snprintf(command, sizeof(command), "find ~ -type f -newermt \"%s\" -print0 | tar -czf temp.tar.gz --null -T -", date);
    
    if (system(command) == -1) {
        perror("Error executing system command");
        return;
    }

    send_tarfile(client_socket);

    // Open and send the tar.gz file back to client
    // FILE *tar_file = fopen("temp.tar.gz", "rb");
    // if (tar_file == NULL) {
    //     perror("Error opening temp.tar.gz");
    //     return;
    // }

    // char buffer[1024];
    // size_t bytes_read;
    // while ((bytes_read = fread(buffer, 1, sizeof(buffer), tar_file)) > 0) {
    //     if (send(client_socket, buffer, bytes_read, 0) == -1) {
    //         perror("Error sending tar.gz file to client");
    //         fclose(tar_file);
    //         return;
    //     }
    // }

    // fclose(tar_file);

    // // Signal end of transmission
    // send(client_socket, "END", strlen("END"), 0);

    // // Remove the temporary tar.gz file
    // if (remove("temp.tar.gz") != 0) {
    //     perror("Error removing temporary tar.gz file");
    // }    
}

void execute_w24ft(char *extensions, int client_socket){

    char *ext1 = strtok(extensions, " ");
    char *ext2 = strtok(NULL, " ");
    char *ext3 = strtok(NULL, " ");

    if (!ext1) {
        send(client_socket, "No file found", strlen("No file found"), 0);
        return;
    }

    // Construct the find command with tar to create a tar file containing found files with specified extensions
    char command[512];
    if (ext2 && ext3) {
        snprintf(command, sizeof(command), "find ~ -type f \\( -name '*.%s' -o -name '*.%s' -o -name '*.%s' \\) -print0 | tar -czf temp.tar.gz --null -T -", ext1, ext2, ext3);
    } else if (ext2) {
        snprintf(command, sizeof(command), "find ~ -type f \\( -name '*.%s' -o -name '*.%s' \\) -print0 | tar -czf temp.tar.gz --null -T -", ext1, ext2);
    } else {
        snprintf(command, sizeof(command), "find ~ -type f -name '*.%s' -print0 | tar -czf temp.tar.gz --null -T -", ext1);
    }

    if (system(command) == -1) {
        perror("Error executing system command");
        return;
    }

    // Open and send the tar.gz file back to client
    FILE *tar_file = fopen("temp.tar.gz", "rb");
    if (tar_file == NULL) {
        perror("Error opening temp.tar.gz");
        return;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), tar_file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Error sending tar.gz file to client");
            fclose(tar_file);
            return;
        }
    }

    fclose(tar_file);

    // Signal end of transmission
    send(client_socket, "END", strlen("END"), 0);

    // Remove the temporary tar.gz file
    if (remove("temp.tar.gz") != 0) {
        perror("Error removing temporary tar.gz file");
    }

}



void execute_w24fn(int sockfd, const char *filename) {
    struct stat fileStat;
    char command[MAX_COMMAND_LENGTH];
    char output[MAX_COMMAND_LENGTH];
    snprintf(command, 
        sizeof(command), 
        "find %s -name \"%s\" -type f -print -exec ls -lh --time-style=+\"%%Y-%%m-%%d %%H:%%M:%%S\" {} \\; 2>/dev/null | head -n1",
        getenv("HOME"),
        filename
        );
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
    }
    if (fgets(output, sizeof(output), fp) != NULL) {
        output[strcspn(output, "\n")] = 0;
        stat(output, &fileStat);

        mode_t mode = fileStat.st_mode;
        char permissions[11];

        // Owner permissions
        permissions[0] = (mode & S_IRUSR) ? 'r' : '-';
        permissions[1] = (mode & S_IWUSR) ? 'w' : '-';
        permissions[2] = (mode & S_IXUSR) ? 'x' : '-';

        // Group permissions
        permissions[3] = (mode & S_IRGRP) ? 'r' : '-';
        permissions[4] = (mode & S_IWGRP) ? 'w' : '-';
        permissions[5] = (mode & S_IXGRP) ? 'x' : '-';

        // Other permissions
        permissions[6] = (mode & S_IROTH) ? 'r' : '-';
        permissions[7] = (mode & S_IWOTH) ? 'w' : '-';
        permissions[8] = (mode & S_IXOTH) ? 'x' : '-';
        permissions[9] = '\0'; // Null-terminate the string

        off_t size = fileStat.st_size;

        char file_info[1024];
        snprintf(file_info, 1024, "%s\t%s\t%s\t%ld\n",filename,ctime(&fileStat.st_ctime), permissions,size);
        send(sockfd, file_info, strlen(file_info), 0);
        //close(sockfd);    
    }
    else
    {
        char *message = "File not found";
        send(sockfd, message, strlen(message), 0);
    }

}


void crequest(int server_socket, int sockfd) {
    char buffer[1024];

    pid_t child = fork();

    if(child==0){

        close(server_socket);
        while (1)
        {
            int n;
          //  bzero(buffer, 1024);
            memset(buffer, '\0', sizeof(buffer));
            // receive the client request
            n = recv(sockfd, buffer, 1024,0);

            if (strlen(buffer) == 0)
            {
                continue;
            }
            if (n < 0) 
                error("Error reading from socket");

            // Check if the received command is "quitc"
            if (strncmp(buffer, "quitc", 5) == 0) {
                printf("Client requested to quit.\n");
                return;
            }
            // Check if the received command is "w24fn"
            if (strncmp(buffer, "w24fn", 5) == 0) {
                // Extract filename from the command
                char *filename = strtok(buffer + 6, "\n");
                if (filename == NULL) {
                    write(sockfd, "Invalid command syntax", 22);
                    return;
                }
                execute_w24fn(sockfd, filename);
            }else if(strncmp(buffer,"dirlist",7)==0){
                char *token = strtok(buffer+8, " ");
                if(token == NULL){
                    write(sockfd, "Invalid command systax",22);
                
                }
                if(strncmp(token,"-a",2)==0){
                    execute_dirlist_a(getenv("HOME"),sockfd);
                }else if(strncmp(token,"-t",2)==0){
                    execute_dirlist_t(getenv("HOME"),sockfd);
                }else{
                    write(sockfd, "Invalid command systax",22);
                    return; 
                }
                return;
            }
            else if(strncmp(buffer, "w24fda",6)==0){
                const char *date = strtok(buffer + 7, "\n");
                if (date == NULL) {
                    write(sockfd, "Invalid command syntax", 22);
                    return;
                }
                execute_w24fda(date,sockfd);
            }
            else if(strncmp(buffer, "w24fdb",6)==0){
                const char *date = strtok(buffer + 7, "\n");
                if (date == NULL) {
                    write(sockfd, "Invalid command syntax", 22);
                    return;
                }
                execute_w24fdb(date,sockfd);
            }
            else if(strncmp(buffer, "w24ft",5)==0){
                char *extentions = strtok(buffer + 6, "\n");
                if (extentions == NULL) {
                    write(sockfd, "Invalid command syntax", 22);
                    return;
                }
                execute_w24ft(extentions,sockfd);
            }
            else if(strncmp(buffer,"w24fz",5)==0){
                    char *token = strtok(buffer + 6, " ");
                if (token == NULL) {
                    write(sockfd, "Invalid command syntax", 22);
                    return;
                }
                char *size1 = token;
                token = strtok(NULL, " ");
                if (token == NULL) {
                    write(sockfd, "Invalid command syntax", 22);
                    return;
                }
                char *size2 = token;
                // Call function to create tarball with files within the specified size range
        //        execute_w24fz(getenv("HOME"), size1, size2, sockfd);
                execute_w24fz(size1,size2,sockfd);
            }else {
                // Invalid command
                write(sockfd, "Invalid command", 15);
            }
            memset(buffer, 0, sizeof(char) * sizeof(buffer));
        }
    }
}


int init_socket(){
    int serverSocket;
    int opt = 1;
    struct sockaddr_in serverAddr;

    // Create and verify socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Server Socket is created.\n");

    // prepare the sockaddr_in structure
    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    //serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // set socket options
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        printf("setsockopt failed");
        exit(1);
    }
    // bind the socket to defined options
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("[-]Error in binding.\n");
        exit(1);
    }
    printf("[+]Bind to port %d\n", PORT);

    // start Listening
    if (listen(serverSocket, 10) == 0)
    {
        printf("[+]Listening....\n");
    }
    else
    {
        printf("[-]Error in listening....\n");
        exit(1);
    }
    return serverSocket;
}


int client=0;
int main() {
  
    home = getenv("HOME");
    int serverSocket;
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t addr_size;

    serverSocket = init_socket();

    int serverw24_connections = 0;
    int mirror1_connections = 0;
    int mirror2_connections = 0;

    while (1) {
        // Accept connection from client

        newsockfd = accept(serverSocket, (struct sockaddr *) &cli_addr, &addr_size);
        //if (newsockfd < 0) 
        //    error("Error on accept");

        // int current_server;
        // if (serverw24_connections == 0) {
        //     current_server = 1; // serverw24
        //     serverw24_connections++;
        // } else if (serverw24_connections == 1) {
        //     current_server = 2; // mirror1
        //     serverw24_connections++;
        // } else {
        //     current_server = 3; // mirror2
        //     serverw24_connections++;
        // }

        // if(current_server==1){
        //     write(newsockfd, "1", 1);
        // }else if(current_server==2){
        //     write(newsockfd, "2", 1);
        //     continue;
        // }else if(current_server==3){
        //     write(newsockfd, "3", 1);
        //     continue;
        // }


        //}
    //    if(current_server==1){
            // pid = fork();
            // if (pid < 0) 
            //     error("Error on fork");

            // if (pid == 0) {
            //     close(sockfd); // Close unused socket descriptor
            //     crequest(newsockfd); // 
            //    // close(newsockfd); // Close client socket after handling request
            //     exit(0); // Exit child process
            // } else {
            //     // This is the parent process
            //    close(newsockfd); // Close unused socket descriptor
            // }
        // }else if(current_server==2){

        // }else if(current_server==3){

        // }
               
        crequest(serverSocket,newsockfd);
        //close(newsockfd);
    }

    close(newsockfd); // Close server socket
    return 0;
}
