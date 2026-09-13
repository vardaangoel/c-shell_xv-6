#define _DEFAULT_SOURCE
#include"activities.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<errno.h>
#include<signal.h>
#include<time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include<sys/user.h>

typedef struct{
    pid_t pid;char name[256];
    int state,exit,printed;
}process;
typedef struct{
    int job_id,bg,active,num;
    pid_t pgid;
process prs[200];
char cmd[1000];
}job;

job jobs[2000];
int job_count=0;
volatile sig_atomic_t timed_out = 0;

void sigalrm_handler(int sig) {
    (void)sig;
    timed_out = 1;
}

void update_state(pid_t in_pid, int in_status){
if (in_pid>0){
            for (int i=0;i<job_count;i++){
            if (!jobs[i].active)continue;
            for (int j=0;j<jobs[i].num;j++){
                if (jobs[i].prs[j].pid==in_pid){
                    if (WIFEXITED(in_status)){jobs[i].prs[j].state=2;jobs[i].prs[j].exit=1;}
                    else if (WIFSIGNALED(in_status))
{
jobs[i].prs[j].state=2;jobs[i].prs[j].exit=0;
}                   else if (WIFSTOPPED(in_status)){jobs[i].prs[j].state=1;}
else if (WIFCONTINUED(in_status)){jobs[i].prs[j].state=0;}
        }}
        int flag=0;
        for (int k=0;k<jobs[i].num;k++){
if (jobs[i].prs[k].state!=2){flag=1;break;}
        
            }
            if (flag==0)jobs[i].active=0;
        }
}
else{
    int status;pid_t pid;
    while((pid=waitpid(-1,&status,WNOHANG|WUNTRACED|WCONTINUED))>0){
        for (int i=0;i<job_count;i++){
            if (!jobs[i].active)continue;
            for (int j=0;j<jobs[i].num;j++){
                if (jobs[i].prs[j].pid==pid){
                    if (WIFEXITED(status)){jobs[i].prs[j].state=2;jobs[i].prs[j].exit=1;}
                    else if (WIFSIGNALED(status))
{
jobs[i].prs[j].state=2;jobs[i].prs[j].exit=0;
}                   else if (WIFSTOPPED(status)){jobs[i].prs[j].state=1;}
else if (WIFCONTINUED(status)){jobs[i].prs[j].state=0;}
        }}
        int flag=0;
        for (int k=0;k<jobs[i].num;k++){
if (jobs[i].prs[k].state!=2){flag=1;break;}
        
            }
            if (flag==0)jobs[i].active=0;
        }
    }
}
}

void sigchld(int sig){
int saved=errno;
update_state(-1,0);
errno=saved;
}
void print_prs(){
update_state(-1,0);
for (int i=0;i<job_count;i++){
    if (jobs[i].bg){
        for (int j=0;j<jobs[i].num;j++){
            if (jobs[i].prs[j].state==2&&!jobs[i].prs[j].printed){
                if (jobs[i].prs[j].exit){
                    printf("%s with pid %d exited normally\n",jobs[i].prs[j].name,(int)jobs[i].prs[j].pid);
                }
                else {
                    printf("%s with pid %d exited abnormally\n",jobs[i].prs[j].name,(int)jobs[i].prs[j].pid);
                }
                fflush(stdout);
                jobs[i].prs[j].printed=1;
            }
        }
    }
}
}


void print_activities(){
update_state(-1,0);
for (int i=0;i<job_count;i++){
    if (!jobs[i].active)continue;
    int flag=0;
    for (int j=0;j<jobs[i].num;j++){
        if (jobs[i].prs[j].state!=2){
            flag=1;break;
        }
    }
if (flag==0)continue;
printf("[%d] pgid %d\n",jobs[i].job_id,(int)jobs[i].pgid);
for (int j=0;j<jobs[i].num;j++ ){
    if (jobs[i].prs[j].state!=2){
        if (jobs[i].prs[j].state==0){
            printf("  %d %s Running\n",(int)jobs[i].prs[j].pid,jobs[i].prs[j].name);
        }
        else {
            printf("  %d %s Stopped\n",(int)jobs[i].prs[j].pid,jobs[i].prs[j].name);
        }
    }
}
fflush(stdout);
}
}


