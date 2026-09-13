#define _DEFAULT_SOURCE
#include"shell_prompt.h"
#include<stdio.h>
#include<stdlib.h>
#include"lexer.h"
#include"grammar.h"
#include"hop.h"
#include <sys/types.h>
#include<string.h>
#include"reveal.h"
#include"peek.h"
#include"locate.h"
#include"execution.h"
#include<signal.h>
#include<errno.h>
#include"activities.h"
#include <unistd.h>
void sigint_handler(int sig){(void)sig;write(STDOUT_FILENO,"\n",1);}
int main(){

    home();
    struct sigaction sa_int;
    sa_int.sa_handler=sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags=0;
    sigaction(SIGINT,&sa_int,NULL);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
struct sigaction sa;
sa.sa_handler=sigchld;
sigemptyset(&sa.sa_mask);
sa.sa_flags=SA_RESTART;
sigaction(SIGCHLD,&sa,NULL);
pid_t shellpgid=getpid();
setpgid(shellpgid,shellpgid);
tcsetpgrp(STDIN_FILENO,shellpgid);
    char* line=NULL;
    size_t l=0;
    ssize_t r;int warning=0;
    while(1){
        print_prs();
        prompt();
        r=getline(&line,&l,stdin);
        if (r==-1){
            if (!feof(stdin)){
                clearerr(stdin); warning=0;continue;
            }
           
printf("\n");
if (stopped()&&!warning){
    printf("cshell: there are stopped jobs\n");warning=1;
    clearerr(stdin);
    continue;
}
send_sighup();break;

        }
        warning=0;
        if (r>0&&line[r-1]=='\n'){line[r-1]='\0';r--;}
        if (r==0)continue;
        Token*tokens=convert(line);
        if (tokens){if (check(tokens)==0){
            fprintf(stderr, "cshell: invalid syntax\n");}
            else{Token*curr=tokens;
                while(curr!=NULL){
                    char*input[256];
                    int count=0;
                    int bg=0;
                    while(curr!=NULL&&strcmp(curr->val,";")!=0&&strcmp(curr->val,"&")!=0&&count<256){
                        input[count]=curr->val;count++;
                        curr=curr->next;
                    }
                    if (curr!=NULL&&strcmp(curr->val,"&")==0){bg=1;
                        curr=curr->next;}
                    else if (curr!=NULL&&strcmp(curr->val,";")==0){
                        bg= 0;
                        curr=curr->next;
                    }
                    if (count!=0){
                        int flag=1;
                        if (strcmp(input[0],"hop")==0){
 hopper(input+1,count-1);
                        }
                        else if (strcmp(input[0],"reveal")==0){
 reveal(input+1, count-1);
                        }
                        else if (strcmp(input[0],"peek")==0){
peek(input+1,count-1);
                        }
                        else if (strcmp(input[0],"locate")==0){
                            locate(input+1,count-1);
                        }
                        else if(strcmp(input[0],"activities")==0){print_activities();}
                        else if (strcmp(input[0],"resume")==0){
                            resume(input,count);
                        }
                        else if (strcmp(input[0],"exit")==0){
                            send_sighup();exit(0);
                        }
                        else if (strcmp(input[0],"ping")==0){
                            ping(input,count);
                        }
                        else if (strcmp(input[0],"cd")==0){
                            if (count==1){chdir(getenv("HOME"));}
                            else if (count==2){if (chdir(input[1])!=0){
                                fprintf(stderr,"cshell: cd: %s: No such file or directory\n",input[1]);
                            }}
                            else {fprintf(stderr,"cshell: cd: too many arguments\n");}
                        }
                        else {
                            flag=execute(input,count,bg);
                        }
                        
                        if (flag==0&&!bg)break;
                    }}
print_prs();                }

                
            
            freee(tokens);}
    }
    free(line);

    return 0;
}