#include <stdarg.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

void send_str(char *str)
{
	int curr_char = 0;
	while (str[curr_char] != '\0') {
		send_byte(str[curr_char]);
		curr_char++;
	}
}

void itoa(int n, char *buffer)
{
	if (n == 0)
		*(buffer++) = '0';
	else {
		int f = 10000;

		if (n < 0) {
			*(buffer++) = '-';
			n = -n;
		}

		while (f != 0) {
			int i = n / f;
			if (i != 0) {
				*(buffer++) = '0'+(i%10);;
			}
			f/=10;
		}
	}
	*buffer = '\0';
}


/* ref PJayChen */
void print(const char *format, ...)
{
        va_list ap;
        va_start(ap, format);
        int curr_ch = 0;
        char out_ch[2] = {'\0', '\0'};
        char percentage[] = "%";
        char *str;
        char str_num[10];
        int out_int;

        while( format[curr_ch] != '\0' ){
            if(format[curr_ch] == '%'){
                if(format[curr_ch + 1] == 's'){
                    str = va_arg(ap, char *);
					send_str(str);
					//parameter(...,The address of a pointer that point to the string which been put in queue,...)
				}else if(format[curr_ch + 1] == 'd'){
                    itoa(va_arg(ap, int), str_num);
					send_str(str_num);
				}else if(format[curr_ch + 1] == 'c'){
					/* char are converted to int then pushed on the stack */
					out_ch[0] =(char) va_arg(ap, int);
					out_ch[1] = '\0';					
					send_str(out_ch); 
				}else if(format[curr_ch + 1] == '%'){
					send_str(percentage); 
				}
			curr_ch++;
            }
			else{
                out_ch[0] = format[curr_ch];
				send_str(&out_ch);
            }
            curr_ch++;
        }//End of while
        va_end(ap);
}