void add_job(int job_id,pid_t pgid,int num,pid_t* pids, char cmd_names[][256],int bg,char*cmd){
    int i=job_count++;
    jobs[i].job_id=job_id;
    jobs[i].pgid=pgid;
    jobs[i].num=num;
    jobs[i].bg=bg;
    jobs[i].active=1;
    if (cmd){strcpy(jobs[i].cmd,cmd);}
    else {jobs[i].cmd[0]='\0';}
    for (int j=0;j<num;j++){
        jobs[i].prs[j].pid=pids[j];
        strcpy(jobs[i].prs[j].name,cmd_names[j]);
        jobs[i].prs[j].state=0;
        jobs[i].prs[j].exit=1;
        jobs[i].prs[j].printed=0;
    }
}

int stopped(){
    update_state(-1,0);
    for (int i=0;i<job_count;i++){
        if (!jobs[i].active)continue;
        for (int j=0;j<jobs[i].num;j++){
            if (jobs[i].prs[j].state==1)return 1;
        }
    }
    return 0;
}

void send_sighup(){
    update_state(-1,0);
    for (int i=0;i<job_count;i++){
        if (jobs[i].active&&jobs[i].pgid>0){
         kill(-jobs[i].pgid,SIGHUP);
        }
    }
}


void resume(char**input,int count){
    if (count<3||input[1]==NULL||input[1][0]!='%'){
        printf("resume: invalid syntax\n");
        fflush(stdout);
        return;

    }
    char*num=input[1]+1;
    if (*num=='\0'){
        printf("resume: invalid syntax\n");
        fflush(stdout);
        return;
    }
    for (int i=0;num[i]!='\0';i++){
        if (num[i]<'0'||num[i]>'9'){
                    printf("resume: invalid syntax\n");
        fflush(stdout);
        return;
        }
    }
    int job_id=atoi(num);char fgbg[3]="";int timeout=0;
    if (strcmp(input[2],"bg")==0){strcat(fgbg,"bg");

        if (count!=3){        printf("resume: invalid syntax\n");
        fflush(stdout);
        return;}
    }
    else if (strcmp(input[2],"fg")==0){
strcat(fgbg,"fg");
if (count==3){timeout=0;}
else if(count==5){
    if (strcmp(input[3],"--timeout")!=0||input[4]==NULL){
        printf("resume: invalid syntax\n");
        fflush(stdout);
        return;
    }
    for (int i=0;input[4][i]!='\0';i++){
if (input[4][i]<'0'||input[4][i]>'9'){
    printf("resume: invalid syntax\n");
        fflush(stdout);
        return;
}
    }
    timeout=atoi(input[4]);
    if (timeout<=0){
        printf("resume: invalid syntax\n");
        fflush(stdout);
        return;
    }
}
else {printf("resume: invalid syntax\n");
        fflush(stdout);
        return;}
    }
    else {
        printf("resume: invalid syntax\n");
        fflush(stdout);
        return;
    }

    update_state(-1,0);
    int job_i=-1;
    for (int i=0;i<job_count;i++){
if (jobs[i].active&&jobs[i].job_id==job_id) {
            job_i= i;
            break;
        }
    }
    if (job_i==-1) {printf("resume: no such job\n");
        fflush(stdout);
        return;
    }
if (strcmp(fgbg,"bg")==0) {
        jobs[job_i].bg=1;
        for (int k=0; k<jobs[job_i].num;k++) {
            if (jobs[job_i].prs[k].state!=2) {
                jobs[job_i].prs[k].state=0;
            }
        }
        kill(-jobs[job_i].pgid, SIGCONT);
        printf("[%d] + Running    %s\n",jobs[job_i].job_id,jobs[job_i].cmd);
        fflush(stdout);
        return;
    }
 if (strcmp(fgbg,"fg")==0){
        printf("%s\n",jobs[job_i].cmd);
        fflush(stdout);
        tcsetpgrp(STDIN_FILENO, jobs[job_i].pgid);
        for (int k=0;k<jobs[job_i].num;k++) {
            if (jobs[job_i].prs[k].state != 2) {
                jobs[job_i].prs[k].state=0;
            }
        }
        kill(-jobs[job_i].pgid,SIGCONT);   


      struct sigaction sa_old, sa_alrm;
        if (timeout>0) {
            timed_out=0;
            sa_alrm.sa_handler = sigalrm_handler;
            sigemptyset(&sa_alrm.sa_mask);
            sa_alrm.sa_flags=0;
            sigaction(SIGALRM,&sa_alrm,&sa_old);
            alarm(timeout); 
        }
        int stopped=0;
        for (int k=0;k<jobs[job_i].num;k++) {
            if (jobs[job_i].prs[k].state == 2) continue;
            int status;
            pid_t res;
            while (1) {
                res = waitpid(jobs[job_i].prs[k].pid, &status, WUNTRACED);
                if (res > 0) {
                    update_state(res, status);
                    if (WIFSTOPPED(status)) stopped=1;
                    break;
                } else if (res < 0) {
                    if (errno == EINTR) {
                        if (timed_out) break;
                        continue;
                    }
                    break;
                }
            }
            if (timed_out) break;
        }
        if (timeout>0) {
            alarm(0);
            sigaction(SIGALRM, &sa_old, NULL);
        }
        if (timed_out) {
            kill(-jobs[job_i].pgid, SIGTERM);
            printf("resume: job timed out\n");
            fflush(stdout);
            update_state(-1, 0);
        }
        tcsetpgrp(STDIN_FILENO,getpid());
        if (stopped&&!timed_out) {
            printf("[%d] + Stopped    %s\n", jobs[job_i].job_id, jobs[job_i].cmd);
            fflush(stdout);
        }
        print_prs();
    }
    
}


