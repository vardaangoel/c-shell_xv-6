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

int main(){

    home();
struct sigaction sa;
sa.sa_handler=sigchld;
sigemptyset(&sa.sa_mask);
sa.sa_flags=SA_RESTART;
sigaction(SIGCHLD,&sa,NULL);
    char* line=NULL;
    size_t l=0;
    ssize_t r;
    while(1){
        print_prs();
        prompt();
        r=getline(&line,&l,stdin);
        if (r==-1){
            printf("\n");break;

        }
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