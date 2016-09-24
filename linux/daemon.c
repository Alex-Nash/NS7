#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <pthread.h>
#include <getopt.h>

#include "log.h"
#include "command_receiver.h"

#define PID_FILE    "/var/run/ns7_daemon.pid"
#define INIT_FILE   "ns7_mb.bin"
#define PORT        32000;

static char *app_name = NULL;
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
    printf("\n Usage: %s [OPTIONS]\n\n", app_name);
    printf("\n");
    printf("  Options:\n");
    printf("\n");
    printf("   -h --help                 Print this help\n");
    printf("\n");
    printf("   -i --init      filename   Init microblaze from file\n");
    printf("   (default the working directory filename: ns7-mb.bin)\n");
    printf("\n");
    printf("   -p --port      port num   Listen port for server\n");
    printf("   (default port number: 32000)\n");
    printf("\n");
    printf("   -s --start                Start server of motor commands\n");
    printf("\n");
    printf("   -c --stop                 Stop server\n");
    printf("\n");
    printf("   -d --daemon               Daemonize server\n");
    printf("\n");
    printf("   -l --log_file  filename   Write logs to the file\n");
    printf("   (default '/var/log/' filename: ns7-log.log)\n");
    printf("");
    printf("\n");
}


int main(int argc, char** argv)
{
    app_name = argv[0];

    if(argc == 1)
    {
        usage();
        return -1;
    }


    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"init", optional_argument, 0, 'i'},
        {"port", optional_argument, 0, 'p'},
        {"start", no_argument, 0, 's'},
        {"stop", no_argument, 0, 'c'},
        {"daemon", no_argument, 0, 'd'},
        {"log_file", optional_argument, 0, 'l'},
        {NULL, 0, 0, 0}
    };
    int value, option_index = 0, ret;

    int init = 0;
    int port_num = PORT;
    char *init_filename;

    int start = 0;
    int stop = 0;

    // Try to process all command line arguments
    while( (value = getopt_long(argc, argv, "i::p::l::scdh", long_options, &option_index)) != -1) {
        switch(value) {
        case 'h':
            usage();
            return 0;
        case 'i':
            init = 1;
            if(optarg != NULL)
            {
                init_filename = strdup(optarg);
            }
            else
            {
                init_filename = INIT_FILE;
            }
            break;
        case 'p':
            if(optarg != NULL)
            {
                port_num = atoi(optarg);
            }
            else
            {
                port_num = PORT;
            }
            break;
        case 's':
            start = 1;
            break;
        case 'c':
            stop = 1;
            break;
        case 'd':
            daemonized = 1;
            break;
        case 'l':
            if(optarg != NULL)
            {
                log_filename = strdup(optarg);
            }
            return 0;
        case '?':
            usage();
            return 0;
        default:
            break;
        }
    }

    if(init)
    {
        printf("Try to initialize microblaze...\n");
        printf("some code\n");
    }

    if(start && stop)
    {
        printf("Try to restart server\n");
    }

    int pid;
    char buf[256];

    pid_fd = open(PID_FILE, O_RDONLY);

    // stop daemon server
    if(stop)
    {
        if(pid_fd == -1)
        {
            printf("Fail to connect to server process for stoping\n");
        }

        if(read(pid_fd, buf, 256) == -1)
        {
            printf("Fail to read pid of server\n");
            close(pid_fd);
            unlink(PID_FILE);
        }
        else
        {
            pid = atoi(buf);

            if(pid <= 0)
            {
                printf("Fail: wrong pid of daemon\n");
                close(pid_fd);
                unlink(PID_FILE);
            }

            printf("Send stop signal to server (%u)\n", pid);

            if(kill(pid, SIGINT) == -1)
            {
                printf("kill: %s\n", strerror(errno));
                unlink(PID_FILE);
            }

            printf("See log for stopping status!\n");
        }
    }

    if(start && (pid_fd != -1) && !stop)
    {
        printf("Server is already running\n");
        return -1;
    }

    if(!start) return 0;

    printf("Try to start server\n");


    // open log
    if(open_log() == -1)
    {
        printf("Fail to open log: %s\n", strerror(errno));
        return -1;
    }

    if(daemonized)
    {
        printf("Server in demonized status!\n");
        printf("See log file for status.\n");
        if(daemonize() == -1)
        {
            log("Fail to daemonize process\n");
            return -1;
        }

        log("Daemonize process succses\n");

        // Daemon will handle signals
        signal(SIGINT, handle_signal);
    }

    log("Starting\n");

    running = 1;

    if(init_command_socket(port_num) == -1)
    {
        if(pid_fd != -1)
        {
            lockf(pid_fd, F_ULOCK, 0);
            close(pid_fd);
        }

        if(daemonized) unlink(PID_FILE);
    }

    log("Stop\n");

    close_log();

    return 0;
}