void ping(char**input,int count){
    if (count!=3||input[1]==NULL||input[2]==NULL){
        printf("ping: invalid syntax\n");
        fflush(stdout);
        return;
    }
    if (input[2][0]=='\0'){
    printf("ping: invalid syntax\n");
            fflush(stdout);
            return;
}

for (int i=0;input[2][i]!='\0';i++){
        if (input[2][i]<'0'||input[2][i]>'9'){
            printf("ping: invalid syntax\n");
            fflush(stdout);
            return;
        }
    }
    int sig=atoi(input[2]);
    
    int Sig=sig%64;
    update_state(-1,0);
    char*target=input[1];
    if (target[0]=='%'){
        char*num=target+1;
        if (*num=='\0'){
            printf("ping: no such process found\n");
fflush(stdout);
            return;
        }
        for (int i=0;num[i]!='\0';i++){
            if (num[i]<'0'||num[i]>'9'){
                printf("ping: no such process found\n");
fflush(stdout);
                return;
            }
        }
        int job_id=atoi(num);
        int job_i=-1;
        for (int i=0;i<job_count;i++){
            if (jobs[i].active&&jobs[i].job_id==job_id) {
                job_i= i;
                break;
            }
        }
        if (job_i==-1) {printf("ping: no such process found\n");
fflush(stdout);
            return;
        }
        kill(-jobs[job_i].pgid,Sig);
        printf("Sent signal %d to %s\n",sig,target);
        fflush(stdout);
    }
    else {
        for (int i=0;target[i]!='\0';i++){
            if (target[i]<'0'||target[i]>'9'){
                printf("ping: no such process found\n");
fflush(stdout);
                return; 
    }
        }
        pid_t pid=(pid_t)atoi(target);
        int found=0;
        for (int i=0;i<job_count;i++){
            if (!jobs[i].active)continue;
            for (int j=0;j<jobs[i].num;j++){
                if (jobs[i].prs[j].pid==pid&&jobs[i].prs[j].state!=2){
                    found=1;break;
                }
            }
            if (found) break;
        }
        if (!found){
            printf("ping: no such process found\n");
            fflush(stdout);
            return;
        }
        kill(pid,Sig);
        printf("Sent signal %d to %s\n",sig,target);
        fflush(stdout);
    }
    }
