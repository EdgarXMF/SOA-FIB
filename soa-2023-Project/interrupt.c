/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <sched.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

char buffer_circular[876];
int escrever = 0;
int leer = 0;
int entero = 0;

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

int zeos_ticks = 0;
extern page_table_entry PT[NR_TASKS];
extern Byte phys_mem[TOTAL_PAGES];

void clock_routine()
{
  zeos_show_clock();
  zeos_ticks ++;
  
  schedule();
}

char bufft[67];
void keyboard_routine()
{
  
  unsigned char c = inb(0x60);
  if (c&0x80){
    if(entero<876){
    buffer_circular[escrever] = char_map[c&0x7f];
    escrever++;
    if(escrever>=876) escrever=0;
    entero++;
    }
  }
 

  //if (c&0x80) printc_xy(0, 0, char_map[c&0x7f]);


}

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

void clock_handler();
void keyboard_handler();
void system_call_handler();
void pagefault_handler(void);
int get_cr2();
void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

char buff[256];
char Hex[24];
char bufona[200];
void pagefault_routine(unsigned long a,unsigned long b){
  int zonainicialdata=NUM_PAG_CODE+NUM_PAG_KERNEL;
  //printk("page fault rechulon\n");
  int c = get_cr2();
  /*itoa(c,bufona);
    printk(bufona);*/
  int id_logic_cr2=c>>12;
  if(((c>>12)<((PAG_LOG_INIT_DATA))) || ((c>>12)>((PAG_LOG_INIT_DATA+NUM_PAG_DATA-1)))){
    //printk("liada");
    decToHex(b,Hex);
    itoa(b,Hex);
    printk("\nprocess generates a PAGE FAULT exception at EIP: 0x");
    printk(Hex);
    while(1);
  } 
   
  page_table_entry *process_PT = get_PT(current());
  
    itoa(phys_mem[get_frame(process_PT, c>>12)],Hex);
    printk(Hex);
  if(phys_mem[get_frame(process_PT, c>>12)] == 1){
    //printk("tres\n");
    process_PT[c>>12].bits.rw=1;
    //void (*llamada)() = (void (*)())b; 
    //llamada();
  }else if(phys_mem[get_frame(process_PT, c>>12)] > 1){
    //printk("\nquatro\n");
    int frame_free=alloc_frame();
    set_ss_pag(process_PT,NUM_PAG_DATA+NUM_PAG_CODE+NUM_PAG_KERNEL,frame_free);
    copy_data((void*)(id_logic_cr2<<12), (void*)((NUM_PAG_DATA+NUM_PAG_CODE+NUM_PAG_KERNEL)<<12), PAGE_SIZE);
    del_ss_pag(process_PT, NUM_PAG_DATA+NUM_PAG_CODE+NUM_PAG_KERNEL);
    del_ss_pag(process_PT, c>>12);
    set_ss_pag(process_PT, c>>12, frame_free);
    //printk("endo");
    //set_cr3(current());
    //phys_mem[frame_free]+=1;
    phys_mem[get_frame(process_PT, c>>12)]-=1;
    //process_PT[c>>12].bits.rw=1;
    //void (*llamada)() = (void (*)())(b-1); 
    //llamada();
  
  }else{
    decToHex(b,Hex);
    itoa(b,Hex);
    printk("\nprocess generates a PAGE FAULT exception at EIP: 0x");
    printk(Hex);
    while(1);
  }
  

	
}


void setSysenter()
{
  setMSR(0x174, 0, __KERNEL_CS);
  setMSR(0x175, 0, INITIAL_ESP);
  setMSR(0x176, 0, (unsigned long)system_call_handler);
}

void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);
  setInterruptHandler(14, pagefault_handler, 0);

  setSysenter();

  set_idt_reg(&idtR);
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




