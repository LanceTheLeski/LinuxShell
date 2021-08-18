#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h> 

#define MAX_LINE 80 
#define MAX_ARGS (MAX_LINE/2 + 1)
#define REDIRECT_OUT_OP '>'
#define REDIRECT_IN_OP '<'
#define PIPE_OP '|'
#define BG_OP '&'

#define EXECUTED 0
#define RUNNING 1
#define STOPPED 2

/* Holds a single command. */
typedef struct Cmd 
{
  /* The command as input by the user. */
  char line[MAX_LINE + 1];

  /* The command as null terminated tokens. */
  char tokenLine[MAX_LINE + 1];

  /* Pointers to each argument in tokenLine, non-arguments are NULL. */
  char* args[MAX_ARGS];

  /* Pointers to each symbol in tokenLine, non-symbols are NULL. */
  char* symbols[MAX_ARGS];

  /* The process id of the executing command. */
  pid_t pid;

  /* TODO: Additional fields may be helpful. */
  int status;

  int num;
} Cmd;

Cmd *cmdList [0];
int cmdLength = 0;
int bgNum = 1;

/* The processof the currently executing foreground command, or 0. */
pid_t foregroundPid = 0;

/* Parses the command string contained in cmd->line.
* Assumes all fields in cmd (except cmd->line) are initailized to zero.
* On return, all fields of cmd are appropriatly populated. */
void parseCmd(Cmd* cmd) 
{
  char* token;
  int i = 0;
  strcpy(cmd->tokenLine, cmd->line);
  strtok(cmd->tokenLine, "\n");
  token = strtok(cmd->tokenLine, " ");
  while (token != NULL) 
  {
    if (*token == '\n') 
    {
      cmd->args[i] = NULL;
    } 
    else if (*token == REDIRECT_OUT_OP || *token == REDIRECT_IN_OP || *token == PIPE_OP || *token == BG_OP) 
    {
      cmd->symbols[i] = token;
      cmd->args[i]= NULL;
    } 
    else 
    {
      cmd->args[i] = token;
    }
    token = strtok(NULL, " ");
    i++;
  }
  cmd->args[i] = NULL;
}
  
/* Finds the index of the first occurance of symbol in cmd->symbols.
* Returns -1 if not found. */
int findSymbol(Cmd* cmd, char symbol) 
{
  for (int i = 0; i < MAX_ARGS; i++) 
  {
    if (cmd->symbols[i] && *cmd->symbols[i] == symbol) 
    {
      return i;
    }
  }
  return -1;
}

/* Signal handler for SIGTSTP (SIGnal -Terminal SToP),
* which is caused by the user pressing control+z. */  
void sigtstpHandler(int sig_num) 
{ 
  /* Reset handler to catch next SIGTSTP. */
  signal(SIGTSTP, sigtstpHandler); 
  if (foregroundPid > 0) 
  {
    /* Foward SIGTSTP to the currently running foreground process. */
    kill(foregroundPid, SIGTSTP);

    /* DONE: Add foregroundcommand to the list of jobs. */
    for (int t = 0; t < cmdLength; t++)
    {
      if (cmdList [t]->num == 0)
      {
        cmdList [t]->status = STOPPED;
        cmdList [t]->num = bgNum;
        bgNum++;

        printf ("\tstop pid: %d\n", cmdList [t]->pid);

        printf ("[%d] ", cmdList [t]->num);
        printf ("\tStopped. ");
        printf ("\t\t%s", cmdList [t]->line);

        break;
      }
    }

    return;
  }
} 

void addCmd2 (Cmd * cmd)
{
  cmdLength++;

  *cmdList = (Cmd *) realloc(*cmdList, cmdLength * sizeof (Cmd));
  cmdList [cmdLength - 1] = cmd;

  if (cmd->num == 0) foregroundPid = cmd->pid;
}

void removeCmd2 (int index)
{
  cmdLength --;

  if ((cmdList [index]->num == 0) && (cmdList [index]->pid == foregroundPid)) foregroundPid = 0;

  for (int t = index; t < cmdLength; t++)
  {
    for (int i = 0; i < MAX_ARGS; i++)
    {
      cmdList [t]->args [i] = cmdList [t + 1]->args [i];
      cmdList [t]->symbols [i] = cmdList [t + 1]->symbols [i];
    }

    strcpy (cmdList [t]->line, cmdList [t + 1]->line);
    strcpy (cmdList [t]->tokenLine, cmdList [t + 1]->tokenLine);

    cmdList [t]->pid = cmdList [t + 1]->pid;
    cmdList [t]->status = cmdList [t + 1]->status;
    cmdList [t]->num = cmdList [t + 1]->num;
  }

  *cmdList = (Cmd *) realloc(*cmdList, cmdLength * sizeof (Cmd));

  if (cmdLength == 0) bgNum = 1;
}

