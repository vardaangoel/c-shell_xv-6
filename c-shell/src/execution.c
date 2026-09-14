#include"execution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include"activities.h"


// typedef struct{
//     pid_t pid;
//     int id;
//     char name[256];
//     int done,running,exit;
// }process;
// process prs[1000];
// int prs_count=0;
int prs_id=1;



static int isfile(char*path){
     struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISREG(st.st_mode) && access(path, X_OK) == 0) {
            return 1;
        }
    }    
    return 0;
}
static char* path(char*cmd){
int t=0;char*search=cmd;
if (cmd[0]=='%'){
    t=1;search=cmd+1;
}
if (!t&&strchr(cmd,'/')!=NULL){
    if (isfile(search)){return strdup(search);}
    return NULL;
}
char cwd[PATH_MAX];
char temp[PATH_MAX];
if (!t){
    if (getcwd(cwd,PATH_MAX)!=NULL){
        strcpy(temp,cwd);strcat(temp,"/");strcat(temp,search);
        if (isfile(temp)){return strdup(temp);}
    }
}
 char*path=getenv("PATH");
        if (path!=NULL){
          char* copy=(char*)malloc(strlen(path)+1);
          strcpy(copy,path);
          char* dir=strtok(copy,":");
          while(dir!=NULL){
            int n=strlen(dir);
            if(n>0&&dir[n-1]=='/'){
                strcpy(temp,dir);strcat(temp,search);
            }
            else {
                                strcpy(temp,dir);strcat(temp,"/");strcat(temp,search);
            }
            if (isfile(temp)){
                free(copy);return strdup(temp);
            }
            dir=strtok(NULL,":");
          }  
          free(copy);
}
return NULL;}

