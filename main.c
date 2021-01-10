#include "shell.h"

const void *
COMMANDS[CMDLEN] = {
	// name        function           description
	".echo",       cmd_echo,          "hello description",
	".help",       cmd_listcmds,      "list all commads",
	".history",    cmd_getHistory,    "list all history",
	".clear",      cmd_clear,         "clear the screen",
	".exit",       cmd_exit,          "exit accord",
};

int main(){
	cli_start(NULL);
}
