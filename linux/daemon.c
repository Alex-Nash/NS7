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
    FILE *file;

    file = fopen(filename, "w+");
    if (file)
    {
        fprintf(file, "%u", getpid());
        fclose(file);
        return 0;
    }

    return -1;
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
        return -1;
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
            exit(1);
        }

        if(pid <= 0) {
            printf("Error: wrong pid of daemon\n");
            unlink(PID_FILE);
            exit(1);
        }

        printf("Try to stop daemon (%u)\n", pid);

        status = kill(pid, SIGQUIT);

        if(status == -1)
        {
            printf("kill: %s\n", strerror(errno));
            unlink(PID_FILE);
            exit(1);
        }

        printf("Daemon stoped!\n");

        exit(1);
    }

    if(start && (status != -1))
    {
        printf("Daemon is already running\n");
        exit(1);
    }

    pid = fork(); // create monitor process

    if (pid == -1)
    {
        printf("Fail to start daemon: %s\n", strerror(errno));
        return -1;
    }
    else if (pid > 0)
    {
        printf("Starting daemon....\nSee log for more information\n");
        return 0;
    }

    // monitor
    int sid;

    // open log
    if(open_log() == -1)
    {
        printf("Fail to open log: %s\n", strerror(errno));
        return -1;
    }

    umask(0); // all privilage for created file
    sid = setsid(); // create new session
    if(sid == -1)
    {
        log("Fail to create new session: %s\n", strerror(errno));
        return -1;
    }

    if(chdir("/") == -1) // go to disk root
    {
        log("Fail to change working dir: %s", strerror(errno));
        return -1;
    }

    // close input/output descriptor
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    status = monitor_proc();

    return status;
}