int compare(const void*a,const void*b){
    return (*(int*)a-*(int*)b);
}
char* type(char*path){
    struct stat st;
    if (stat(path,&st)==0){
    if (S_ISREG(st.st_mode)){
        return "REG";}
    if (S_ISDIR(st.st_mode)){
        return "DIR";}
    if (S_ISLNK(st.st_mode)){
        return "LNK";}
    if (S_ISCHR(st.st_mode)){
        return "CHR";}
    if (S_ISBLK(st.st_mode)){
        return "BLK";}
    if (S_ISFIFO(st.st_mode)){
        return "FIFO";}
    if (S_ISSOCK(st.st_mode)){
        return "SOCK";}
    }
    
        if (strncmp(path,"socket:",7)==0){
            return "SOCK";
        }
        if (strncmp(path,"pipe:",5)==0){
            return "FIFO";
        }

    return "REG";
}

    void spy(char**input,int count){
if (count>=3){
    printf("spy: invalid syntax\n");
    fflush(stdout); return;
}
pid_t pid;
if (count==1){
    pid=getpid();   
    }
    else{
        char*target=input[1];
        if (target[0]=='\0'){
            printf("spy: invalid syntax\n");
            fflush(stdout);
            return;
        }
        for (int i=0;target[i]!='\0';i++){
            if (target[i]<'0'||target[i]>'9'){
                printf("spy: no such process\n");
                fflush(stdout);
                return;
            }
        }
        pid=(pid_t)atoi(target);}
        char proc[256];
        char str[100];
        sprintf(str,"%d",(int)pid);
        strcpy(proc,"/proc/");
        strcat(proc,str);
        if (access(proc,F_OK)!=0){
            printf("spy: no such process\n");
            fflush(stdout);
            return;
        }
printf("%-7s %-6s %-6s %s\n","PID","FD","TYPE","PATH");
char path[1000];
char link[1000];
ssize_t len;
strcpy(link,proc);
strcat(link,"/cwd");
len=readlink(link,path,sizeof(path)-1);
if (len!=-1){
    path[len]='\0';
    char*t=type(path);
    printf("%-7d %-6s %-6s %s\n",(int)pid,"cwd",t,path);
}
strcpy(link,proc);
strcat(link,"/exe");
len=readlink(link,path,sizeof(path)-1);
if (len!=-1){
    path[len]='\0';
    char*t=type(path);
    printf("%-7d %-6s %-6s %s\n",(int)pid,"txt",t,path);
}

strcpy(link,proc);
strcat(link,"/maps");
FILE*fp=fopen(link,"r");
if (fp!=NULL){
    char line[1000];
    int s=0;
    static char done[4000][1000];
    while(fgets(line,sizeof(line),fp)!=NULL){
        char*slash=strchr(line,'/');    
        if (slash!=NULL){
            char*p=slash;
            while(*p!='\n'&&*p!='\0')p++;
            *p='\0';
        }
        else continue;
        int temp=0;
        for (int i=0;i<s;i++){
            if (strcmp(done[i],slash)==0){temp=1;break;}
        }
        if (temp==0){
            strcpy(done[s],slash);
            s++;
            char*t=type(slash);
            printf("%-7d %-6s %-6s %s\n",(int)pid,"mem",t,slash);   
        }
    }
    fclose(fp);
}
strcpy(link,proc);
strcat(link,"/fd");
DIR*dir=opendir(link);
if (dir!=NULL){
    struct dirent*entry;
    int fd[1000];
    int fd_count=0;
    while((entry=readdir(dir))!=NULL){
        int num=1;
        if(entry->d_name[0]=='\0'){num=0;}
        for (int i=0;entry->d_name[i]!='\0';i++){
            if (entry->d_name[i]<'0'||entry->d_name[i]>'9'){num=0;break;}
        }
        if (num){
            fd[fd_count++]=atoi(entry->d_name);
        }       
    }
    closedir(dir);
    qsort(fd,fd_count,sizeof(int),compare);
    for (int i=0;i<fd_count;i++){
        char fd_path[1000];
        sprintf(fd_path,"%d",fd[i]);
        strcpy(link,proc);
        strcat(link,"/fd/");
        strcat(link,fd_path);
        len=readlink(link,path,sizeof(path)-1); 
        if (len!=-1){
            path[len]='\0';
            char*t=type(path);
            printf("%-7d %-6d %-6s %s\n",(int)pid,fd[i],t,path);
        }
    }
}   fflush(stdout); }


