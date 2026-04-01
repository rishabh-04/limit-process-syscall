#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#define __NR_limit_process 462
#define LP_ACTION_KILL     0
#define LP_ACTION_THROTTLE 1
#define LP_ACTION_TERM     2

long limit_process(pid_t pid, unsigned long cpu_limit,
                   unsigned long memory_limit, int action)
{
    return syscall(__NR_limit_process, pid, cpu_limit, memory_limit, action);
}

int main(void)
{
    pid_t child;
    int status;
    long ret;

    printf("=== Test 1: SIGKILL ===\n");
    child = fork();
    if (child == 0) { while(1); }
    sleep(1);
    ret = limit_process(child, 1, 0, LP_ACTION_KILL);
    printf("returned: %ld\n", ret);
    waitpid(child, &status, 0);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL)
        printf("PASS: killed\n");

    printf("\n=== Test 2: THROTTLE ===\n");
    child = fork();
    if (child == 0) { while(1); }
    sleep(1);
    ret = limit_process(child, 1, 0, LP_ACTION_THROTTLE);
    printf("returned: %ld\n", ret);
    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    if (ret == 1) printf("PASS: throttled\n");

    printf("\n=== Test 3: SIGTERM ===\n");
    child = fork();
    if (child == 0) { while(1); }
    sleep(1);
    ret = limit_process(child, 1, 0, LP_ACTION_TERM);
    printf("returned: %ld\n", ret);
    waitpid(child, &status, 0);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM)
        printf("PASS: terminated\n");

    printf("\nAll tests done.\n");
    return 0;
}

