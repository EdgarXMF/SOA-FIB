
/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <zeos_interrupt.h>

extern struct task_struct * idle_task;
extern struct task_struct * init_task;

Gate idt[IDT_ENTRIES];
Register    idtR;
int zeos_ticks = 0;
char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','�','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','�',
  '\0','�','\0','�','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void clock_handler(void);
void keyboard_handler(void);
void pagefault_handler(void);
void syscall_handler_sysenter(void);

void wrMSR(unsigned long a, unsigned long b, unsigned long c);

void clock_routine(){
  zeos_ticks = zeos_ticks + 1;
	zeos_show_clock();
  sched();

}

void perror(void);

void keyboard_routine(){
	char value = inb(0x60);
	if (!(value & 0x80)){
     printc_xy(60,20,char_map[value & 0x7F]);
  }
}

int gettime();

int getpid(void);

char buff[256];
char Hex[24];
void pagefault_routine(unsigned long a,unsigned long b){
	decToHex(b,Hex);
	itoa(b,Hex);
	printk("\nprocess generates a PAGE FAULT exception at EIP: 0x");
	printk(Hex);
	while(1);
}



//int write (int fd, char * buffer, int size);
void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;

  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
 setInterruptHandler(32, clock_handler, 0);
 
 setInterruptHandler(33, keyboard_handler, 0);
 
 setInterruptHandler(14, pagefault_handler,0);
  set_idt_reg(&idtR);
  
 	wrMSR(0x174,0,__KERNEL_CS);
	wrMSR(0x175,0,INITIAL_ESP);
	wrMSR(0x176,0,(unsigned long)syscall_handler_sysenter);

}

void decToHex(int decimal, char* hex) {
    int i = 0;
    while(decimal > 0) {
        int remainder = decimal % 16;
        if (remainder < 10) {
            hex[i] = remainder + '0';
        }
        else {
            hex[i] = remainder + 'A' - 10;
        }
        decimal /= 16;
        i++;
    }
    hex[i] = '\0';

    // Reverse the string
    int j = 0, k = i - 1;
    while (j < k) {
        char temp = hex[j];
        hex[j] = hex[k];
        hex[k] = temp;
        j++;
        k--;
    }
}