char* syscall_names[]={
    [0] = "read", [1] = "write", [2] = "open", [3] = "close", [4] = "stat",
    [5] = "fstat", [6] = "lstat", [7] = "poll", [8] = "lseek", [9] = "mmap",
    [10] = "mprotect", [11] = "munmap", [12] = "brk", [13] = "rt_sigaction",
    [14] = "rt_sigprocmask", [15] = "rt_sigreturn", [16] = "ioctl", [17] = "pread64",
    [18] = "pwrite64", [19] = "readv", [20] = "writev", [21] = "access",
    [22] = "pipe", [23] = "select", [24] = "sched_yield", [25] = "mremap",
    [26] = "msync", [27] = "mincore", [28] = "madvise", [29] = "shmget",
    [30] = "shmat", [31] = "shmdt", [32] = "dup", [33] = "dup2", [34] = "pause",
    [35] = "nanosleep", [36] = "getitimer", [37] = "alarm", [38] = "setitimer",
    [39] = "getpid", [40] = "sendfile", [41] = "socket", [42] = "connect",
    [43] = "accept", [44] = "sendto", [45] = "recvfrom", [46] = "sendmsg",
    [47] = "recvmsg", [48] = "shutdown", [49] = "bind", [50] = "listen",
    [51] = "getsockname", [52] = "getpeername", [53] = "socketpair", [54] = "setsockopt",
    [55] = "getsockopt", [56] = "clone", [57] = "fork", [58] = "vfork",
    [59] = "execve", [60] = "exit", [61] = "wait4", [62] = "kill", [63] = "uname",
    [64] = "semget", [65] = "semop", [66] = "semctl", [67] = "shmctl",
    [68] = "msgget", [69] = "msgsnd", [70] = "msgrcv", [71] = "msgctl",
    [72] = "fcntl", [73] = "flock", [74] = "fsync", [75] = "fdatasync",
    [76] = "truncate", [77] = "ftruncate", [78] = "getdents", [79] = "getcwd",
    [80] = "chdir", [81] = "fchdir", [82] = "rename", [83] = "mkdir",
    [84] = "rmdir", [85] = "creat", [86] = "link", [87] = "unlink",
    [88] = "symlink", [89] = "readlink", [90] = "chmod", [91] = "fchmod",
    [92] = "chown", [93] = "fchown", [94] = "lchown", [95] = "umask",
    [96] = "gettimeofday", [97] = "getrlimit", [98] = "getrusage", [99] = "sysinfo",
    [100] = "times", [101] = "ptrace", [102] = "getuid", [103] = "syslog",
    [104] = "getgid", [105] = "setuid", [106] = "setgid", [107] = "geteuid",
    [108] = "getegid", [109] = "setpgid", [110] = "getppid", [111] = "getpgrp",
    [112] = "setsid", [113] = "setreuid", [114] = "setregid", [115] = "getgroups",
    [116] = "setgroups", [117] = "setresuid", [118] = "getresuid", [119] = "setresgid",
    [120] = "getresgid", [121] = "getpgid", [122] = "setfsuid", [123] = "setfsgid",
    [124] = "getsid", [125] = "capget", [126] = "capset", [127] = "rt_sigpending",
    [128] = "rt_sigtimedwait", [129] = "rt_sigqueueinfo", [130] = "rt_sigsuspend",
    [131] = "sigaltstack", [132] = "utime", [133] = "mknod", [134] = "uselib",
    [135] = "personality", [136] = "ustat", [137] = "statfs", [138] = "fstatfs",
    [139] = "sysfs", [140] = "getpriority", [141] = "setpriority", [142] = "sched_setparam",
    [143] = "sched_getparam", [144] = "sched_setscheduler", [145] = "sched_getscheduler",
    [146] = "sched_get_priority_max", [147] = "sched_get_priority_min",
    [148] = "sched_rr_get_interval", [149] = "mlock", [150] = "munlock",
    [151] = "mlockall", [152] = "munlockall", [153] = "vhangup", [154] = "modify_ldt",
    [155] = "pivot_root", [156] = "_sysctl", [157] = "prctl", [158] = "arch_prctl",
    [159] = "adjtimex", [160] = "setrlimit", [161] = "chroot", [162] = "sync",
    [163] = "acct", [164] = "settimeofday", [165] = "mount", [166] = "umount2",
    [167] = "swapon", [168] = "swapoff", [169] = "reboot", [170] = "sethostname",
    [171] = "setdomainname", [172] = "iopl", [173] = "ioperm", [174] = "create_module",
    [175] = "init_module", [176] = "delete_module", [177] = "get_kernel_syms",
    [178] = "query_module", [179] = "quotactl", [180] = "nfsservctl", [181] = "getpmsg",
    [182] = "putpmsg", [183] = "afs_syscall", [184] = "tuxcall", [185] = "security",
    [186] = "gettid", [187] = "readahead", [188] = "setxattr", [189] = "lsetxattr",
    [190] = "fsetxattr", [191] = "getxattr", [192] = "lgetxattr", [193] = "fgetxattr",
    [194] = "listxattr", [195] = "llistxattr", [196] = "flistxattr", [197] = "removexattr",
    [198] = "lremovexattr", [199] = "fremovexattr", [200] = "tkill", [201] = "time",
    [202] = "futex", [203] = "sched_setaffinity", [204] = "sched_getaffinity",
    [205] = "set_thread_area", [206] = "io_setup", [207] = "io_destroy",
    [208] = "io_getevents", [209] = "io_submit", [210] = "io_cancel",
    [211] = "get_thread_area", [212] = "lookup_dcookie", [213] = "epoll_create",
    [214] = "epoll_ctl_old", [215] = "epoll_wait_old", [216] = "remap_file_pages",
    [217] = "getdents64", [218] = "set_tid_address", [219] = "restart_syscall",
    [220] = "semtimedop", [221] = "fadvise64", [222] = "timer_create",
    [223] = "timer_settime", [224] = "timer_gettime", [225] = "timer_getoverrun",
    [226] = "timer_delete", [227] = "clock_settime", [228] = "clock_gettime",
    [229] = "clock_getres", [230] = "clock_nanosleep", [231] = "exit_group",
    [232] = "epoll_wait", [233] = "epoll_ctl", [234] = "tgkill", [235] = "utimes",
    [236] = "vserver", [237] = "mbind", [238] = "set_mempolicy", [239] = "get_mempolicy",
    [240] = "mq_open", [241] = "mq_unlink", [242] = "mq_timedsend",
    [243] = "mq_timedreceive", [244] = "mq_notify", [245] = "mq_getsetattr",
    [246] = "kexec_load", [247] = "waitid", [248] = "add_key", [249] = "request_key",
    [250] = "keyctl", [251] = "ioprio_set", [252] = "ioprio_get", [253] = "inotify_init",
    [254] = "inotify_add_watch", [255] = "inotify_rm_watch", [256] = "migrate_pages",
    [257] = "openat", [258] = "mkdirat", [259] = "mknodat", [260] = "fchownat",
    [261] = "futimesat", [262] = "newfstatat", [263] = "unlinkat", [264] = "renameat",
    [265] = "linkat", [266] = "symlinkat", [267] = "readlinkat", [268] = "fchmodat",
    [269] = "faccessat", [270] = "pselect6", [271] = "ppoll", [272] = "unshare",
    [273] = "set_robust_list", [274] = "get_robust_list", [275] = "splice",
    [276] = "tee", [277] = "sync_file_range", [278] = "vmsplice", [279] = "move_pages",
    [280] = "utimensat", [281] = "epoll_pwait", [282] = "signalfd",
    [283] = "timerfd_create", [284] = "eventfd", [285] = "fallocate",
    [286] = "timerfd_settime", [287] = "timerfd_gettime", [288] = "accept4",
    [289] = "signalfd4", [290] = "eventfd2", [291] = "epoll_create1", [292] = "dup3",
    [293] = "pipe2", [294] = "inotify_init1", [295] = "preadv", [296] = "pwritev",
    [297] = "rt_tgsigqueueinfo", [298] = "perf_event_open", [299] = "recvmmsg",
    [300] = "fanotify_init", [301] = "fanotify_mark", [302] = "prlimit64",
    [303] = "name_to_handle_at", [304] = "open_by_handle_at", [305] = "clock_adjtime",
    [306] = "syncfs", [307] = "sendmmsg", [308] = "setns", [309] = "getcpu",
    [310] = "process_vm_readv", [311] = "process_vm_writev", [312] = "kcmp",
    [313] = "finit_module", [314] = "sched_setattr", [315] = "sched_getattr",
    [316] = "renameat2", [317] = "seccomp", [318] = "getrandom", [319] = "memfd_create",
    [320] = "kexec_file_load", [321] = "bpf", [322] = "execveat", [323] = "userfaultfd",
    [324] = "membarrier", [325] = "mlock2", [326] = "copy_file_range",
    [327] = "preadv2", [328] = "pwritev2", [329] = "pkey_mprotect", [330] = "pkey_alloc",
    [331] = "pkey_free", [332] = "statx", [333] = "io_pgetevents", [334] = "rseq"
};