void jobs ()
{
  for (int t = 0; t < cmdLength; t++)
  {
    if (cmdList [t]->num == 0 && cmdList [t]->status != STOPPED) continue;

    printf ("[%d] ", cmdList [t]->num);
    if (cmdList [t]->status == STOPPED)
      printf ("\tStopped. ");
    else if (cmdList [t]->status == RUNNING)
      printf ("\tRunning. ");
    else
      printf ("\tUncertain?[Error] ");
    //printf ("\t\t%s", cmdList [t]->line);
    for (int r = 0; r < MAX_ARGS; r++) 
    {
      if (cmdList [t]->args [r] != NULL) printf (" %s", cmdList [t]->args [r]);
      else break;
    }
    printf ("\n");
  }
}

void bg (int num)
{
  for (int t = 0; t < cmdLength; t++)
  {
    if ((cmdList [t]->num == num) && (cmdList [t]->status == STOPPED))
    {
      
      kill (cmdList [t]->pid, SIGCONT);
      cmdList [t]->status = RUNNING;

      printf ("[%d] ", cmdList [t]->num);
      printf ("\tRunning. ");
      //printf ("\t\t%s", cmdList [t]->line);
      for (int r = 0; r < MAX_ARGS; r++) 
      {
        if (cmdList [t]->args [r] != NULL) printf (" %s", cmdList [t]->args [r]);
        else break;
      }
      printf ("\n");
    } 
    else if (cmdList [t]->num == num)
      printf ("This job has not been stopped..");
  }
}

void killBG (pid_t pid)
{
  for (int t = 0; t < cmdLength; t++)
  {
    if (cmdList [t]->pid == pid)
    {
      kill (cmdList [t]->pid, SIGTERM);
      if (waitpid (cmdList [t]->pid, NULL, WNOHANG) == -1)
      {
        printf ("[%d] ", cmdList [t]->num);
        printf ("\tTerminated. ");
        //printf ("\t\t%s", cmdList [t]->line);
        for (int r = 0; r < MAX_ARGS; r++) 
        {
          if (cmdList [t]->args [r] != NULL) printf (" %s", cmdList [t]->args [r]);
          else break;
        }
        printf ("\n");
      }
      else printf ("Could not terminate..");

      removeCmd2 (t);
      t--;
    } 
  }
}

