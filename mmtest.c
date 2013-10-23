/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

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
void mmtest(void)
{
	int i, size;
	char *p;

	size = prng() & 0x7FF;
    
	while (1) {
  //	 	print("try to allocate %d bytes\n\r", size);
		p = (char *) pvPortMalloc(size);
  //		print("malloc returned %x\n\r", p);

    if (p == NULL) {
        // can't do new allocations until we free some older ones
        while (circbuf_size() > 0) {
            // confirm that data didn't get trampled before freeing
            struct slot foo = read_cb();
            p = foo.pointer;
            lfsr = foo.lfsr;  // reset the PRNG to its earlier state
            size = foo.size;
  //          print("free a block, size %d\n\r", size);
            for (i = 0; i < size; i++) {
                unsigned char u = p[i];
                unsigned char v = (unsigned char) prng();
                if (u != v) {
  //                  print("OUCH: u=%X, v=%X\n\r", u, v);
                    return 1;
                    }
                }
                vPortFree(p);
                if ((prng() & 1) == 0) break;
            }
        } else {
  //          print("allocate a block, size %d\n\r", size);
            write_cb((struct slot){.pointer=p, .size=size, .lfsr=lfsr});
            for (i = 0; i < size; i++) {
                p[i] = (unsigned char) prng();
            }
        }
	}
}
/* mmtest */
