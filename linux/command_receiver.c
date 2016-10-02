/*
** server.c -- a stream socket server for xilinx linux
*/
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define DEBUG       3

#include "log.h"
#include "command_handler.h"
#include "command_receiver.h"


// function to handle cmd
int handle_command(char *cmd)
{
  DEBUG_MSG("CMD: \"%s\"", cmd);
  execute_command(cmd);
  return 0;
}


void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

int init_command_socket(int port)
{
    //int sockfd;  // listen on sock_fd
    struct sockaddr_in my_addr;    // my address information
    struct sigaction sa;
    int yes = 1;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ERROR_MSG("socket: fail (%s)", strerror(errno));
        return -1;
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        ERROR_MSG("setsockopt: fail (%s)", strerror(errno));
        return -1;
    }

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        ERROR_MSG("bind: fail (%s)", strerror(errno));
        return -1;
    }

    if(listen(sockfd, SOMAXCONN) == -1) {
        ERROR_MSG("listen: fail (%s)", strerror(errno));
        return -1;
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        ERROR_MSG("sigaction: fail (%s)", strerror(errno));
        return -1;
    }

    // main accept() only one connection
    while(running)
    {
        //int client_fd; // new connection or reset of connetction
        struct sockaddr_in their_addr; // connector's address information
        int sin_size;

        USER_MSG("wait for connection...");

        sin_size = sizeof(struct sockaddr_in);
        if((clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
        {
            ERROR_MSG("accept: fail (%s)", strerror(errno));
            continue;
        }

        USER_MSG("server: got connection (%s)", inet_ntoa(their_addr.sin_addr));

        int numbytes;
        char cmd[CMDSIZE];

        while(running)
        {
            if((numbytes = recv(clientfd, cmd, CMDSIZE, 0)) == -1)
            {
                ERROR_MSG("recv: fail (%s)", strerror(errno));
                break;
            }

            if(numbytes == 0)
            {
                USER_MSG("connection: lost");
                break;
            }

            if(handle_command(cmd) == -1)
            {
                ERROR_MSG("handle_command: fail (unknown command)");
                break;
            }

        }

        close(clientfd);
        close(sockfd);
        return -1;
    }

    return 0;
}