typedef struct{
    int num;
    char name[1000];
    int count;
    double time;
    int first;
}system_call;

void call_name(int num,char*name,int size){int last=(int)sizeof(syscall_names)/sizeof(syscall_names[0]);
    if (num>=0&&num<last&&syscall_names[num]!=NULL){
        strncpy(name,syscall_names[num],size-1);
        name[size-1]='\0';
    }
    else {
        sprintf(name,"syscall_%d",num);
    }
}
int compare2(const void*a,const void*b){
    system_call*a1=(system_call*)a;
    system_call*a2=(system_call*)b;
if (a1->count!=a2->count){
    return a2->count-a1->count;
}
return a1->first-a2->first;
}

void snoop(char**input,int count){
    if (count<2){
        printf("snoop: invalid syntax\n");
        fflush(stdout);
        return;
    }
    pid_t pid=-1;
    if (strcmp(input[1],"-p")==0){
        if (count!=3||input[2]==NULL||input[2][0]=='\0'){
            printf("snoop: invalid syntax\n");
            fflush(stdout);
            return;
        }
        for (int i=0;input[2][i]!='\0';i++){
            if (input[2][i]<'0'||input[2][i]>'9'){
                printf("snoop: no such process\n");
                fflush(stdout);
                return;
            }
        }
        pid=(pid_t)atoi(input[2]);
        if (ptrace(PTRACE_ATTACH,pid,NULL,NULL)==-1){
            printf("snoop: no such process\n");
            fflush(stdout);
            return;
        }}
        else{
            char*cmd=input[1];
            int found=0;
            if (strchr(cmd,'/')!=NULL){
                if (access(cmd,X_OK)==0){
found=1;
                }}
                else{    char*path=getenv("PATH");
                    if (!path)path="/bin:/usr/bin";
                    char copy[2000];
                    strncpy(copy,path,sizeof(copy)-1);
                    copy[sizeof(copy)-1]='\0';
                    char*dir=strtok(copy,":");
                    while(dir!=NULL){
                        char full[1000];
                        strcpy(full,dir);
                        strcat(full,"/");
                        strcat(full,cmd);
                        if (access(full,X_OK)==0){found=1;break;}
                        dir=strtok(NULL,":");
                    }}
            
        
        if (!found){
            printf("snoop: command not found\n");
            fflush(stdout);
            return;
        }
        pid_t child=fork();
        if (child==0){
            ptrace(PTRACE_TRACEME,0,NULL,NULL);
            execvp(cmd,input+1);
            exit(1);
        }
        else if (child<0){
            printf("snoop: fork failed\n");
            fflush(stdout);
            return;
        }
        else{pid=child;}}

        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &oldmask);    
        int status;
        if (waitpid(pid, &status, 0) == -1) {
sigprocmask(SIG_SETMASK, &oldmask, NULL);
            fflush(stdout);
            return;
        }
        ptrace(PTRACE_SETOPTIONS, pid, 0, (void*)(long)PTRACE_O_TRACESYSGOOD);
        system_call calls[1000];
        int call_count=0;
        int in_syscall=0;
        int curr=-1;
        struct timespec start;
