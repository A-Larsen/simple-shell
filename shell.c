#include "shell.h"

#define CTRL_KEY(k) ((k) & 0x1f)

typedef void (*cmd_t)(int, char **);

// eval might be a good thing to use 
// to look a memory in program

char CMD[255] = {};
int CMDIND = 0;
int PS1LEN = 0;

static char **CMDHISTORY;
static char **CMDHISTORYP;
static int CMDHISTORYIND = 0;
static int BACKIDX = 0;
FILE *FP_HISTORY;

void
file_MoveLinesBack(fp, num)
  FILE *fp;
  int num;
{

	rewind(fp);
	fpos_t fpos;
	fgetpos(fp, &fpos);
	fseek(fp, 0, SEEK_END);

	long int size = ftell(fp);

	char ch = 0;
	int b = 0;

	int count = 0;

	int i = 0;
	while(count <= num && i <= size){
		if(ch == '\n'){
			count++;
		}

		fseek(fp, b, SEEK_END);
		ch = fgetc(fp);
		i++;
		b--;
	}

	long int pos = ftell(fp);

	if(pos == 1){
		fseek(fp, b+1, SEEK_END);
	}else{

		fseek(fp, b+3, SEEK_END);
	}
}

static void
addToHistEnd(a, str)
  char **a;
  char *str;
{
	CMDHISTORY = realloc(CMDHISTORY, HISTORYLEN * 8);

	for(int i = 1; i < HISTORYLEN; i++){
		CMDHISTORY[i-1] = a[i];
	}

	CMDHISTORY[HISTORYLEN-1] = strdup(str);
}


/* extern const void *COMMANDS[CMDLEN]; */

void
drawPS1()
{
	write(STDOUT_FILENO, "\r\n", 2);
	write(STDOUT_FILENO, PS1, PS1LEN);
}

void 
cli_clear()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	write(STDOUT_FILENO, PS1, PS1LEN);
}

void
cli_write( const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char str[255];
	int len = vsprintf(str, fmt, args);

	write(STDOUT_FILENO, str, len);
	va_end(args);
}

void die(const char *s)
{
	perror(s);
	exit(1);
}

// command calls
void 
cmd_echo(args, argv)
	int args;
	char **argv;
{
	if(!args){
		cli_write( "\r\nno enough argument");
	}

	for(int i = 0; i < args; i++){
		cli_write("\r\n%s", argv[i]);
	}

	drawPS1();
}
void 
cmd_clear(args, argv)
	int args;
	char **argv;
{
	cli_clear();
}

void 
cmd_exit(args, argv)
	int args;
	char **argv;
{
	cli_write("\r\nexiting\r\n");
	exit(0);
}

void
notcmd()
{
	cli_write("\r\nnot a command");
	drawPS1();
}

void
cmd_listcmds(args, argv)
	int args;
	char **argv;
{
	char *cmd1 = NULL;
	char *cmd2 = NULL;
	for(int i = 0; i < CMDLEN; i++){

		if(i%CMDDIV == 0){
			cmd1 = (char *)COMMANDS[i];
		}

		if(i%CMDDIV == 2){
			cmd2 = (char *)COMMANDS[i];
		}

		if(cmd1 && cmd2){
			/* int space = 10; */

			/* int len = strlen(cmd1); */
			int space = 15 - strlen(cmd1);
			char seperator[15];

			int i = 0;
			for(; i < space; i++){
				seperator[i] = ' ';
			}
			seperator[i] = '\0';
			
			cli_write("\r\n%s%s%s", cmd1, seperator, cmd2);


			/* cli_write("\r\n%s\t%s", cmd1, cmd2); */

			cmd1 = NULL;
			cmd2 = NULL;
		}
	}

	drawPS1();
}

void *
searchcmds(query)
  char *query;
{
	for(int i = 0; i < CMDLEN; i += CMDDIV){
		if(!strcmp(query, COMMANDS[i])){
			return (void *)COMMANDS[i+1];
		}
	}

	return NULL;
}

// terminal io
struct termios orig_termios;

void disableRawMode(){
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
		die("tcsetattr");
}

void cli_enableRawMode(){
	if(tcgetattr(STDIN_FILENO, &orig_termios) == -1)
		die("tcgetattr");

	atexit(disableRawMode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 2;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw))
		die("tcsetattr");
}

void
cmd_getHistory(args, argv)
  int args;
  char **argv;
{
	for(int i = 0; i < CMDHISTORYIND; i++){

		cli_write("\r\n%s", CMDHISTORY[i]);
	}

	drawPS1();
}


/* char * // free me */
char
cli_getCmd(cmdstr, cmd, args, argv)
  char *cmdstr;
  char *cmd;
  int *args;
  char **argv;
{
	char *right = strdup(cmdstr);
	char *left = strsep(&right, " ");

	/* strlcpy(cmd, left, strlen(left)+1); */
	strncpy(cmd, left, strlen(left)+1);

	while(right != NULL){
		if(right[0] == '"'){
			char tmp[255];
			int i = 0;


			while(*++right != '"'){
				if(i > 255){
					return 0;
				}
				tmp[i++] = *right;
			}

			tmp[i] = '\0';

			if(i > 0){
				*argv++ = strdup(tmp);
				*args += 1;
			}

			right++;

		}else{
			left = strsep(&right, " ");
			if(left[0] != '\0'){
				*argv++ = strdup(left);
				*args += 1;
			}
		}

	}

	return 1;
}

