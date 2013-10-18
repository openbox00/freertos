/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* print */
#include "print.h"

#define MAX_ARGC 9
#define MAX_CMDNAME 19
#define MAX_CMDHELP 100
#define HISTORY_COUNT 5
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
void show_cmd_info(int argc, char *argv[], int *num);
void show_echo(int argc, char* argv[], int *num);
void show_history(int argc, char *argv[], int *num);
void show_task_info(int argc, char* argv[], int *num);

/* Enumeration for command types. */
enum {
	CMD_HELP = 0,
	CMD_HISTORY,
	CMD_ECHO,
	CMD_PS,
	CMD_COUNT
} CMD_TYPE;

/* Structure for command handler. */
typedef struct {
	char cmd[MAX_CMDNAME + 1];
	void (*func)(int, char**, int**);
	char description[MAX_CMDHELP + 1];
} hcmd_entry;

const hcmd_entry cmd_data[CMD_COUNT] = {
	[CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."},	
	[CMD_HISTORY] = {.cmd = "history", .func = show_history, .description = "Show latest commands entered."}, 
	[CMD_ECHO] = {.cmd = "echo", .func = show_echo, .description = "Show words you input."},
	[CMD_PS] = {.cmd = "ps", .func = show_task_info, .description = "List all the processes."}
	
};

void show_task_info(int argc, char* argv[], int *num)
{
	portCHAR buf[100];
	vTaskList(buf);
	print("Name\t\tState  Priority Stack  Num");
    print("%s", buf); 
}

//echo but can't solve '' "" situation 
void show_echo(int argc, char* argv[], int *num)
{
	int i = 1;
	int j = 0;
	for(;i<argc;i++){
		print("%s",argv[i]);
		for(j;j<num[i];j++){
			print(" ");
		}		
	}
	print("\n");
}

void show_history(int argc, char *argv[], int *num)
{
	int i;
	for (i = cur_his+1; i <= cur_his + HISTORY_COUNT; i++) {
		if (cmd[i % HISTORY_COUNT][0]) {
			print("%s", cmd[i % HISTORY_COUNT]);
			print("%s",next_line);
		}
	}
}

//help
void show_cmd_info(int argc, char* argv[], int *num)
{
	char *help_desp = "This system has commands as follow\n\r\0";
	int i;

	print("%s",help_desp);
	for (i = 0; i < CMD_COUNT; i++) {
		print("%s",cmd_data[i].cmd);
		print("\t: ");
		print("%s",cmd_data[i].description);
		print("%s",next_line);
	}
}

/* ref tim37021 */
int cmdtok(char *argv[], char *cmd, int *num)
{
	char tmp[CMDBUF_SIZE];
	int i = 0;
	int j = 0;
	int flag;

	int x = -1;
	
	while (*cmd != '\0'){
		if(*cmd == ' '){
			if (flag){
				num[x]++;
			}
			cmd++;
		}else{
			flag = 1;
			while (1) {
				if ((*cmd != ' ') && (*cmd != '\0')){
					tmp[i++] = *cmd;
					cmd++;
				}
				else { 
				tmp[i] = '\0';
				i = 0;
				break;
				}		
			}
			strcpy(argv[j++],tmp);
			x++;
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
	
	/* for echo used*/
	int num[10] = {NULL};
	
	argc = cmdtok(argv, cmdstr, num);



	for (i = 0; i < CMD_COUNT; i++) {
		if (!strcmp(cmd_data[i].cmd, argv[0])) {
			cmd_data[i].func(argc, argv, num);
			break;
		}
	}

	if (i == CMD_COUNT) {
		print("%s",argv[0]);
		print(": command not found");
		print("%s",next_line);
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

		print("%s",str);

		while (1) {
			put_ch = receive_byte();			

			if (put_ch == '\r' || put_ch == '\n') {
				*p = '\0';
				print("%s",next_line);
				break;
			}
			else if (put_ch== 127 || put_ch == '\b') {
				if (p > &cmd[cur_his][0]) {
					p--;
					print("\b \b");
				}
			}
			else if (p - &cmd[cur_his][0] < CMDBUF_SIZE - 1) {
				*(p++) = put_ch;
				print("%c",put_ch);
			}	
		}
		check_keyword();		
	}
}






