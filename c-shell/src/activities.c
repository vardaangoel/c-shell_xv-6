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