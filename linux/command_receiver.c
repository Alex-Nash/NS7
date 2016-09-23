/*
** server.c -- a stream socket server for xilinx linux
*/
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

#include "log.h"
#include "cos.h"
#include "command_handler.h"
#include "command_bram.h"
#include "command_receiver.h"


// function to handle cmd
int handle_command(char *cmd)
{
  char command_str[7];

  write_log("CMD: %s\n", cmd);

  memcpy(command_str, cmd, 7);
  write_log("command_str: %s\n", command_str);

  struct command cur_command;
  parse_command(&cur_command, command_str);

  write_log(" position: %c\n", cur_command.position);
  write_log("direction: %lu\n", cur_command.direction);
  write_log("     torq: %lu\n", cur_command.torq);

  send_command(&cur_command);
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
        write_log("socket: %s", strerror(errno));
        return -1;
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        write_log("setsockopt: %s", strerror(errno));
        return -1;
    }

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);       // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        write_log("bind: %s", strerror(errno));
        return -1;
    }

    if(listen(sockfd, SOMAXCONN) == -1) {
        write_log("listen: %s", strerror(errno));
        return -1;
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        write_log("sigaction: %s", strerror(errno));
        return -1;
    }

    // Set cos array
    if (set_cos_array() < 0) {
        write_log("set_cos_array: error set cos array\n");
        return -1;
    }

    // main accept() only one connection
    while(running)
    {
        //int client_fd; // new connection or reset of connetction
        struct sockaddr_in their_addr; // connector's address information
        int sin_size;

        write_log("Wait for connection...\n");

        sin_size = sizeof(struct sockaddr_in);
        if((clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            write_log("accept: s", strerror(errno));
            continue;
        }

        write_log("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

        int numbytes;
        char cmd[CMDSIZE];

        while(running)
        {
            if((numbytes = recv(clientfd, cmd, CMDSIZE, 0)) == -1)
            {
                write_log("recv: %s\n", strerror(errno));
                break;
            }

            if(numbytes == 0)
            {
                write_log("connection lost\n");
                break;
            }

            if(handle_command(cmd) == -1)
            {
                write_log("handle_command: bad command\n");
                break;
            }

        }

        close(clientfd);
        close(sockfd);
        return -1;
    }

    return 0;
}
