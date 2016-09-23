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

// save monitor pid
int set_pid_file(char *filename)
{
    int pid_fd;
    char str[256];

    pid_fd = open(filename, O_RDWR|O_CREAT, 0640);

    if(pid_fd == -1)
    {
        // Can't open lockfile
        return -1;
    }

    if(lockf(pid_fd, F_TLOCK, 0) == -1)
    {
        // Can't lock file
        return -1;
    }

    // Get current PID
    sprintf(str, "%d\n", getpid());
    // Write PID to lockfile
    write(pid_fd, str, strlen(str));

    close(pid_fd);

    return 0;
}

// load monitor pid
int open_pid_file(char *filename, int *pid)
{
    FILE *file;

    file = fopen(filename, "r");

    if (file)
    {
        fscanf(file, "%u", pid);
        fclose(file);
        return 0;
    }

    return -1;
}


// handle error and log it
static void signal_error(int sig, siginfo_t *si, void *ptr)
{
    write_log("we here!!!\n");
}

void stop_daemon_handler(int signum)
{
    write_log("we here!!!\n");
    close(clientfd);
    close(sockfd);

    return;
}


int work_proc()
{
    sigset_t         sigset;
	int              signo;
	int              status;

    //handle error signal
    /*
    struct sigaction sigact;
    sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = signal_error;

	sigemptyset(&sigact.sa_mask);

    sigaction(SIGFPE, &sigact, 0);
    sigaction(SIGILL, &sigact, 0);
    sigaction(SIGSEGV, &sigact, 0);
    sigaction(SIGBUS, &sigact, 0);
    */

    /*sigemptyset(&sigset);

    sigaddset(&sigset, SIGQUIT);
	sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);*/

  //  struct sigaction act;
    //sigset_t set;
    //memset(&act, 0, sizeof(act));
   // act.sa_handler = stop_daemon_handler;
   // sigemptyset(&act.sa_mask);
    //sigaddset(&set, SIGQUIT);
    //sigaddset(&set, SIGINT);
    //sigaddset(&set, SIGTERM);
   // sigaction(SIGQUIT, &act, 0);
   // sigaction(SIGINT, &act, 0);
   // sigaction(SIGTERM, &act, 0);

    struct sigaction sigact;
    // сигналы об ошибках в программе будут обрататывать более тщательно
    // указываем что хотим получать расширенную информацию об ошибках
    sigact.sa_flags = SA_SIGINFO;
    // задаем функцию обработчик сигналов
    sigact.sa_sigaction = stop_daemon_handler;

    sigemptyset(&sigact.sa_mask);

    // установим наш обработчик на сигналы

    sigaction(SIGQUIT, &sigact, 0); // ошибка FPU
    sigaction(SIGTERM, &sigact, 0); // ошибочная инструкция
    sigaction(SIGINT, &sigact, 0); // ошибка доступа к памяти

    write_log("[DAEMON] Started\n");

    status = init_command_socket(32000);

    write_log("[DAEMON] Stop\n");

    if(status == -1)
    {
        return DAEMON_NEED_WORK;
    }

    return DAEMON_NEED_TERMINATE;
}



int monitor_proc()
{
    int pid;
    int status;
    int need_start = 1;
    int start_attempt;

	sigset_t  sigset;
	siginfo_t siginfo;

	sigemptyset(&sigset);

    sigaddset(&sigset, SIGQUIT);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGCHLD);

	sigprocmask(SIG_BLOCK, &sigset, NULL);

    status = set_pid_file(PID_FILE); // save monitor pid

    if(status == -1)
    {
        write_log("[MONITOR] Faild to start daemon\n");
        return -1;
    }

    write_log("[MONITOR] Start\n");

    while(1)
	{
        if(need_start)
		{
            pid = fork();
		}

		need_start = 1;
        start_attempt = DAEMON_START_ATTEMP;

        if(pid == -1)
        {
            write_log("[MONITOR] Fork failed (%s)\n", strerror(errno));
		}
        else if (!pid) // daemon
		{
            status = work_proc(); // init working thread
			exit(status);
		}
        else // monitor wait for signal
		{
			sigwaitinfo(&sigset, &siginfo);

            write_log("[MONITOR] Signal %s\n", strsignal(siginfo.si_signo));

            if(siginfo.si_signo == SIGCHLD)
			{
				wait(&status);

				status = WEXITSTATUS(status);

                if(status == DAEMON_NEED_TERMINATE)
				{
                    write_log("[MONITOR] Daemon stopped\n");
					break;
				}
                else if(status == DAEMON_NEED_WORK)
				{
                    write_log("[MONITOR] Daemon restarting...%d of %d\n", (DAEMON_START_ATTEMP - (--start_attempt)), DAEMON_START_ATTEMP);
                    if(start_attempt == 0)
                    {
                        write_log("[MONITOR] Can't start daemon... %d", DAEMON_START_ATTEMP);
                        break;
                    }

                    continue; // try to start again
				}
			}
            else
			{
                status = kill(pid, SIGTERM);

                if(status == -1)
                {
                    write_log("[MONITOR] Faild to close daemon %s\n", strerror(errno));
                }
                else
                {
                    status = 0;
                }

				break;
			}
		}
	}

    write_log("[MONITOR] Stopped\n");

	unlink(PID_FILE);

	return status;
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
        log("Fail to create new session: %s", strerror(errno));
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
        log("Fail to change working dir: %s", strerror(errno));
        return -1;
    }

    // Close all open file descriptors
    for(fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }

    /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
    /*stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");*/

    // Try to write PID of daemon to lockfile
    char str[256];
    int pid_fd;

    pid_fd = open(PID_FILE, O_RDWR|O_CREAT, 0640);
    if(pid_fd < 0)
    {
        // Can't open lockfile
        log("Fail to open pid file: %s", strerror(errno));
        return -1;
    }
    if(lockf(pid_fd, F_TLOCK, 0) < 0)
    {
        // Can't lock file
        log("Fail to lock pid file: %s", strerror(errno));
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

    int status;
    int pid;

    status = open_pid_file(PID_FILE, &pid);

    if(stop) // stop daemon
    {
        if(status == -1)
        {
            printf("Error: faild to connect daemon process\n");
            printf("Try to start first!\n");
            exit(-1);
        }

        if(pid <= 0) {
            printf("Error: wrong pid of daemon\n");
            unlink(PID_FILE);
            exit(-1);
        }

        printf("Try to stop daemon (%u)\n", pid);

        status = kill(pid, SIGQUIT);

        if(status == -1)
        {
            printf("kill: %s\n", strerror(errno));
            unlink(PID_FILE);
            exit(-1);
        }

        printf("Daemon stoped!\n");

        exit(-1);
    }

    if(start && (status != -1))
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
        exit(-1);
    }


    return status;
}

