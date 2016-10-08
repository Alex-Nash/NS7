#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <getopt.h>
#include <sys/stat.h>

#include "log.h"
#include "command_receiver.h"
#include "command_handler.h"
#include "bin_file_loader.h"
#include "gpio.h"

#define PID_FILE    "/var/run/ns7_daemon.pid"
#define INIT_FILE   "mb_server.bin"
#define LOG_FILE    "ns7.log"
#define PORT        32000;

static char *app_name = NULL;
int pid_fd;

int is_daemon;



/**
 * Callback function for handling signals.
 * sig	identifier of signal
 */
void handle_signal(int sig)
{
    if(sig == SIGINT)
    {
        DEBUG_MSG("stop signal: received");
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
        DEBUG_MSG("SIGCHLD signal: received");
    }
}

/**
 *  This function will daemonize this app
 */
static int daemonize()
{
    pid_t pid = 0;
    int fd;

    USER_MSG("starting daemon server");
    USER_MSG("see log for status");
    daemonized = is_daemon;

    // Fork off the parent process
    pid = fork();

    // An error occurred
    if(pid == -1)
    {
        ERROR_MSG("process (%d): fail to fork (%s)", getpid(), strerror(errno));
        return -1;
    }

    // Success: Let the parent terminate
    if(pid > 0)
    {
        DEBUG_MSG("process (%d): exit", getpid());
        exit(0);
    }

    // On success: The child process becomes session leader
    if(setsid() == -1)
    {
        ERROR_MSG("new session: fail to create (%s)", strerror(errno));
        return -1;
    }

    // Ignore signal sent from child to parent process
    signal(SIGCHLD, SIG_IGN);

    // Fork off for the second time
    pid = fork();

    // An error occurred
    if(pid == -1)
    {
        ERROR_MSG("process (%d): fail to fork (%s)", getpid(), strerror(errno));
        return -1;
    }

    // Let the parent terminate
    if(pid > 0)
    {
        DEBUG_MSG("process (%d): exit", getpid());
        exit(0);
    }


    // Set new file permissions
    umask(0);

    /* Change the working directory to the root directory
     or another appropriated directory */
    if(chdir("/") == -1)
    {
        ERROR_MSG("working dir: fail to change (%s)", strerror(errno));
        return -1;
    }

    // Close all open file descriptors
    /*for(fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }*/

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2)
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");

    // open log
    /* DEBUG_MSG("log file (%s): open", log_filename);
    if(open_log() == -1)
    {
        ERROR_MSG("log file (%s): fail to open (%s)", log_filename, strerror(errno));
        return -1;
    } */

    // Try to write PID of daemon to lockfile
    DEBUG_MSG("PID of process: save");
    char str[256];

    pid_fd = open(PID_FILE, O_RDWR|O_CREAT, 0640);
    if(pid_fd == -1)
    {
        // Can't open lockfile
        ERROR_MSG("PID file (%s): fail to open (%s)", PID_FILE, strerror(errno));
        return -1;
    }
    if(lockf(pid_fd, F_TLOCK, 0) < 0)
    {
        // Can't lock file
        ERROR_MSG("PID file: fail to lock (%s)", strerror(errno));
        return -1;
    }
    // Get current PID
    sprintf(str, "%d\n", getpid());
    // Write PID to lockfile
    write(pid_fd, str, strlen(str));
    DEBUG_MSG("PID of daamon: %d", getpid());

    return 0;
}

