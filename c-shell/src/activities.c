#define _DEFAULT_SOURCE
#include"activities.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<errno.h>

typedef struct{
    pid_t pid;char name[256];
    int state,exit,printed;
}process;
typedef struct{
    int job_id,bg,active,num;
    pid_t pgid;
process prs[200];
}job;

job jobs[2000];
int job_count=0;

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


void add_job(int job_id,pid_t pgid,int num,pid_t* pids, char cmd_names[][256],int bg){
    int i=job_count++;
    jobs[i].job_id=job_id;
    jobs[i].pgid=pgid;
    jobs[i].num=num;
    jobs[i].bg=bg;
    jobs[i].active=1;
    for (int j=0;j<num;j++){
        jobs[i].prs[j].pid=pids[j];
        strcpy(jobs[i].prs[j].name,cmd_names[j]);
        jobs[i].prs[j].state=0;
        jobs[i].prs[j].exit=1;
        jobs[i].prs[j].printed=0;
    }
}