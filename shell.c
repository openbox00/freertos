/**/
#include <stddef.h>
#include "systemdef.h"
#include <ctype.h> 

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define MAX_ARGC 19
#define HISTORY_COUNT 10
#define CMDBUF_SIZE 100
#define MAX_ENVVALUE 127
#define MAX_ENVNAME 15


#define Backspace 127

char next_line[3] = {'\n','\r','\0'};
char backspace[3] = {'\b',' ','\b'};
char cmd[HISTORY_COUNT][CMDBUF_SIZE];
int cur_his=0;


void send_str(char *str)
{
	int curr_char;
	curr_char = 0;
	while (str[curr_char] != '\0') {
		send_byte(str[curr_char]);
		curr_char++;
	}
}


void shell(void *pvParameters)
{
	char put_ch;
	char *p = NULL;
	char *str ="\rShell:~$";	

	for (;; cur_his = (cur_his + 1) % HISTORY_COUNT) {
		p = cmd[cur_his];

		send_str(str);

		while (1) {
			put_ch = receive_byte();			

			if (put_ch == '\r' || put_ch == '\n') {
				*p = '\0';
				send_str(next_line);
				break;
			}
			else if (put_ch== Backspace || put_ch == '\b') {
				if (p > cmd[cur_his]) {
					p--;
					send_str(backspace);
				}
			}
			else if (p - cmd[cur_his] < CMDBUF_SIZE - 1) {
				*p++ = put_ch;
				send_byte(put_ch);
			}
	
		}
//		check_keyword();		
	}
}	












