/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define MAX_ARGC 9
#define MAX_CMDNAME 19
#define MAX_CMDHELP 100
#define HISTORY_COUNT 10
#define CMDBUF_SIZE 50
#define MAX_ENVVALUE 127
#define MAX_ENVNAME 15

int strcmp(const char *a, const char *b) __attribute__ ((naked));
int strcmp(const char *a, const char *b)
{
	__asm__(
        "strcmp_lop:                \n"
        "   ldrb    r2, [r0],#1     \n"
        "   ldrb    r3, [r1],#1     \n"
        "   cmp     r2, #1          \n"
        "   it      hi              \n"
        "   cmphi   r2, r3          \n"
        "   beq     strcmp_lop      \n"
		"	sub     r0, r2, r3  	\n"
        "   bx      lr              \n"
		:::
	);
}

char next_line[3] = {'\n','\r','\0'};
char cmd[HISTORY_COUNT][CMDBUF_SIZE];
int cur_his=0;

/* Command handlers. */
void show_cmd_info(int argc, char *argv[]);

/* Enumeration for command types. */
enum {
	CMD_HELP = 0,
	CMD_COUNT
} CMD_TYPE;

/* Structure for command handler. */
typedef struct {
	char cmd[MAX_CMDNAME + 1];
	void (*func)(int, char**);
	char description[MAX_CMDHELP + 1];
} hcmd_entry;
const hcmd_entry cmd_data[CMD_COUNT] = {
	[CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."}
};

void send_str(char *str)
{
	int curr_char = 0;
	while (str[curr_char] != '\0') {
		send_byte(str[curr_char]);
		curr_char++;
	}
}

//help
void show_cmd_info(int argc, char* argv[])
{
	char *help_desp = "This system has commands as follow\n\r\0";
	int i;

	send_str(help_desp);
	for (i = 0; i < CMD_COUNT; i++) {
		send_str(cmd_data[i].cmd);
		send_str("\t: ");
		send_str(cmd_data[i].description);
		send_str(next_line);
	}
}

/* ref tim37021 */
int cmdtok(char *argv[], char *cmd)
{
	char tmp[CMDBUF_SIZE];
	int i = 0;
	int j = 0;
	
	while (*cmd != '\0'){
		if(*cmd == ' '){
			cmd++;
		}else{
			while (1) {
				if ((*cmd != ' ') && (*cmd != '\0')){
					tmp[i++] = *cmd;
					cmd++;
				}
				else {
				tmp[i] = '\0';
				break;
				}		
			}
			strcpy(argv[j++],tmp);
		}
	}

	return j;	
}

void check_keyword()
{
	char tok[MAX_ARGC + 1][100];
	char *argv[MAX_ARGC + 1];
	int k = 0;
	
	for (k;k<MAX_ARGC + 1;k++){
	argv[k] = &tok[k][0];
	}
	int i;
	int argc;
	
	char cmdstr[CMDBUF_SIZE];
	strcpy(cmdstr, &cmd[cur_his][0]);
	
	argc = cmdtok(argv,cmdstr);
	

	for (i = 0; i < CMD_COUNT; i++) {
		if (!strcmp(cmd_data[i].cmd, argv[0])) {
			cmd_data[i].func(argc, argv);
			break;
		}
	}

	if (i == CMD_COUNT) {
		send_str(argv[0]);
		send_str(": command not found");
		send_str(next_line);
	}
}

void shell(void *pvParameters)
{
	char put_ch;
	char *p = NULL;
	char *str ="\rShell:~$";	

	for (;; cur_his = (cur_his + 1) % HISTORY_COUNT) {
		/* need use & that p can work correct, idk why p = cmd[cur_his] can't work */
		p = &cmd[cur_his][0];

		send_str(str);

		while (1) {
			put_ch = receive_byte();			

			if (put_ch == '\r' || put_ch == '\n') {
				*p = '\0';
				send_str(next_line);
				break;
			}
			else if (put_ch== 127 || put_ch == '\b') {
				if (p > &cmd[cur_his][0]) {
					p--;
					send_str("\b \b");
				}
			}
			else if (p - &cmd[cur_his][0] < CMDBUF_SIZE - 1) {
				*(p++) = put_ch;				
				send_byte(put_ch);
			}	
		}
		check_keyword();		
	}
}





