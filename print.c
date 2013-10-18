#include <stdarg.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*for print*/
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

/* for ps , ref zzz0072 */
size_t strlen(const char *string)
{
    size_t chars = 0;

    while(*string++) {
        chars++;
    }
    return chars;
}

char *strcat(char *dest, const char *src)
{
    size_t src_len = strlen(src);
    size_t dest_len = strlen(dest);

    if (!dest || !src) {
        return dest;
    }

    memcpy(dest + dest_len, src, src_len + 1);
    return dest;
}

int puts(const char *msg)
{
    if (!msg) {
        return -1;
    }

    return (int)fio_write(1, msg, strlen(msg));
}

static int printf_cb(char *dest, const char *src)
{
    return puts(src);
}

static int sprintf_cb(char *dest, const char *src)
{
    return (int)strcat(dest, src);
}

typedef int (*proc_str_func_t)(char *, const char *);

/* Common body for sprintf and printf */
static int base_printf(proc_str_func_t proc_str, \
                char *dest, const char *fmt_str, va_list param)
{
    char param_chr[] = {0, 0};
    int param_int = 0;

    char *str_to_output = 0;
    int curr_char = 0;

    /* Make sure strlen(dest) is 0
* for first strcat */
    if (dest) {
        dest[0] = 0;
    }

    /* Let's parse */
    while (fmt_str[curr_char]) {
        /* Deal with normal string
* increase index by 1 here */
        if (fmt_str[curr_char++] != '%') {
            param_chr[0] = fmt_str[curr_char - 1];
            str_to_output = param_chr;
        }
        /* % case-> retrive latter params */
        else {
            switch (fmt_str[curr_char]) {
                case 'S':
                case 's':
                    {
                        str_to_output = va_arg(param, char *);
                    }
                    break;

                case 'd':
                case 'D':
                case 'u':
                    {
                       param_int = va_arg(param, int);
                       itoa(param_int, str_to_output);
                       //str_to_output = itoa(param_int);
                    }
                    break;

                case 'X':
                case 'x':
                    {
                       //param_int = va_arg(param, int);
                       //str_to_output = htoa(param_int);
                    }
                    break;

                case 'c':
                case 'C':
                    {
                        param_chr[0] = (char) va_arg(param, int);
                        str_to_output = param_chr;
                        break;
                    }

                default:
                    {
                        param_chr[0] = fmt_str[curr_char];
                        str_to_output = param_chr;
                    }
            } /* switch (fmt_str[curr_char]) */
            curr_char++;
        } /* if (fmt_str[curr_char++] == '%') */
        proc_str(dest, str_to_output);
    } /* while (fmt_str[curr_char]) */

    return curr_char;
}

int sprintf(char *str, const char *format, ...)
{
    int rval = 0;
    va_list param = {0};

    va_start(param, format);
    rval = base_printf(sprintf_cb, (char *)str, format, param);
    va_end(param);

    return rval;
}