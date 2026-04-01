#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/cputime.h>
#include <linux/pid.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/signal.h>
#include <linux/cred.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

static unsigned long get_process_cpu_ms(struct task_struct *task)
{
    u64 utime, stime;
    utime = task->utime;
    stime = task->stime;
    return (unsigned long)((utime + stime) / 1000000ULL);
}

static unsigned long get_process_memory_kb(struct task_struct *task)
{
    struct mm_struct *mm;
    unsigned long rss = 0;
    mm = get_task_mm(task);
    if (!mm)
        return 0;
    rss = get_mm_rss(mm);
    mmput(mm);
    return rss * (PAGE_SIZE / 1024);
}

SYSCALL_DEFINE4(limit_process,
                pid_t,          pid,
                unsigned long,  cpu_limit,
                unsigned long,  memory_limit,
                int,            action)
{
    struct task_struct *task;
    unsigned long cpu_used_ms;
    unsigned long mem_used_kb;
    int ret = 0;

    if (pid <= 0 || action < 0 || action > 2)
        return -EINVAL;

    rcu_read_lock();
    task = find_task_by_vpid(pid);
    if (!task) {
        rcu_read_unlock();
        return -ESRCH;
    }
    get_task_struct(task);
    rcu_read_unlock();

    if (!capable(CAP_KILL) &&
        !uid_eq(task_uid(task), current_uid())) {
        put_task_struct(task);
        return -EPERM;
    }

    cpu_used_ms = get_process_cpu_ms(task);
    mem_used_kb  = get_process_memory_kb(task);

    if ((cpu_limit    > 0 && cpu_used_ms > cpu_limit) ||
        (memory_limit > 0 && mem_used_kb  > memory_limit)) {

        switch (action) {
        case 0:
            ret = send_sig(SIGKILL, task, 0);
            break;
        case 1:
            set_user_nice(task, min(task_nice(task) + 10, 19));
            ret = 0;
            break;
        case 2:
            ret = send_sig(SIGTERM, task, 0);
            break;
        }
        put_task_struct(task);
        return (ret < 0) ? ret : 1;
    }

    put_task_struct(task);
    return 0;
}