void
cli_runCmd(cmdstr)
  char *cmdstr;
{
	char cmd[255];
	int args = 0;
	char **argv = malloc(255*4);
	if(!cli_getCmd(cmdstr, cmd, &args,  argv)){
		cli_write("\r\nerror: could not parse command");
		drawPS1();
		return;
	}

	cmd_t fun = searchcmds(cmd);

	if(fun){
		fun(args, argv);
	}else{
		notcmd();
	}

	char *tmp = strdup(cmdstr);
	/* int len = strlcat(tmp, "\n", strlen(tmp)+2); */
	int len = strlen(tmp)+2;
	/* strncat(tmp, "\n", strlen(tmp)+2); */
	strncat(tmp, "\n", len);

	fwrite(tmp, len, 1, FP_HISTORY);
	free(tmp);

	if(CMDHISTORYIND < HISTORYLEN){
		CMDHISTORYP[CMDHISTORYIND++] = strdup(cmdstr);
	}

	else{
		addToHistEnd(CMDHISTORYP, cmdstr);
	}
}

int
readKey()
{
	char c = 0;
	if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
		die("read");

	return c;
}

void
cli_removeback()
{
	write(STDOUT_FILENO, "\x1b[D", 3);
	write(STDOUT_FILENO, " ", 1);
	write(STDOUT_FILENO, "\x1b[D", 3);
}

void
cli_keyHandle()
{
	char c = readKey();

	switch(c){

		case 13:{
			cli_runCmd(CMD);
			CMDIND = 0;
			BACKIDX = 0;

			break;
		}

		case CTRL_KEY('p'):{

			if(BACKIDX < CMDHISTORYIND){

				int cmdlen = strlen(CMD) + PS1LEN;

				BACKIDX++;
				int pos = CMDHISTORYIND-BACKIDX;

				if(BACKIDX == 1){

					while(cmdlen--){
						cli_removeback();
					}

					cli_write("%s%s",PS1, CMDHISTORY[pos]);
				}

				else{
					int len = strlen(CMDHISTORY[pos+1]);

					while(len--){
					   cli_removeback();
					}

					cli_write("%s", CMDHISTORY[pos]);
				}

				int len = strlen(CMDHISTORY[pos]);

				/* strlcpy(CMD, CMDHISTORY[pos], */ 
				/* 		len+1); */
				strncpy(CMD, CMDHISTORY[pos], 
						len+1);
				CMDIND = len;
			}


			break;
	   }

		case CTRL_KEY('n'):{
			if(BACKIDX > 0){

				int cmdlen = strlen(CMD);
				BACKIDX--;


				int pos = CMDHISTORYIND-BACKIDX;

				if(BACKIDX == 0){
					while(cmdlen-- > 0){
					   cli_removeback();
					}

					if(cmdlen > 0){
						cli_write("%s", CMD);
					}else{
						CMDIND = 0;
					}

				}

				else{
					int len = strlen(CMDHISTORY[pos-1]);

					while(len--){
						cli_removeback();
					}

					cli_write("%s", CMDHISTORY[pos]);

					int len2 = strlen(CMDHISTORY[pos]);
					/* strlcpy(CMD, CMDHISTORY[pos], */ 
					/* 		len2+1); */
					strncpy(CMD, CMDHISTORY[pos], 
							len2+1);
					CMDIND = len2;
				}

			}
			break;
	   }

		case CTRL_KEY('c'): 
		case CTRL_KEY('d'): {
			cli_write("\r\nexiting\r\n");
			exit(0);
			break;
		}
		case CTRL_KEY('l'): {
			cli_clear();
			break;
		}

		case 127: {
			if((CMDIND+PS1LEN) > PS1LEN){
				CMDIND--;
				cli_removeback();
			}

			break;
	  	}

		default: if(c != 0) {
			char str[1] = {c};

			CMD[CMDIND++] = c;
			CMD[CMDIND] = '\0';
			write(STDOUT_FILENO, str, 1);
			 break;
		 }
	}
}

void
cli_init()
{
	CMDHISTORYP = malloc(HISTORYLEN * 8);
	CMDHISTORY = CMDHISTORYP;
	PS1LEN = strlen(PS1);

	FP_HISTORY = fopen("history", "a+");
	file_MoveLinesBack(FP_HISTORY, HISTORYLEN);
	char line[255];

	while(fgets(line, sizeof(line), FP_HISTORY)){
		CMDHISTORYP[CMDHISTORYIND++] = strndup(line, strlen(line)-1);
	}
	cli_clear();
}

void *
cli_start(data)
  void *data;
{
	cli_enableRawMode();
	cli_init();

	while(1){
		cli_keyHandle();
	}

	/* return NULL; */
}