void usage()
{
    printf("\n Usage: %s [OPTIONS]\n\n", app_name);
    printf("\n");
    printf("  Options:\n");
    printf("\n");
    printf("   -h --help                   Print this help\n");
    printf("\n");
    printf("   -i --init        filename   Init microblaze from file\n");
    printf("   (default the working directory filename: mb-server.bin)\n");
    printf("\n");
    printf("   -p --port        port num   Listen port for server\n");
    printf("   (default port number: 32000)\n");
    printf("\n");
    printf("   -s --start                  Start server of motor commands\n");
    printf("\n");
    printf("   -c --stop                   Stop server\n");
    printf("\n");
    printf("   -d --daemon                 Daemonize server\n");
    printf("\n");
    printf("   -l --log_file    filename   Write logs to the file\n");
    printf("   (default the working directory filename: ns7.log)\n");
    printf("\n");
    printf("   -m --microblaze  value      Enable or disable GPIO reset\n");
    printf("   (set value enable or disable)\n");
    printf("\n");
    printf("   -o --off_smooth             Turn off smooting algoritm\n");
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
        {"microblaze", required_argument, 0, 'm'},
        {"off_smooth", no_argument, 0, 'o'},
        {NULL, 0, 0, 0}
    };

    int value, option_index = 0;

    int init = 0;
    int port_num = PORT;
    char *init_filename;
    log_filename = strdup(LOG_FILE);


    int start_srv = 0;
    int stop_srv = 0;

    int enable_mb = 0;
    int disable_mb = 0;

    char *my_optarg;

    SMOOTHING_FLAG = 1;

    // Try to process all command line arguments
    while( (value = getopt_long(argc, argv, "i::p::l::scdhm:o", long_options, &option_index)) != -1) {
        switch(value) {
        case 'h':
            usage();
            return 0;
        case 'i':
            init = 1;
            if(optarg != NULL)
            {
                init_filename = strdup(optarg);
                break;
            }

            my_optarg = NULL;
            if(!optarg
               && optind < argc // make sure optind is valid
               && NULL != argv[optind] // make sure it's not a null string
               && '\0' != argv[optind][0] // ... or an empty string
               && '-' != argv[optind][0] // ... or another option
              )
            {
                my_optarg = argv[optind++];
            }
            if(my_optarg != NULL)
            {
                init_filename = strdup(my_optarg);
                break;
            }

            init_filename = INIT_FILE;
            break;
        case 'p':
            if(optarg != NULL)
            {
                port_num = atoi(optarg);
                break;
            }

            my_optarg = NULL;
            if(!optarg
               && optind < argc // make sure optind is valid
               && NULL != argv[optind] // make sure it's not a null string
               && '\0' != argv[optind][0] // ... or an empty string
               && '-' != argv[optind][0] // ... or another option
              )
            {
                my_optarg = argv[optind++];
            }

            if(my_optarg != NULL)
            {
                port_num = atoi(my_optarg);
                break;
            }

            port_num = PORT;
            break;
        case 's':
            start_srv= 1;
            break;
        case 'c':
            stop_srv = 1;
            break;
        case 'd':
            is_daemon = 1;
            break;
        case 'l':
            if(optarg != NULL)
            {
                log_filename = strdup(optarg);
                break;
            }
            my_optarg = NULL;
            if(!optarg
               && optind < argc // make sure optind is valid
               && NULL != argv[optind] // make sure it's not a null string
               && '\0' != argv[optind][0] // ... or an empty string
               && '-' != argv[optind][0] // ... or another option
              )
            {
                my_optarg = argv[optind++];
            }
            if(my_optarg != NULL)
                log_filename = strdup(my_optarg);
            break;
        case 'm':
            if(optarg != NULL)
            {
                if(strcmp(optarg, "enable") == 0)
                {
                    enable_mb = 1;
                    disable_mb = 0;
                    break;
                }
                if(strcmp(optarg, "disable") == 0)
                {
                    disable_mb = 1;
                    enable_mb = 0;
                    break;
                }
            }
            usage();
            return -1;
        case 'o':
            SMOOTHING_FLAG = 0;
            break;
        case '?':
            usage();
            return 0;
        default:
            break;
        }
    }


    // open log
    DEBUG_MSG("log file (%s): open", log_filename);
    if(open_log() == -1)
    {
        ERROR_MSG("log file (%s): fail to open (%s)", log_filename, strerror(errno));
        return -1;
    }


    if(init)
    {
        DEBUG_MSG("GPIO reset: disable");
        if(mb_stop() == -1)
        {
            ERROR_MSG("GPIO reset: fail to disable");
            return -1;
        }
        DEBUG_MSG("GPIO reset: disabled");

        // Load bin file to the memmory
        DEBUG_MSG("binary file (%s): load", init_filename);
        if (file_loader(init_filename) == -1)
        {
            ERROR_MSG("binary file (%s): fail to load", init_filename);
            return -1;
        }
        DEBUG_MSG("binary file (%s): loaded", init_filename);
    }

    if(enable_mb)
    {
        DEBUG_MSG("GPIO reset: enable");
        if(mb_start() == -1)
        {
            ERROR_MSG("GPIO reset: fail to enable");
            return -1;
        }
        DEBUG_MSG("GPIO reset: enabled");
    }

    if(disable_mb)
    {
        DEBUG_MSG("GPIO reset: disable");
        if(mb_stop() == -1)
        {
            ERROR_MSG("GPIO reset: fail to disable");
            return -1;
        }
        DEBUG_MSG("GPIO reset: disabled");
    }

    if(start_srv && stop_srv)
    {
        USER_MSG("server: restart");
    }

    int pid;
    char buf[256];

    pid_fd = open(PID_FILE, O_RDONLY);

    // stop server
    if(stop_srv)
    {
        DEBUG_MSG("server: stop");
        if(pid_fd == -1)
        {
            ERROR_MSG("server PID: fail to open");
        }
        else
        {
            if(read(pid_fd, buf, 256) == -1)
            {
                ERROR_MSG("server PID: fail to read");
                close(pid_fd);
                unlink(PID_FILE);
            }
            else
            {
                pid = atoi(buf);

                if(pid <= 0)
                {
                    ERROR_MSG("server PID: fail to check\n");
                    close(pid_fd);
                    unlink(PID_FILE);
                }

                DEBUG_MSG("stop signal (%u): send", pid);

                if(kill(pid, SIGINT) == -1)
                {
                    ERROR_MSG("stop signal: fail to send (%s)", strerror(errno));
                    unlink(PID_FILE);
                }

            }
        }
    }

    if(start_srv && (pid_fd != -1) && !stop_srv)
    {
        ERROR_MSG("server error: already running");
        return -1;
    }

    if(!start_srv) return 0;

    DEBUG_MSG("server: start");

    if(is_daemon)
    {
        DEBUG_MSG("process: daemonize");

        if(daemonize() == -1)
        {
            ERROR_MSG("process: fail to daemonize");
            return -1;
        }

        // Daemon will handle signals
        signal(SIGINT, handle_signal);
    }

    running = 1;

    if(init_command_socket(port_num) == -1)
    {
        DEBUG_MSG("server: closed");
        if(pid_fd != -1)
        {
            lockf(pid_fd, F_ULOCK, 0);
            close(pid_fd);
        }

        if(daemonized) unlink(PID_FILE);
    }

    DEBUG_MSG("programm: end");

    close_log();

    return 0;
}
