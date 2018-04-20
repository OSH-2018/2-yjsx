#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

typedef struct cmd{
    char *name;
    char *para;
    char *total[2];
} str_cmd;

void print_prompt(){
    char local[4096];
    printf("[dsh]");
    puts(getcwd(local,4096));
    printf("# ");
}
void show_cmds(str_cmd *cmds,int pipe_num){
    int i = 0;
    for (i = 0;i<pipe_num + 1;i++){
        printf("%d,name:%s,para:%s\n",i,cmds[i].name,cmds[i].para);
    }
}
int inner_cmd(char *name,char *para){
    int i;
    // printf("name:%s,para:%s\n",name,para);
    if (!name)
        return 1;

    /* 内建命令 */
    if (strcmp(name, "cd") == 0) {
        if (para)
            chdir(para);
        return 1;
    }
    if (strcmp(name, "pwd") == 0) {
        char wd[4096];
        puts(getcwd(wd, 4096));
        return 1;
    }
    if (strcmp(name, "export") == 0) {
        char *path[2];
        path[0] = para;
        for (i = 0; *path[i]; i++)
            for (path[i+1] = path[i] + 1; *path[i+1]; path[i+1]++)
                if (*path[i+1] == '=') {
                    *path[i+1] = '\0';
                    path[i+1]++;
                    break;
            }
        setenv(path[0],path[1],1);
        return 1;
    }
    if (strcmp(name, "exit") == 0)
        return 2;
}

int pipe_cmd(str_cmd *cmds,int pipe_num){
    // show_cmds(cmds,pipe_num);
    pid_t top = fork();
    if(top == 0){   
        int fd[2];
        pipe(fd);
        pid_t pid = fork();
        if(pid == 0){
            close(fd[0]);
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);
            if(pipe_num > 0){
                pipe_cmd(cmds,pipe_num - 1);
            }
            // execvp(cmds[pipe_num - 1].name, cmds[pipe_num - 1].total);
            _exit(0);
            return 255;
        }
        else{
            close(fd[1]);
            dup2(fd[0],STDIN_FILENO);
            close(fd[0]);
            waitpid(-1,NULL,0);
            pid_t f = fork();
            if (f == 0) {
                /* 子进程 */
                execvp(cmds[pipe_num].name, cmds[pipe_num].total);
                /* execvp失败 */
                return 255;
            }
            // execvp(cmds[1].name, cmds[1].total);
            wait(NULL);
            _exit(0);
        }
    }
    else
        wait(NULL);
}

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];
    while (1) {
        /* 提示符 */
        print_prompt();
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 清理结尾的换行符 */
        int cmd_num = 0;
        int pipe_num = 0;
        str_cmd cmds[4] = {NULL,NULL,{NULL,NULL}};
        int i;
        int l = strlen(cmd);
        cmd[l-1] = '\0';
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
                if (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    args[i+1]++;
                    break;
                }
        args[i] = NULL;
        int all_num = i;
        for(i=0;i<all_num;i++){
            if( strcmp(args[i],"|") == 0){
                pipe_num ++;
                if(cmds[cmd_num].name == NULL)
                    continue;
                else{
                    cmd_num++;
                    continue;
                }
            }
            if(cmds[cmd_num].name == NULL){
                cmds[cmd_num].name = args[i];
                cmds[cmd_num].total[0] = cmds[cmd_num].name;
            }
            else {
                cmds[cmd_num].para = args[i];
                cmds[cmd_num].total[1] = cmds[cmd_num].para;
                cmd_num ++;
            }
        }
        
        if(pipe_num == 0){
            int f = inner_cmd(cmds[0].name,cmds[0].para);
            if(f == 1)
                continue;
            else if(f == 2)
                return 0;
            else{
                pipe_cmd(cmds,pipe_num);
                // pid_t pid = fork();
                // if (pid == 0) {
                //     /* 子进程 */
                //     execvp(args[0], args);
                //     /* execvp失败 */
                //     return 255;
                // }
                // /* 父进程 */
                // wait(NULL);
            }
        }
        else{
            pipe_cmd(cmds,pipe_num);
        }
    }
}