int execute(char**input,int count,int bg){
    if (input[0]==NULL||count==0)return 1;
    char cmd[1000]="";
    for (int i=0;i<count;i++){
        strcat(cmd,input[i]);
        if (i<count-1)strcat(cmd," ");
    }
    int pipeindex[1000];
    int n=1;
    pipeindex[0]=-1;
    for (int i=0;i<count;i++){
        if (strcmp(input[i],"|")==0){pipeindex[n++]=i;input[i]=NULL;}
    }
    pipeindex[n]=count;
    int in_fd = STDIN_FILENO; 
    pid_t pids[256];
    pid_t feeder_pids[256];
    pid_t writer_pids[256];
    char cmd_names[256][256];
    for (int i=0;i<256;i++){
        pids[i]=-1;
        feeder_pids[i]=-1;
        writer_pids[i]=-1;
    }
    int sync_pipe[2]={-1,-1};
    if(bg&&pipe(sync_pipe)<0){perror("pipe error");}
    
    
    for (int c = 0; c < n; c++) {
        int start = pipeindex[c] + 1;
        int end = pipeindex[c + 1];
        int cmd_count = end - start;
        char** cmd_input = &input[start];
        if (cmd_count == 0) {
            fprintf(stderr, "cshell: syntax error near unexpected token `|`\n");
            if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
            return 0;
        }
        int fd[2] = {-1, -1};
        if (c < n - 1) {
            if (pipe(fd) < 0) {
                perror("pipe error");
            if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
                return 0;
            }
        }
    char*arg[1000];char*files[1000];int file_count=0;
    char*out_files[1000];int out_count=0;
    int out[1000];
    int num=0;
    for (int i=0;i<cmd_count;i++){
        if (strcmp(cmd_input[i],"<")==0){
            if (i+1>=cmd_count){fprintf(stderr,"cshell: syntax error\n");
                            if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
                return 0;}
            else {files[file_count++]=cmd_input[i+1];i++;}
        }
        else if(strcmp(cmd_input[i],">")==0){
                        if (i+1>=cmd_count){fprintf(stderr,"cshell: syntax error\n");            if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
return 0;}
           else {out[out_count]=0;out_files[out_count]=cmd_input[i+1];out_count++;i++;}

        }
        else if (strcmp(cmd_input[i],">>")==0){
                        if (i+1>=cmd_count){fprintf(stderr,"cshell: syntax error\n");            if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
return 0;}
                  else {out[out_count]=1;out_files[out_count]=cmd_input[i+1];out_count++;i++;}

                    }
                    else {
                        arg[num++]=cmd_input[i];
                    }
    }
    if (num==0)continue;
    arg[num]=NULL;
    if (arg[0]!=NULL){if (arg[0][0]!='%')
        strcpy(cmd_names[c],arg[0]);
     else strcpy(cmd_names[c],arg[0]+1);
    }
    int stored[1000];
    for (int i=0;i<file_count;i++){
        stored[i]=open(files[i],O_RDONLY);
        if (stored[i]<0){
            fprintf(stderr,"cshell: no such file or directory\n");
            for (int j=0;j<i;j++)close(stored[j]);
                        if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
            return 0;}
    }
int out_stored[1000];
for (int i=0;i<out_count;i++){
    int flags = O_WRONLY | O_CREAT | (out[i] ? O_APPEND : O_TRUNC);
        out_stored[i] = open(out_files[i], flags, 0644);
        if (out_stored[i] < 0) {
            fprintf(stderr, "cshell: unable to create file for writing\n");
            for (int j = 0; j < file_count; j++) close(stored[j]);
            for (int j = 0; j < i; j++) close(out_stored[j]);
                        if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
            return 0;
        }
}
    char* cmd=arg[0];
    char* p=path(cmd);
    if (p==NULL){
        if (cmd[0]=='%'){
            fprintf(stderr,"cshell: command not found (%s)\n",cmd+1);

        }
        else {fprintf(stderr,"cshell: command not found (%s)\n",cmd);}
        for (int i=0;i<file_count;i++)close(stored[i]);
        for (int i=0;i<out_count;i++)close(out_stored[i]);
        if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
        return 0;
    }
    if (arg[0][0]=='%'){arg[0]+=1;}
int pipefd[2] = {-1, -1};
    pid_t feeder_pid = -1;
    if (file_count > 1) {
        if (pipe(pipefd) < 0) {
            perror("pipe error");
        }
        feeder_pid = fork();
        if (feeder_pid == 0) {
            close(pipefd[0]); 
            char buf[4096];
            ssize_t bytes_read;
            for (int i = 0; i < file_count; i++) {
                while ((bytes_read = read(stored[i], buf, sizeof(buf))) > 0) {
                    write(pipefd[1], buf, bytes_read);
                }
                close(stored[i]);
            }
            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        }
    }

int out_pipefd[2]={-1,-1};
pid_t writer_pid=-1;
if (out_count>1){if (pipe(out_pipefd) < 0) {
            perror("pipe error");
        }
        writer_pid=fork();
        if (writer_pid==0){
        close(out_pipefd[1]);
        char buf[4096];
     ssize_t bytes_read;
                     while ((bytes_read = read(out_pipefd[0], buf, sizeof(buf))) > 0) {
            for (int i = 0; i < out_count; i++) {
                    write(out_stored[i], buf, bytes_read);
                }}
                close(out_pipefd[0]);
            
           for (int i=0;i<out_count;i++)close(out_stored[i]);
            exit(EXIT_SUCCESS);}
        }
    


pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");free(p);
                    if (bg&&sync_pipe[0]!=-1){close(sync_pipe[0]);close(sync_pipe[1]);}
        return 0;
    } 
    else if (pid == 0) {pid_t pgid;
        if (c==0)pgid=0;
        else pgid=pids[0];
        setpgid(0,pgid);
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);        
        if (bg){close(sync_pipe[1]);
char temp;
read(sync_pipe[0],&temp,1);close(sync_pipe[0]);
        if (file_count==0&&in_fd==STDIN_FILENO){
            int dev=open("/dev/null",O_RDONLY);
            if (dev>=0){dup2(dev,STDIN_FILENO);close(dev);}
        }
}
        if (file_count == 1) {
            dup2(stored[0], STDIN_FILENO);
        } else if (file_count > 1) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
        }
        else if (in_fd!=STDIN_FILENO){dup2(in_fd,STDIN_FILENO);}
        if (out_count==1){dup2(out_stored[0],STDOUT_FILENO);
        } else if (out_count > 1) {
            dup2(out_pipefd[1], STDOUT_FILENO);
            close(out_pipefd[0]);
            close(out_pipefd[1]);}
        else if (c<n-1){
            dup2(fd[1],STDOUT_FILENO);
          
        }
        if (in_fd!=STDIN_FILENO)close (in_fd);
        if (c<n-1){close(fd[0]);close(fd[1]);}
        for (int i = 0; i < file_count; i++) close(stored[i]);
        for (int i = 0; i < out_count; i++) close(out_stored[i]);
        execv(p, arg);
        perror("execv error");
        exit(EXIT_FAILURE);
    } 
    else {pid_t pgid;pids[c]=pid;
        if (c==0){pgid=pid;}
        else pgid=pids[0];
        setpgid(pid,pgid);

        if (file_count > 1) {
            close(pipefd[0]);
            close(pipefd[1]);
        }
        if (out_count>1){
           close(out_pipefd[0]);
            close(out_pipefd[1]);
        }
        for (int i = 0; i < file_count; i++) close(stored[i]);
                for (int i = 0; i < out_count; i++) close(out_stored[i]);
if (in_fd!=STDIN_FILENO)close(in_fd);
if (c<n-1){close(fd[1]);in_fd=fd[0];}
pids[c]=pid;
feeder_pids[c]=feeder_pid;
writer_pids[c]=writer_pid;
    }
    free(p);}
    int job_id=prs_id++;
    add_job(job_id,pids[0],n,pids,cmd_names,bg,cmd);
    if (bg){close(sync_pipe[0]);

printf("[%d] %d\n",job_id,(int)pids[0]);
fflush(stdout);
close(sync_pipe[1]);
    }
    else{tcsetpgrp(STDIN_FILENO, pids[0]);int stopped=0;
    for (int c=0;c<n;c++){    int status;
        if (pids[c]>0){
        waitpid(pids[c], &status, WUNTRACED);
        update_state(pids[c],status);if (WIFSTOPPED(status)){stopped=1;}
        }
        if (feeder_pids[c] > 0) {
            waitpid(feeder_pids[c], NULL, 0);}
            if (writer_pids[c] > 0) {
    waitpid(writer_pids[c], NULL, 0);
            }
        
    
        }
        tcsetpgrp(STDIN_FILENO, getpid());
        if (stopped){printf("[%d] + Stopped    %s\n",job_id,cmd);}
        print_prs();}
        return 1;
}