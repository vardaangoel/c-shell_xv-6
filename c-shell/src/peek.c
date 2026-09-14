#include"peek.h"
#include"shell_prompt.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<linux/limits.h>
typedef struct{
    char name[PATH_MAX];
    char**lines;
    int total,nonempty;
    int line_num;
}File;
static void path(char*inp,char*out){
   if (strcmp(inp,"-")==0) {
        strcpy(out,"-");
    } else if (strcmp(inp,"~")==0) {
        strcpy(out,home_shell);
    } else if (strncmp(inp,"~/",2)==0) {
        strcpy(out,home_shell);strcat(out,"/");strcat(out,inp+2);
    } else {
        strcpy(out,inp);
    } 
}
static void buff(File *f) {
    int cap = 128;
    f->lines = malloc(cap*sizeof(char*));
    f->total = 0;
    f->nonempty = 0;
    char *line = NULL; 
    size_t len = 0; 
    ssize_t r;
    while ((r=getline(&line,&len,stdin))!=-1) {
        if (f->total >= cap) {
            cap*=2;
            f->lines=realloc(f->lines, cap * sizeof(char*));
        }
        if (r>0&&line[r-1]=='\n') {
            line[r- 1] = '\0';
        }
        if (r>1&&line[r-2]=='\r') {
            line[r- 2] = '\0';
        }
        f->lines[f->total++] = strdup(line);
        int e=1;
        for (size_t i=0;i<strlen(line);i++){
            if (line[i]!='\r'&&line[i]!='\n'&&line[i]!='\0'){e=0;break;}
        }
        if (e==0) {
            f->nonempty++;
        }
    }
    free(line);
}

static void cnt(File*f){
    FILE*fi=fopen(f->name,"r");
if (!fi)return;
f->nonempty=0;
char*line=NULL;size_t len=0;ssize_t r;
while((r=getline(&line,&len,fi))!=-1){int e=1;for (ssize_t i=0;i<r;i++){
    if (line[i]!='\n'&&line[i]!='\r'&&line[i]!='\0'){e=0;break;}
}
if (e==0){f->nonempty++;}

}
free(line);fclose(fi);

}

static void print(File*f,int n,int*line_n){
    FILE*fi;
    if (strcmp(f->name,"-")==0){fi=stdin;}
    else {fi=fopen(f->name,"r");}
    if (!fi){perror("peek error");return;}
    char*line=NULL; size_t len = 0;
    ssize_t r;
    while ((r=getline(&line,&len,fi))!=-1) {
        if (n) {int e=1;for (int i=0;i<r;i++){
    if (line[i]!='\n'&&line[i]!='\r'&&line[i]!='\0'){e=0;break;}
}
if (e==0){printf("%d ",(*line_n)++);}
        }
        fwrite(line, 1,r,stdout);
    }
    free(line);
    if (fi!=stdin)
    fclose(fi);

}

static void print_rev(File*f,int n){
    if (strcmp(f->name,"-")==0){
int curr=f->line_num;
for (int j=f->total-1;j>=0;j--){
    if (n){int e=1;int len=strlen(f->lines[j]);for (int i=0;i<len;i++){
    if (f->lines[j][i]!='\n'&&f->lines[j][i]!='\r'&&f->lines[j][i]!='\0'){e=0;break;}
}
if (e==0){printf("%d ",curr);curr--;}
    }
    printf("%s\n",f->lines[j]);
}
    }
    else {
    int fd=open(f->name,O_RDONLY);
    if (fd==-1) { 
        perror("peek error"); 
        return; 
    }int pos = lseek(fd,0,SEEK_END);
    char chunk[4096];
    char *tail=NULL; 
    int tail_len=0;
    int first=1;
    int curr=f->line_num;
    while (pos>0) {
        int r=(pos>(int)sizeof(chunk))?(int)sizeof(chunk):pos;
        pos-=r;
        lseek(fd,pos,SEEK_SET);
        read(fd,chunk,r);
        int end=r-1;
        if (first&&chunk[end]=='\n') {
            end--; 
        }
first =0;
        for (int j=end;j>=0;j--) {
            if (chunk[j] == '\n') {
                int p_len = end - j;
                const char *part = chunk + j + 1;
                int e=1;
                for (int i=0;i<p_len;i++){
    if (part[i]!='\n'&&part[i]!='\r'&&part[i]!='\0'){e=0;break;}
}
for (int i=0;i<tail_len;i++){
    if (tail[i]!='\n'&&tail[i]!='\r'&&tail[i]!='\0'){e=0;break;}
}
if (e==0&&n){
                    printf("%d ",curr);curr--;
                }              
                if (p_len > 0) fwrite(part, 1, p_len, stdout);
                if (tail_len > 0) fwrite(tail,1,tail_len,stdout);
                printf("\n");
                free(tail); 
                tail=NULL; 
                tail_len=0;
                end=j-1;
            }
        }
        if (end >= 0) {
            int p_len = end + 1;
            char *new_tail = malloc(p_len + tail_len);
            memcpy(new_tail, chunk, p_len);
            if (tail_len > 0) {
                memcpy(new_tail + p_len, tail, tail_len);
            }
            free(tail);
            tail = new_tail;
            tail_len += p_len;
        }
    }
    if (tail_len > 0) {
        if (n) {int e=1;
            for (int i=0;i<tail_len;i++){if (tail[i]!='\r'&&tail[i]!='\n'&&tail[i]!='\0'){e=0;break;}}
          if (e==0)  {printf("%d ",curr);curr--;}
        }
        fwrite(tail, 1, tail_len, stdout);
        printf("\n");
        free(tail);
    }    
    close(fd);
}
    }
void peek(char**input,int count){
    int n=0,r=0;
    char*all[1000];
    int num=0;
    for (int i=0;i<count;i++){
        if (input[i][0]=='-'&&strlen(input[i])>1){
            for(size_t j=1;j<strlen(input[i]);j++){
                if (input[i][j]=='n')n=1;
                else if (input[i][j]=='r')r=1;
                else {
                    fprintf(stderr,"peek: invalid syntax\n");
                    return;
                }
            }
        }
       else{ all[num]=input[i];num++;}
    }
    if (num==0){all[0]="-";num=1;}
    File*inputs=calloc(num,sizeof(File));
    int line_counter=0;
    for (int i=0;i<num;i++){
        path(all[i],inputs[i].name);
        if (strcmp(inputs[i].name,"-")==0){if (r)buff(&inputs[i]);}
        else if (n&&r){cnt(&inputs[i]);}
if (r&&n){inputs[i].line_num=line_counter+inputs[i].nonempty;line_counter+=inputs[i].nonempty;}

    }  int fw_count=1;
    for (int i=0;i<num;i++){
        struct stat st;
        if (strcmp(inputs[i].name,"-")!=0){
        if (stat(inputs[i].name,&st)==-1){fprintf(stderr,"peek: no such file or directory\n");
                continue;}
            if (S_ISDIR(st.st_mode)){
                fprintf(stderr, "peek: is a directory\n");continue;
            }}
          
            if (!r){print(&inputs[i],n,&fw_count);}
            else {print_rev(&inputs[i],n);}
    }
    for (int i=0;i<num;i++) {
        if (inputs[i].lines) {
            for(int j=0; j<inputs[i].total;j++){
                free(inputs[i].lines[j]);
            }
            free(inputs[i].lines);
        }
    }
    free(inputs);
}
