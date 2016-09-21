/*
** server.c -- a stream socket server for xilinx linux
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define DEBUG

#include "cos.c"
#include "command_handler.c"
#include "command_bram.c"
#include "command_receiver.h"
#include "bin_file_loader.c"


// function to handle cmd
int handle_cmd(char *cmd)
{
  printf("CMD: %s\n", cmd);
  execute_command(cmd);
  return 0;
}


void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr,"usage: server port\n");
        exit(1);
    }

    int sockfd;  // listen on sock_fd
    struct sockaddr_in my_addr;    // my address information
    struct sigaction sa;
    int yes = 1;
    int port;

    port = atoi(argv[1]);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        DEBUG_PRINT("socket: %s", strerror(errno));
		exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        DEBUG_PRINT("setsockopt: %s", strerror(errno));
		exit(1);
    }

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        DEBUG_PRINT("bind: %s", strerror(errno));
		exit(1);
    }

    if(listen(sockfd, SOMAXCONN) == -1) {
        DEBUG_PRINT("listen: %s", strerror(errno));
		exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        DEBUG_PRINT("sigaction: %s", strerror(errno));
		exit(1);
    }

    // Set cos array
    if (set_cos_array() < 0) {
        DEBUG_PRINT("set_cos_array: error set cos array\n");
    }
    DEBUG_PRINT("set_cos_array: OK!\n");
    // Load bin file to the memmory

    if (file_loader("/home/mb_hello.bin") < 0) {
        DEBUG_PRINT("file_loader: error load file\n");
    }
    DEBUG_PRINT("file_loader: OK!\n");
    // main accept() only one connection
    while(1)
    {
        int client_fd; // new connection or reset of connetction
        struct sockaddr_in their_addr; // connector's address information
        int sin_size;

        printf("Wait for connection...\n");

        sin_size = sizeof(struct sockaddr_in);
        if((client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            DEBUG_PRINT("accept");
            continue;
        }

        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

        int numbytes;
        char cmd[CMDSIZE];

        while(1)
        {

            if((numbytes = recv(client_fd, cmd, CMDSIZE, 0)) == -1) {
                DEBUG_PRINT("recv");
                break;
            }

            if(numbytes == 0) {
                DEBUG_PRINT("connection lost");
                break;
            }

            if(handle_cmd(cmd) == -1)
            {
                DEBUG_PRINT("bad cmd");
                break;
            }

        }

        close(client_fd);
    }

    return 0;
}