ptrace(PTRACE_SYSCALL, pid, 0, 0);
        while (1) {
            if (waitpid(pid, &status, 0) == -1) {
                break;
            }
            if (WIFEXITED(status)) {
                if (in_syscall){
                    struct timespec end;
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                    int found=0;
                    for (int i=0;i<call_count;i++){if (calls[i].num==curr){calls[i].count++;calls[i].time+=elapsed;found=1;break;}}
                    if (!found){
                        calls[call_count].num=curr;
                        call_name(curr,calls[call_count].name,sizeof(calls[call_count].name));
                        calls[call_count].count=1;
                        calls[call_count].time=elapsed;
                        calls[call_count].first=call_count;
                        call_count++;
                }
                in_syscall=0;
                }
                break;
            }
            if (WIFSTOPPED(status)){int sig=WSTOPSIG(status);
              if (sig==(SIGTRAP|0x80)){
                struct user_regs_struct regs;
              if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == 0) {
                    if (in_syscall == 0) {
                        curr = regs.orig_rax;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        in_syscall = 1;
                    } else {
                        struct timespec end;
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                        int found=0;
                        for (int i=0;i<call_count;i++){if (calls[i].num==curr){calls[i].count++;calls[i].time+=elapsed;found=1;break;}}
                        if (!found){
                            calls[call_count].num=curr;
                            call_name(curr,calls[call_count].name,sizeof(calls[call_count].name));
                            calls[call_count].count=1;
                            calls[call_count].time=elapsed;
                            calls[call_count].first=call_count;
                            call_count++;
                        }
                        in_syscall = 0;
                    }

                }
                ptrace(PTRACE_SYSCALL, pid, 0, 0);      

            }
              else if (sig==SIGTRAP){
                
                ptrace(PTRACE_SYSCALL, pid, 0, 0);
              }
              else{
                ptrace(PTRACE_SYSCALL, pid, 0,(void*)(long) sig);        
              }
            }
        }
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        qsort(calls,call_count,sizeof(system_call),compare2);
        if (call_count>0){
printf("%-14s%-8s%s\n", "syscall", "calls", "time");
for (int i=0;i<call_count;i++){
    printf("%-14s%-8d%.3fs\n",calls[i].name,calls[i].count,calls[i].time);
}
        }
        fflush(stdout);
        }

