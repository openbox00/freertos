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

/*mmtest*/
struct slot {
    void *pointer;
    unsigned int size;
    unsigned int lfsr;
};

#define CIRCBUFSIZE 50  //5000 too big
unsigned int write_pointer, read_pointer;
static struct slot slots[CIRCBUFSIZE];
static unsigned int lfsr = 0xACE1;
/*mmtest*/

/* Command handlers. */
void show_cmd_info(int argc, char *argv[]);
void show_echo(int argc, char* argv[]);
void show_history(int argc, char *argv[]);
void show_task_info(int argc, char* argv[]);
void show_mmtest(int argc, char* argv[]);
void show_mem_info(int argc, char* argv[]);

/* Enumeration for command types. */
enum {
	CMD_HELP = 0,
	CMD_HISTORY,
	CMD_ECHO,
	CMD_MMTEST,
	CMD_PS,
	CMD_MEMINFO,
	CMD_COUNT
} CMD_TYPE;

/* Structure for command handler. */
typedef struct {
	char cmd[MAX_CMDNAME + 1];
	void (*func)(int, char**);
	char description[MAX_CMDHELP + 1];
} hcmd_entry;

const hcmd_entry cmd_data[CMD_COUNT] = {
	[CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."},	
	[CMD_HISTORY] = {.cmd = "history", .func = show_history, .description = "Show latest commands entered."}, 
	[CMD_ECHO] = {.cmd = "echo", .func = show_echo, .description = "Show words you input."},
	[CMD_MMTEST] = {.cmd = "mmtest", .func = show_mmtest, .description = "Test memory allocate and free."},
	[CMD_PS] = {.cmd = "ps", .func = show_task_info, .description = "List all the processes."},
	[CMD_MEMINFO] = {.cmd = "meminfo", .func = show_mem_info, .description = "Show memory info."}	
};


/* meminfo  Ref PJayChen*/
void show_mem_info(int argc, char* argv[]){
	print("Maximun size  : %d (0x%x) byte\n\r", configTOTAL_HEAP_SIZE, configTOTAL_HEAP_SIZE);
	print("Free Heap Size: %d (0x%x) byte\n\r", xPortGetFreeHeapSize(), xPortGetFreeHeapSize());
}

/* char to integar */
int atoi(const char *str){
	int result = 0;
	while (*str != '\0'){
		result = result * 10;
		result = result + *str - '0';
		str++;
	}
	return result;
}

static unsigned int circbuf_size(void)
{
    return (write_pointer + CIRCBUFSIZE - read_pointer) % CIRCBUFSIZE;
}

/* no exit function so i add else to replace exit*/
static void write_cb(struct slot foo)
{
    if (circbuf_size() == CIRCBUFSIZE - 1) {
        print("circular buffer overflow\n");
    }else{
    slots[write_pointer++] = foo;
    write_pointer %= CIRCBUFSIZE;
	}
}

static struct slot read_cb(void)
{
    struct slot foo;
    if (write_pointer == read_pointer) {
        // circular buffer is empty
        return (struct slot){ .pointer=NULL, .size=0, .lfsr=0 };
    }
    foo = slots[read_pointer++];
    read_pointer %= CIRCBUFSIZE;
    return foo;
}


// Get a pseudorandom number generator from Wikipedia
static int prng(void)
{
    static unsigned int bit;
    /* taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    return lfsr & 0xffff;
}

/* mmtest */
void show_mmtest(int argc, char* argv[])
{
	int i, size;
	char *p;

	size = prng() & 0x7FF;

   	print("try to allocate %d bytes\n\r", size);
	p = (char *) pvPortMalloc(size);
	print("malloc returned %x\n\r", p);

    if (p == NULL) {
        // can't do new allocations until we free some older ones
        while (circbuf_size() > 0) {
            // confirm that data didn't get trampled before freeing
            struct slot foo = read_cb();
            p = foo.pointer;
            lfsr = foo.lfsr;  // reset the PRNG to its earlier state
            size = foo.size;
            print("free a block, size %d\n\r", size);
            for (i = 0; i < size; i++) {
                unsigned char u = p[i];
                unsigned char v = (unsigned char) prng();
                if (u != v) {
                    print("OUCH: u=%X, v=%X\n\r", u, v);
                    return 1;
                    }
                }
                vPortFree(p);
                if ((prng() & 1) == 0) break;
            }
        } else {
            print("allocate a block, size %d\n\r", size);
            write_cb((struct slot){.pointer=p, .size=size, .lfsr=lfsr});
            for (i = 0; i < size; i++) {
                p[i] = (unsigned char) prng();
            }
        }
}
/* mmtest */

/*ref ref zzz0072, PJayChen*/
void show_task_info(int argc, char* argv[])
{
	portCHAR buf[100];
	vTaskList(buf);
	print("Name\t\tState Priority Stack\tNum");
    print("%s", buf); 
}

//echo but can't solve '' "" situation 
void show_echo(int argc, char* argv[])
{
	int i = 1;
	int j = 0;
	for(;i<argc;i++){
		print("%s ",argv[i]);	
	}
	print("\n");
}

//history
void show_history(int argc, char *argv[])
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
void show_cmd_info(int argc, char* argv[])
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
int cmdtok(char *argv[], char *cmd)
{
	char tmp[CMDBUF_SIZE];
	int i = 0;
	int j = 0;
	int flag;

	int x = -1;
	
	while (*cmd != '\0'){
		if(*cmd == ' '){
			cmd++;
		}
		else {
			while (1) {
				/* solve "" & '' in echo command*/
				if ((*cmd == '\'') || (*cmd == '\"')){
					cmd++;
				}
				else if ((*cmd != ' ') && (*cmd != '\0')){
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
	
	argc = cmdtok(argv, cmdstr);

	for (i = 0; i < CMD_COUNT; i++) {
		if (!strcmp(cmd_data[i].cmd, argv[0])) {
			cmd_data[i].func(argc, argv);
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





