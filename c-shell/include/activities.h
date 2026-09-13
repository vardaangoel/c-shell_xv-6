#ifndef ACTIVITIES_H
#define ACTIVITIES_H
#include <sys/types.h>
void sigchld(int sig);
void print_prs();
void print_activities();
void add_job(int job_id,pid_t pgid,int num,pid_t* pids, char cmd_names[][256],int bg);
void update_state(pid_t pid,int status);
#endif