int main(void) 
{
  /* Listen for control+z (suspend process). */
  signal(SIGTSTP, sigtstpHandler);

  //*cmdList = malloc (sizeof (Cmd));
  
  while (1) 
  {
    printf("352> ");
    fflush(stdout);
    Cmd *cmd = (Cmd*) calloc(1, sizeof(Cmd));
    fgets(cmd->line, MAX_LINE, stdin);
    parseCmd(cmd);
    if (!cmd->args[0]) 
    {
      free(cmd);
    } 
    else if (strcmp(cmd->args[0], "exit") == 0) 
    {
      free(cmd);
      exit(0);

      /* DONE: Add built-in commands: jobs and bg. */
    }
    else 
    {
      if (findSymbol(cmd, BG_OP) != -1) 
      {
        /* DONE: Run command in background.                       */

        pid_t pid = fork();
        cmd->pid = pid;

        if (cmd->pid < 0)//Failed process
        {
          printf ("Forking failed...");
        }
        else if (cmd->pid != 0)//Parent process
        {
          cmd->status = RUNNING;
          cmd->num = bgNum;
          bgNum++;

          if (setpgid(cmd->pid, 0) != 0) 
            perror("setpgid() error");

          addCmd2 (cmd);

          printf ("[%d] ", cmd->num);
          printf ("%d\n", cmd->pid);

          if (strcmp (cmd->args [0], "bg") == 0) bg (atol (cmd->args [1]));
        }
        else//Child process
        {

          int input, output = -1;
          input = findSymbol(cmd, REDIRECT_IN_OP);
          output = findSymbol(cmd, REDIRECT_OUT_OP);

          if (input != -1)
          {
            int fd = open (cmd->args [input + 1], O_RDONLY);
            dup2 (fd, STDIN_FILENO);//0
          }
          else if (output != -1)
          {
            int fd = open (cmd->args [output + 1], O_WRONLY|O_TRUNC);
            dup2 (fd, STDOUT_FILENO);//1
          }
          if (strcmp (cmd->args [0], "jobs") == 0) 
          {
            jobs ();
            exit (0);
          }
          else if (strcmp (cmd->args [0], "bg") == 0) 
          {
            exit (0);
          }
          else if (strcmp (cmd->args [0], "kill") == 0) 
          {
            killBG (atol (cmd->args [1]));
            exit (0);
          }
          else if (execvp (cmd->args [0], cmd->args) < 0)
            printf ("Command could not execute...\n");
        }
      } 
      else 
      {
        /* DONE:: Run command in foreground. */
        
        pid_t pid = fork();
        cmd->pid = pid;

        if (cmd->pid < 0)//Failed process
        {
          printf ("Forking failed...");
        }
        else if (cmd->pid != 0)//Parent process
        {
          cmd->status = RUNNING;
          cmd->num = 0;

          if (setpgid(pid, 0) != 0) 
            perror("setpgid() error");

          addCmd2 (cmd);

          if (strcmp (cmd->args [0], "bg") == 0) bg (atol (cmd->args [1]));
          
          waitpid (pid, NULL, WUNTRACED);
        }
        else//Child process
        {
          int input, output, pipeput = -1;
          input = findSymbol(cmd, REDIRECT_IN_OP);
          output = findSymbol(cmd, REDIRECT_OUT_OP);
          pipeput = findSymbol(cmd, PIPE_OP);

          if (input != -1)
          {
            int fd = open (cmd->args [input + 1], O_RDONLY);
            dup2 (fd, STDIN_FILENO);//0
          }
          else if (output != -1)
          {
            int fd = open (cmd->args [output + 1], O_WRONLY|O_TRUNC);
            dup2 (fd, STDOUT_FILENO);//1
          }
          
          if (pipeput != -1)
          {
            int pipefd [2];
            if (pipe (pipefd) < 0) 
            {
              printf ("Pipe error!\n");
              return 1;
            }

            pipeput++;//For indicating first argument after pipe
            Cmd *cmdT = (Cmd*) calloc(1, sizeof(Cmd));
            int t = 0;
            for (t = 0; t < MAX_ARGS - pipeput; t++)
            {
              cmdT->args [t] = cmd->args [t + pipeput];
              cmdT->symbols [t] = cmd->symbols [t + pipeput];
            }

            //Child 2
            int pid2 = fork ();
            if (0 == pid2) 
            { 
              close (pipefd [1]); 
              dup2 (pipefd [0], STDIN_FILENO);
              close (pipefd [0]);

              if (strcmp (cmdT->args [0], "jobs") == 0) 
              {
                jobs ();
                exit (0);
              }
              else if (strcmp (cmdT->args [0], "bg") == 0) 
              {
                exit (0);
              }
              else if (strcmp (cmdT->args [0], "kill") == 0) 
              {
                killBG (atol (cmdT->args [1]));
                exit (0);
              }
              else if (execvp (cmdT->args [0/*pipeput + 1*/], cmdT->args) < 0)
                printf ("Child 2 error!\n");

              return 0;
            }

            //Child 1
            int pid1 = fork ();
            if (0 == pid1) 
            { 
              close (pipefd [0]); 
              dup2 (pipefd [1], STDOUT_FILENO);
              close (pipefd [1]);

              if (strcmp (cmd->args [0], "jobs") == 0) 
              {
                jobs ();
                exit (0);
              }
              else if (strcmp (cmd->args [0], "bg") == 0) 
              {
                exit (0);
              }
              else if (strcmp (cmd->args [0], "kill") == 0) 
              {
                killBG (atol (cmd->args [1]));
                exit (0);
              }
              else if (execvp (cmd->args [0/*pipeput - 1*/], cmd->args) < 0)
                printf ("Child 1 error!\n");
              
              return 0;
            }

            close (pipefd[0]); 
            close (pipefd[1]);

            if (setpgid(pid1, 0) != 0) 
              perror("setpgid() error");

            if (setpgid(pid2, 0) != 0) 
              perror("setpgid() error");

            cmd->status = RUNNING;
            cmd->num = 0;
            addCmd2 (cmd);

            if (strcmp (cmd->args [0], "bg") == 0) bg (atol (cmdT->args [1]));

            waitpid (pid1, NULL, WUNTRACED);
            waitpid (pid2, NULL, WUNTRACED);

            free(cmdT);

            return 0;
          }
          if (strcmp (cmd->args [0], "jobs") == 0) 
          {
            jobs ();
            exit (0);
          }
          else if (strcmp (cmd->args [0], "bg") == 0) 
          {
            exit (0);
          }
          else if (strcmp (cmd->args [0], "kill") == 0) 
          {
            killBG (atol (cmd->args [1]));
            exit (0);
          }
          else if (execvp (cmd->args [0], cmd->args) < 0)
            printf ("Command could not execute...\n");
        }
      }
    }
    /* DONE: Check on status of background processes. */
    for (int t = 0; t < cmdLength; t++)
    {
      if (cmdList [t]->status == STOPPED) continue;
      int *status = NULL;
      int processing = waitpid (cmdList [t]->pid, status, WNOHANG);

      if (processing == 0) cmdList [t]->status = RUNNING;
      else if (processing == -1 && status != NULL)
      {
        printf ("[%d] ", cmdList [t]->num);
        printf ("\tExit. ");
        printf ("%d ", *status);
        //printf ("\t\t%s", cmdList [t]->line);
        for (int r = 0; r < MAX_ARGS; r++) 
        {
          if (cmdList [t]->args [r] != NULL) printf (" %s", cmdList [t]->args [r]);
          else break;
        }
        printf ("\n");
        removeCmd2 (t);
        t--;
      }
      else if (cmdList [t]->num != 0)
      {
        printf ("[%d] ", cmdList [t]->num);
        printf ("\tDone! ");
        //printf ("\t\t%s", cmdList [t]->line);
        for (int r = 0; r < MAX_ARGS; r++) 
        {
          if (cmdList [t]->args [r] != NULL) printf (" %s", cmdList [t]->args [r]);
          else break;
        }
        printf ("\n");
        removeCmd2 (t);
        t--;
      }
    }

  }
  return 0;
}