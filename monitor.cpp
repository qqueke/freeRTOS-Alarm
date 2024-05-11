//#ifdef notdef

/***************************************************************************
| File: monitor.c
|
| Autor: Carlos Almeida (IST), from work by Jose Rufino (IST/INESC), 
|        from an original by Leendert Van Doorn
| Data:  Nov 2002
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

extern char* my_fgets(char *, int, FILE *);

/*-------------------------------------------------------------------------+
| Headers of command functions
+--------------------------------------------------------------------------*/ 
extern void cmd_sair (int, char** );
extern void cmd_test (int, char** );
extern void cmd_send (int, char** );
extern void cmd_rc (int, char** );
extern void cmd_sc (int, char** );
extern void cmd_rtl (int, char** );
extern void cmd_rp (int, char** );
extern void cmd_mmp (int, char** );
extern void cmd_mta (int, char** );
extern void cmd_mpp (int, char** );
extern void cmd_rai (int, char** );
extern void cmd_dac (int, char** );
extern void cmd_dtl (int, char** );
extern void cmd_aa (int, char** );
extern void cmd_cai (int, char** );
extern void cmd_ir (int, char** );
extern void cmd_lr (int, char** );
extern void cmd_dr (int, char** );
extern void cmd_pr (int, char** );
void cmd_sos  (int, char** );

/*-------------------------------------------------------------------------+
| Variable and constants definition
+--------------------------------------------------------------------------*/ 
const char TitleMsg[] = "\n Application Control Monitor\n";
const char InvalMsg[] = "\nInvalid command!";

struct  command_d {
  void  (*cmd_fnct)(int, char**);
  char* cmd_name;
  char* cmd_help;
}

const commands[] = {
    {cmd_sos,  "sos","                  help"},
    {cmd_send, "send","<msg>            send message"},
    {cmd_sair, "sair","                 sair"},
    {cmd_test, "test","<arg1> <arg2>    test command"},
    {cmd_rc,   "rc","                         help"},
    {cmd_sc,   "sc","<h> <m> <s>              help"},
    {cmd_rtl,  "rtl","                        help"},
    {cmd_rp,   "rp","                         help"},
    {cmd_mmp,  "mmp","<p>                     help"},
    {cmd_mta,  "mta","<t>                     help"},
    {cmd_mpp,  "mpp","<p>                     help"},
    {cmd_rai,  "rai","                        help"},
    {cmd_dac,  "dac","<h> <m> <s>             help"},
    {cmd_dtl,  "dtl","<T> <L>                 help"},
    {cmd_aa,   "aa","<a>                      help"},
    {cmd_cai,  "cai","                        help"},
    {cmd_ir,   "ir","                         help"},
    {cmd_lr,   "lr","<n> <i>                  help"},
    {cmd_dr,   "dr","                         help"},
    {cmd_pr,   "pr","<h> <m> <s> <h> <m> <s>  help"}
};

#define NCOMMANDS  (sizeof(commands)/sizeof(struct command_d))
#define ARGVECSIZE 7
#define MAX_LINE   50

/*-------------------------------------------------------------------------+
| Function: cmd_sos - provides a rudimentary help
+--------------------------------------------------------------------------*/ 
void cmd_sos (int argc, char **argv)
{
    int i;

    printf("%s\n", TitleMsg);
    for (i=0; i<NCOMMANDS; i++)
    printf("%s %s\n", commands[i].cmd_name, commands[i].cmd_help);
}

/*-------------------------------------------------------------------------+
| Function: my_getline        (called from monitor) 
+--------------------------------------------------------------------------*/ 
int my_getline (char** argv, int argvsize)
{
    static char line[MAX_LINE];
    char *p;
    int argc;

    // fgets(line, MAX_LINE, stdin);   
    my_fgets(line, MAX_LINE, stdin);

    /* Break command line into an o.s. like argument vector,
     i.e. compliant with the (int argc, char **argv) specification --------*/

    for (argc=0,p=line; (*line != '\0') && (argc < argvsize); p=NULL,argc++) {
        p = strtok(p, " \t\n");
        argv[argc] = p;
        if (p == NULL) return argc;
    }
    argv[argc] = p;
    return argc;
}

/*-------------------------------------------------------------------------+
| Function: monitor        (called from main) 
+--------------------------------------------------------------------------*/ 
void monitor (void)
{
  static char *argv[ARGVECSIZE+1], *p;
  int argc, i;

  printf("%s Type sos for help\n", TitleMsg);
  for (;;) {
    printf("\nCmd> ");
    
    /* Reading and parsing command line  ----------------------------------*/
    if ((argc = my_getline(argv, ARGVECSIZE)) > 0) {
      for (p=argv[0]; *p != '\0'; *p=tolower(*p), p++);
      
      for (i = 0; i < NCOMMANDS; i++) 
        if (strcmp(argv[0], commands[i].cmd_name) == 0) 
            break;
      
      /* Executing commands -----------------------------------------------*/
    if (i < NCOMMANDS){
        for (int j = 0; j < argc; ++j){
            printf("Argv %d : %s\n", j, argv[j]);
        }

        commands[i].cmd_fnct (argc, argv);

    }
    else  
        printf("%s", InvalMsg);
    
    } /* if my_getline */
  } /* forever */
}

//#endif //notdef
