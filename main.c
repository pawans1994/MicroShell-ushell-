#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include "parse.h"

#define MAX_SIZE 1024
typedef enum { false, true } bool;
int pipe_flag = 0;
int last_write = 0;
int turn_flag = 1;
int pfd[2];
int in_desc = 0;
char *home_dir;
char *prev_dir;
char *temp;
char *project_dir;


static int get_file_desc(char *flag, char *out_file)
{
    int out_desc;
    if (strcmp(flag, "w") == 0)
    {
        out_desc = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    else if(strcmp(flag, "a")==0)
    {
        out_desc = open(out_file, O_RDWR | O_APPEND | O_CREAT, 0666);
    }

    return out_desc;
}
static void run_pipe(char cmd[MAX_SIZE], char **cmdArgs, int in_desc, int out, bool red_err)
{
    int sloc;
    pid_t child_p;
    child_p = fork();
    if(child_p == 0)
    {
        if(in_desc != 0)
        {
            dup2(in_desc, 0);
            close(in_desc);

        }
        if(out != 1)
        {
            if(red_err == 1)
                dup2(out, 2);
            dup2(out, 1);
            close(out);
        }

        if (execvp(cmd, cmdArgs) != 0)
        {
            perror("Error");
            exit(1);
        }

    }
    else{
        waitpid(child_p, &sloc, WUNTRACED);
    }
}
static void exCmd(char cmd[MAX_SIZE], char  **cmdArgs, char *in_file, char *out_file, int fd, char *flag, int n, int o, bool red_err){
    pid_t c_pid;
    int stat_loc;
    c_pid = fork();
    int file_desc;
    if (c_pid == 0)
    {
        if (n == 1)
        {
            file_desc = open(in_file, O_RDONLY, 644);
            if(file_desc < 0){
                perror("Couldn't open file");
                exit(0);
            }
            dup2(file_desc, STDIN_FILENO);
            close(file_desc);
        }
        if(o == 1)
        {
            if (fd != 0)
            {
                if (strcmp(flag, "a") == 0) {
                    file_desc = open(out_file, O_RDWR | O_APPEND | O_CREAT, 0666);
                }
                if (strcmp(flag, "w") == 0) {
                    file_desc = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }
                if(file_desc < 0){
                    perror("Couldn't open file");
                    exit(0);
                }
                dup2(file_desc, 1);
                if(red_err == 1)
                    dup2(file_desc, 2);
                close(file_desc);
            }
        }

        if(execvp(cmd, cmdArgs) != 0)
        {
            perror("execvp");
            exit(1);
        }

    }
    else
    {
        waitpid(c_pid, &stat_loc, WUNTRACED);
    }


}

static void ch_dir(char *path){

    char new_path[MAX_SIZE];
    char *path_slice;
    path_slice = malloc(sizeof(path) * sizeof(char*));
    strcpy(path_slice, path);
    path_slice++;

    if(path[0] == '\0')
    {
        if(chdir(project_dir) !=0 )
            perror("Error");
    }
    else if(path[0] == '$' && getenv(path_slice)!=NULL)
    {
        if(chdir(getenv(path_slice)) != 0)
            perror("Error");
    }
    else if(!strcmp(path, "-"))
    {
        if (prev_dir == NULL)
            printf("NO PREVIOUS DIRECTORY\n");
        else {
            if (chdir(prev_dir) != 0)
                perror("Error");
        }
    }
    else if(path[0] != '/' && path[0]!='~'){
        getcwd(new_path, sizeof(new_path));
        strcat(new_path, "/");
        strcat(new_path, path);
        if(chdir(new_path) != 0)
            perror("Error");
    }
    else if(path[0] == '~'){
        char *temp = path;
        temp++;
        strcpy(new_path, home_dir);
        strcat(new_path, temp);
        if(chdir(new_path) != 0)
            perror("Error");
    }
    else{
        if(chdir(path) != 0)
            perror("Error");
    }

}

static void prCmd(Cmd c)
{
    int i;
    char cmd[MAX_SIZE];
    char **cmdArgs;
    int fd = 0;
    char *flag;
    int in = 0;
    bool red_err = 0;
    bool isPipe = 0;
    int out = 0;
//    prev_dir = malloc(1024 * sizeof(char*));
//    temp = prev_dir = malloc(1024 * sizeof(char*));
    if ( c ) {
//        printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
        if ( !strcmp(c->args[0], "end") || !strcmp(c->args[0], "logout") )
            exit(0);
        if ( c->in == Tin ) {
            in = 1;
//            printf("<(%s) ", c->infile);
        }
        if ( c->out != Tnil )
        {   //in = 0;
            out= 1;
            if(in != 1)
                in = 0;
            switch ( c->out ) {
                case Tout:
//                    printf(">(%s) ", c->outfile);
                    fd = 1;
                    flag = malloc(10*sizeof (char*));
                    strcpy(flag, "w");
                    break;
                case Tapp:
                    fd = 1;
                    flag = malloc(10*sizeof (char*));
                    strcpy(flag, "a");
                    //printf(">>(%s) ", c->outfile);
                    break;
                case ToutErr:
                    //printf(">&(%s) ", c->outfile);
                    fd = 1;
                    red_err = 1;
                    flag = malloc(10*sizeof (char*));
                    strcpy(flag, "w");
                    break;
                case TappErr:
                    //printf(">>&(%s) ", c->outfile);
                    fd = 1;
                    red_err = 1;
                    strcpy(flag, "a");
                    break;
                case Tpipe:
                    //printf("| ");
                    pipe_flag = 1;
                    isPipe= 1;
                    break;
                case TpipeErr:
                    //printf("|& ");
                    isPipe = 1;
                    pipe_flag = 1;
                    red_err = 1;
                    break;
                default:
                    fprintf(stderr, "Shouldn't get here\n");
                    exit(-1);
            }}
        //Writing the commands from data structure and it's arguments to variable cmd and cmdArgs
        cmdArgs = malloc(MAX_SIZE * sizeof(char*));
        strcpy(cmd, c->args[0]);
        cmdArgs[0] = malloc(MAX_SIZE* sizeof(char));
        strcpy(cmdArgs[0], c->args[0]);
        if ( c->nargs > 1 ) {

//      printf("[");
            for ( i = 1; c->args[i] != NULL; i++ ){
                cmdArgs[i] = malloc(MAX_SIZE* sizeof(char));
                strcpy(cmdArgs[i], c->args[i]);
//                printf("%d:%s,", i, c->args[i]);
//                printf("%s", getenv(cmdArgs[i]));
            }
//      printf("\b]");
        }
        //Executing the command
        char **echo_args;
        echo_args = malloc(MAX_SIZE* sizeof(char));

        if(pipe_flag == 0)
        {

            if(strcmp(c->args[0], "where") == 0)
            {
                strcpy(cmd, "which");
                if (in == 0)
                    exCmd(cmd, cmdArgs, c->infile,c->outfile, fd, flag, in, out, red_err);
                else
                    exCmd(cmd, cmdArgs, c->infile, c->outfile,fd, flag, in, out, red_err);
            }
            else if (strcmp(c->args[0], "cd") == 0) {

                if(cmdArgs[1] == NULL)
                    ch_dir("");
                else
                    ch_dir(cmdArgs[1]);
                prev_dir = (char*)malloc(MAX_SIZE);
                strcpy(prev_dir, temp);
            }
            else if(!strcmp(c->args[0], "setenv"))
            {
//                char *temp = cmdArgs[1];
//                temp++;
                setenv(cmdArgs[1], cmdArgs[2], 1);
            }
            else if(!strcmp(c->args[0], "unsetenv"))
            {
//                char *temp = cmdArgs[1];
//                temp++;
                unsetenv(cmdArgs[1]);
            }
            else if(strcmp(c->args[0], "echo") == 0)
            {
                int i;
                for(i = 1;cmdArgs[i]!=NULL;i++)
                {

                    if (cmdArgs[i][0] == '$')
                    {
                        char *temp = cmdArgs[i];
                        temp ++;
                        printf("%s ", getenv(temp));
                    }
                    else
                    {
                        printf("%s ", cmdArgs[i]);
                    }
                }
                putchar('\n');
            }

            else{
//        {   printf("%s", cmd);
                if (in == 0)
                    exCmd(cmd, cmdArgs, c->infile,c->outfile, fd, flag, in, out, red_err);
                else
                    exCmd(cmd, cmdArgs, c->infile, c->outfile,fd, flag, in, out, red_err);
            }
        }
        else {

            if(!strcmp(c->args[0], "echo"))
            {
                echo_args[0] = malloc(MAX_SIZE* sizeof(char));
                strcpy(echo_args[0], cmdArgs[0]);
                int i;
                for(i = 1;cmdArgs[i]!=NULL;i++)
                {
                    echo_args[i] = malloc(MAX_SIZE* sizeof(char));
                    if (cmdArgs[i][0] == '$')
                    {
                        char *temp = cmdArgs[i];
                        temp++;
                        strcpy(echo_args[i], getenv(temp));
                    }
                    else
                    {
                        strcpy(echo_args[i], cmdArgs[i]);
                    }
                    int j;
                    for(j = 1;echo_args[j]!=NULL;j++)
                    {
                        strcpy(cmdArgs[j], echo_args[j]);
                    }
                }

            }
            if(strcmp(c->args[0], "where") == 0)
            {
                strcpy(cmd, "which");
            }
            if(isPipe == 1) {
                pipe(pfd);
                run_pipe(cmd, cmdArgs, in_desc, pfd[1], red_err);
                close(pfd[1]);
                in_desc = pfd[0];
            }
            else
            {
                if(c->outfile !=NULL)
                    run_pipe(cmd, cmdArgs, in_desc, get_file_desc(flag, c->outfile), red_err);
                else
                    run_pipe(cmd, cmdArgs, in_desc, 1, red_err);
                pipe_flag = 0;
            }

        }
        //putchar('\n');

        // this driver understands one command

    }
}


static void prPipe(Pipe p)
{
    int i = 0;
    Cmd c;

    if ( p == NULL )
        return;

    //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
    for ( c = p->head; c != NULL; c = c->next ) {
        //printf("Cmd #%d: ", ++i);
        prCmd(c);
    }
    //printf("\nEnd pipe\n");
    prPipe(p->next);
}

static void getInputFromFile()
{
    Pipe p;
    char *path;
    path = (char*)malloc(MAX_SIZE);
    strcat(path, getenv("HOME"));
    strcat(path, "/.ushrc");
    int fp;
    fp = open(path, O_RDONLY);
    if (fp == -1) {
        printf("Error Reading the File");
        exit(1);
    }
    dup2(fp, 0);

    while(1) {
        p = parse();
        if (p == NULL || !strcmp(p->head->args[0], "end")) {
            close(fp);
            exit(0);
        }
        prPipe(p);
        freePipe(p);
    }
}

int main(int argc, char *argv[])
{

    Pipe p;
    char *host = "armadello";

    if ((home_dir = getenv("HOME")) == NULL) {
        home_dir = getpwuid(getuid())->pw_dir;}
    project_dir = (char*)malloc(MAX_SIZE);
    getcwd(project_dir, MAX_SIZE);
    pid_t cmd;
    cmd = fork();
    if(cmd ==0){
        getInputFromFile();
    }
    else{
        dup2(dup(0),0);
        while ( 1 ) {
            printf("%s%% ", host);
            temp = (char*)malloc(MAX_SIZE);
            getcwd(temp,MAX_SIZE);
            p = parse();
            prPipe(p);
            freePipe(p);

        }
    }

}

/*........................ end of main.c ....................................*/