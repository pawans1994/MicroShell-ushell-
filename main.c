/******************************************************************************
 *
 *  File Name........: main.c
 *
 *  Description......: Simple driver program for ush's parser
 *
 *  Author...........: Vincent W. Freeh
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "parse.h"

static void exCmd(char cmd[1024], char  **cmdArgs, char *out_file, int fd, char *flag, int n){
    pid_t c_pid;
    int stat_loc;
    c_pid = fork();
    int file_desc;
    if (c_pid == 0)
    {
        if (n == 1)
        {
            file_desc = open(out_file, O_RDONLY, 644);
            dup2(file_desc, STDIN_FILENO);
            close(file_desc);
        }
        else
            {
                if (fd != 0)
                {
                    if (strcmp(flag, "a") == 0) {
                        file_desc = open(out_file, O_RDWR | O_APPEND, 664);
                    }
                    if (strcmp(flag, "w") == 0) {
                        file_desc = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    }
                    dup2(file_desc, 1);
                    close(file_desc);
                }
            }
        execvp(cmd, cmdArgs);
    }

    else
    {
        waitpid(c_pid, &stat_loc, WUNTRACED);
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

  if ( c ) {
    printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    if ( c->in == Tin ) {
        in = 1;
        printf("<(%s) ", c->infile);
    }
    if ( c->out != Tnil )
    {   in = 0;
      switch ( c->out ) {
      case Tout:
	printf(">(%s) ", c->outfile);
	fd = 1;
    flag = malloc(10*sizeof (char*));
	strcpy(flag, "w");
	break;
      case Tapp:
      fd = 1;
      flag = malloc(10*sizeof (char*));
      strcpy(flag, "a");
	printf(">>(%s) ", c->outfile);
	break;
      case ToutErr:
	printf(">&(%s) ", c->outfile);
	fd = 2;
    flag = malloc(10*sizeof (char*));
    strcpy(flag, "w");
	break;
      case TappErr:
	printf(">>&(%s) ", c->outfile);
	fd = 2;
	strcpy(flag, "a");
	break;
      case Tpipe:
	printf("| ");
	break;
      case TpipeErr:
	printf("|& ");
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

      printf("[");
      for ( i = 1; c->args[i] != NULL; i++ ){
            cmdArgs[i] = malloc(1024* sizeof(char));
            strcpy(cmdArgs[i], c->args[i]);
            printf("%d:%s,", i, c->args[i]);
      }
      printf("\b]");
    }
    //Executing the command
    if (in == 0)
        exCmd(cmd, cmdArgs, c->outfile, fd, flag, in);
    else
        exCmd(cmd, cmdArgs, c->infile, fd, flag, in);

    putchar('\n');

    // this driver understands one command
    if ( !strcmp(c->args[0], "end") )
      exit(0);
  }
}


static void prPipe(Pipe p)
{
  int i = 0;
  Cmd c;

  if ( p == NULL )
    return;

  printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
  for ( c = p->head; c != NULL; c = c->next ) {
    printf("Cmd #%d: ", ++i);
    prCmd(c);
  }
  printf("\nEnd pipe\n");
  prPipe(p->next);
}

int main(int argc, char *argv[])
{
  Pipe p;
  char *host = "armadillo";

  while ( 1 ) {
    printf("%s%% ", host);
    p = parse();
    prPipe(p);
    freePipe(p);
  }
}

/*........................ end of main.c ....................................*/
