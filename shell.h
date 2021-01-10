#ifndef __SHELL__
#define __SHELL__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
/* #include <bsd/string.h> */
#include <string.h>
#include <stdarg.h>

#define CMDLEN 6*3
#define CMDDIV 3
#define PS1  "accord> "
#define HISTORYLEN 4
#define ARGSMAX 8

extern const void *COMMANDS[CMDLEN];

void cmd_echo(int args, char **argv);
void cmd_listcmds(int args, char **argv);
void cmd_clear(int args, char **argv);
void cmd_getHistory(int args, char **argv);
void cmd_exit(int args, char **argv);

void cli_init();
void cli_enableRawMode();
void cli_keyHandle();

void *cli_start(void *data);
void cli_free();


#endif
