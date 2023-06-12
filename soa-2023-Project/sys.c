/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{ 
  void* addr =14879376;//THE FIRST One
  char* ptr = (char*)addr;
    for (int pos = 0; pos < PAGE_SIZE; ++pos){
      *ptr = 0;
      ++ptr;
    }
    
	return current()->PID;
}

int global_PID=1000;

extern Byte x,y;
extern char buffer_circular[876];
extern int escrever;
extern int leer;
extern int entero;
extern int fg,bg;

int  pag_logicas = 1024;
extern struct pos_vector_mem{
  int id_frame_ph;
  int num_ref;
  int delete;
};

extern struct pos_vector_mem vector_de_frames[10];

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    //printk("jiji\n");
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
    //printk("jiji\n");
    process_PT[pag].bits.rw=0;
    parent_PT[pag].bits.rw=0;
    //printk("jiji\n");
    ++phys_mem[get_frame(process_PT, pag)];
    
    /* Map one child page to parent's address space. */
    /*set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);*/
  }

  
  for(int posic=PAG_LOG_INIT_DATA+NUM_PAG_DATA*2;posic<TOTAL_PAGES;posic++){
      if(parent_PT[posic].bits.present==1){
        char bufas[200];
             /*printk("\n");
            itoa(posic,bufas);
            printk(bufas);*/
        set_ss_pag(process_PT,posic,get_frame(parent_PT,posic));
        int aleluya=0;
        while(aleluya<10){
          if(vector_de_frames[aleluya].id_frame_ph==get_frame(parent_PT,posic)){
            vector_de_frames[aleluya].num_ref+=1;
            char bufo[200];
             /*printk("\n");
            itoa(vector_de_frames[aleluya].num_ref,bufo);
            printk(bufo);*/
            aleluya=aleluya+10;
          }
          ++aleluya;
        }
      }
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}

int sys_read(char *buffer, int maxChar) {
  if (maxChar<0) return -EINVAL;
  int bytes_left = maxChar;
  int response = 0;
  if(maxChar>entero){
    return -EINVAL;
  } 
  if (!access_ok(VERIFY_READ, buffer, maxChar)) return -EFAULT;
  while(bytes_left>0){
    
    copy_to_user(&buffer_circular[leer], buffer, sizeof(char));
    buffer++;
    leer=leer+1;
    if(leer>=876) leer=0;
    --bytes_left;
    response=response+1;
  }
  //printk(buffer);
  entero=entero-maxChar;
  return response;
	
}

extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  for (int ioio=NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA*2; ioio<TOTAL_PAGES; ioio++)
  {
    if(process_PT[ioio].bits.present==1){
      for(int cartelera=0;cartelera<10;cartelera++){
        if(vector_de_frames[cartelera].id_frame_ph==get_frame(process_PT,ioio)){
          /*char bufo[200];
             printk("\n");
            itoa(vector_de_frames[cartelera].num_ref,bufo);
            printk(bufo);*/
          vector_de_frames[cartelera].num_ref-=1;
           /*char bufos[200];
             printk("\n");
            itoa(vector_de_frames[cartelera].num_ref,bufos);
            printk(bufos);*/
          cartelera=cartelera+10;
        } 
             
      }
      del_ss_pag(process_PT, ioio);
    }
    
  }

  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

int sys_gotoxy(int x1, int y1)
{
  if(x1<80 && x1>=0 && y1<25 && y1>=0 ){
    x=x1;
    y=y1;
    return 0;
  }
  return -EINVAL;
}


int sys_set_color(int fg1, int bg1)
{
    if(fg1<16 && fg1>=0 && bg1<16 && bg1>=0){
      fg = fg1;
      bg = bg1;
      return 0;
    }
    return -EINVAL;
}


void* sys_shmat(int pos, void* addr)
{ 
  if (((int)addr & 0xFFF)==1) return -EINVAL;
  if (pos<0 || pos>9) return -EINVAL;
  page_table_entry * process_PT =  get_PT(current());
  int page_addr;
  if (addr == NULL || process_PT[(unsigned int)addr>>12].bits.present == 1) { 
  int found = 0;
  
      for(int i=NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA+NUM_PAG_DATA; i<pag_logicas && !found; ++i){
          if(process_PT[i].bits.present==0){
            found = 1;
            page_addr = i;
            /*char baff[200];
            itoa(i,baff);
            printk(baff);*/
            
          } 
      }
      if(!found) return -ENOMEM;
  } else page_addr=(unsigned int)addr>>12;
  /*char baff[200];
            itoa(page_addr,baff);
            printk(baff);*/
  set_ss_pag(process_PT,page_addr,vector_de_frames[pos].id_frame_ph);
  vector_de_frames[pos].num_ref+=1;

  return (page_addr<<12);

}


int sys_shmrm(int pos){
  if (pos<0 || pos>9) return -EINVAL;
   
  vector_de_frames[pos].delete=1; 
  return 0;
  
}

int sys_shmdt(void* addr){
  
  if (addr == NULL && ((unsigned int)addr>>12) <= 256+20*2+8) return -EINVAL;
  
  page_table_entry * process_PT =  get_PT(current());
  if(get_frame(process_PT,((unsigned int)addr>>12))==0) return -EINVAL;
  
  for(int i = 0; i<10; i++){
    if (vector_de_frames[i].id_frame_ph == get_frame(process_PT, ((unsigned int) addr)>>12)){
      //remove referencia
      --vector_de_frames[i].num_ref;
      if (vector_de_frames[i].num_ref == 0 && vector_de_frames[i].delete == 1){
        //reseteo y borro
        char *point_ma = (unsigned long)addr&0xFFFFF000; 
        for(int j = 0; j<PAGE_SIZE; ++j){
          *point_ma = 0;
          point_ma++;
        }
      }
      del_ss_pag(process_PT, ((unsigned int)addr>>12));
      return 0;
    }

  } return -EINVAL;

}

