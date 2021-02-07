#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include "parse.h"

typedef enum { false, true } bool;
int pipe_flag = 0;
int last_write = 0;
int turn_flag = 1;
int pfd[2];
int in_desc = 0;
char *home_dir;

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
static void run_pipe(char cmd[1024], char **cmdArgs, int in_desc, int out, bool red_err)
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

        execvp(cmd, cmdArgs);
    }
    else{
        waitpid(child_p, &sloc, WUNTRACED);
    }
}
static void exCmd(char cmd[1024], char  **cmdArgs, char *out_file, int fd, char *flag, int n, bool red_err){
    pid_t c_pid;
    int stat_loc;
    c_pid = fork();
    int file_desc;
    if (c_pid == 0)
    {
        if (n == 1)
        {
            file_desc = open(out_file, O_RDONLY, 644);
            if(file_desc < 0){
                perror("Couldn't open file");
                exit(0);
            }
            dup2(file_desc, STDIN_FILENO);
            close(file_desc);
        }
        else
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

        execvp(cmd, cmdArgs);
//        perror("execvp");
//        exit(1);
    }
    else
    {
        waitpid(c_pid, &stat_loc, WUNTRACED);
    }


}

static void ch_dir(char *path){

    char new_path[1024];
    if(path == NULL)
    {
        if(chdir(home_dir) !=0 )
            perror("Error");
    }
    else if(path[0] != '/'){
        getcwd(new_path, sizeof(new_path));
        strcat(new_path, "/");
        strcat(new_path, path);
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
    char cmd[1024];
    char **cmdArgs;
    int fd = 0;
    char *flag;
    int in;
    bool red_err = 0;
    bool isPipe = 0;
    if ( c ) {
        //printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
        if ( !strcmp(c->args[0], "end") )
            exit(0);
        if ( c->in == Tin ) {
            in = 1;
            //printf("<(%s) ", c->infile);
        }
        if ( c->out != Tnil )
        {   in = 0;
            switch ( c->out ) {
                case Tout:
                    //printf(">(%s) ", c->outfile);
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
        cmdArgs = malloc(1024 * sizeof(char*));
        strcpy(cmd, c->args[0]);
        cmdArgs[0] = malloc(1024* sizeof(char));
        strcpy(cmdArgs[0], c->args[0]);
        if ( c->nargs > 1 ) {

//      printf("[");
            for ( i = 1; c->args[i] != NULL; i++ ){
                cmdArgs[i] = malloc(1024* sizeof(char));
                strcpy(cmdArgs[i], c->args[i]);
//            printf("%d:%s,", i, c->args[i]);
            }
//      printf("\b]");
        }
        //Executing the command
        if(pipe_flag == 0)
        {
            if(strcmp(c->args[0], "where") == 0)
            {
                strcpy(cmd, "whereis");
            }
            if (strcmp(c->args[0], "cd") == 0) {
                ch_dir(cmdArgs[1]);
            }
            else{
//        {   printf("%s", cmd);
                if (in == 0)
                    exCmd(cmd, cmdArgs, c->outfile, fd, flag, in, red_err);
                else
                    exCmd(cmd, cmdArgs, c->infile, fd, flag, in, red_err);
            }
        }
        else {
            if(strcmp(c->args[0], "where") == 0)
            {
                strcpy(cmd, "whereis");
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

int main(int argc, char *argv[])
{
    Pipe p;
    if ((home_dir = getenv("HOME")) == NULL) {
        home_dir = getpwuid(getuid())->pw_dir;}
    char *host = "armadillo";
    while ( 1 ) {
        printf("%s%% ", host);
        p = parse();
        prPipe(p);
        freePipe(p);
    }
}

/*........................ end of main.c ....................................*/