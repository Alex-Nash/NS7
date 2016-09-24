#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <pthread.h>

#include "log.h"
#include "command_receiver.h"

#define DAEMON_NEED_WORK			1
#define DAEMON_NEED_TERMINATE       2

#define DAEMON_START_ATTEMP         10

#define PID_FILE "/var/run/ns7_daemon.pid"

int pid_fd;


/**
 * Callback function for handling signals.
 * sig	identifier of signal
 */
void handle_signal(int sig)
{
    if(sig == SIGINT)
    {
        log("Stopping daemon ...\n");
        // Unlock and close lockfile
        if(pid_fd != -1)
        {
            lockf(pid_fd, F_ULOCK, 0);
            close(pid_fd);
        }

        close(clientfd);
        close(sockfd);

        // Try to delete lockfile
        unlink(PID_FILE);

        running = 0;
        // Reset signal handling to default behavior
        signal(SIGINT, SIG_DFL);
    }
    else if(sig == SIGCHLD)
    {
        log( "Received SIGCHLD signal\n");
    }
}

/**
 *  This function will daemonize this app
 */
static int daemonize()
{
    pid_t pid = 0;
    int fd;

    // Fork off the parent process
    pid = fork();

    // An error occurred
    if(pid == -1)
    {
        log("Fail to fork process: %s\n", strerror(errno));
        return -1;
    }

    // Success: Let the parent terminate
    if(pid > 0)
    {
        exit(0);
    }

    // On success: The child process becomes session leader
    if(setsid() == -1)
    {
        log("Fail to create new session: %s\n", strerror(errno));
        return -1;
    }

    // Ignore signal sent from child to parent process
    signal(SIGCHLD, SIG_IGN);

    // Fork off for the second time
    pid = fork();

    // An error occurred
    if(pid == -1)
    {
        log("Fail to fork process for the second time: %s\n", strerror(errno));
        return -1;
    }

    // Let the parent terminate
    if(pid > 0)
    {
        exit(0);
    }

    // Set new file permissions
    umask(0);

    /* Change the working directory to the root directory
     or another appropriated directory */
    if(chdir("/") == -1)
    {
        log("Fail to change working dir: %s\n", strerror(errno));
        return -1;
    }

    // Close all open file descriptors
    /*for(fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }*/

    /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
    /*stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");*/

    // Try to write PID of daemon to lockfile
    char str[256];

    pid_fd = open(PID_FILE, O_RDWR|O_CREAT, 0640);
    if(pid_fd == -1)
    {
        // Can't open lockfile
        log("Fail to open pid file: %s\n", strerror(errno));
        return -1;
    }
    if(lockf(pid_fd, F_TLOCK, 0) < 0)
    {
        // Can't lock file
        log("Fail to lock pid file: %s\n", strerror(errno));
        return -1;
    }
    // Get current PID
    sprintf(str, "%d\n", getpid());
    // Write PID to lockfile
    write(pid_fd, str, strlen(str));

    return 0;
}

void usage()
{
    printf("Usage: ./daemon start\n");
    printf("       ./daemon stop\n");
}


int main(int argc, char** argv)
{
    if(argc != 2)
    {
        usage();
        return -1;
    }

    int start = 0;
    int stop = 0;

    if(strcmp(argv[1], "start") == 0) start = 1;
    if(strcmp(argv[1], "stop") == 0) stop = 1;

    if(!start && !stop)
    {
        usage();
        exit(-1);
    }


    int pid;
    char buf[256];

    pid_fd = open(PID_FILE, O_RDONLY);

    if(stop) // stop daemon
    {
        if(pid_fd == -1)
        {
            printf("Fail to connect daemon process\n");
            printf("Try to start first!\n");
            exit(-1);
        }

        if(read(pid_fd, buf, 256) == -1)
        {
            printf("Fail to read pid of daemon\n");
            close(pid_fd);
            unlink(PID_FILE);
            exit(-1);
        }

        pid = atoi(buf);

        if(pid<= 0) {
            printf("Fail: wrong pid of daemon\n");
            close(pid_fd);
            unlink(PID_FILE);
            exit(-1);
        }

        printf("Send stop signal to daemon (%u)\n", pid);

        if(kill(pid, SIGINT) == -1)
        {
            printf("kill: %s\n", strerror(errno));
            unlink(PID_FILE);
            exit(-1);
        }

        printf("See log for stopping status!\n");

        exit(-1);
    }

    if(start && (pid_fd != -1))
    {
        printf("Daemon is already running\n");
        exit(-1);
    }

    // open log
    if(open_log() == -1)
    {
        printf("Fail to open log: %s\n", strerror(errno));
        return -1;
    }

    if(daemonize() == -1)
    {
        log("Fail to daemonize process\n");
        exit(-1);
    }

    log("Daemonize process succses\n");

    // Daemon will handle signals
    signal(SIGINT, handle_signal);

    log("Starting\n");

    running = 1;

    if(init_command_socket(32000) == -1)
    {
        if(pid_fd != -1)
        {
            lockf(pid_fd, F_ULOCK, 0);
            close(pid_fd);
        }

        unlink(PID_FILE);
    }

    log("Stop\n");

    close_log();

    return 0;
}

