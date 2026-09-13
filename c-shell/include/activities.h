#ifndef ACTIVITIES_H
#define ACTIVITIES_H
#include <sys/types.h>
void sigchld(int sig);
void print_prs();
void print_activities();
void add_job(int job_id,pid_t pgid,int num,pid_t* pids, char cmd_names[][256],int bg,char*cmd);
void update_state(pid_t pid,int status);
void send_sighup();
int stopped();
void resume(char**input,int count);
void ping(char**input,int count);
void spy(char**input,int count);
void snoop(char**input,int count);
#endif