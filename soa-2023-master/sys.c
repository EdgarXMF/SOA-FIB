/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern int zeos_ticks;
int pidcounter = 200; //random

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int get_ebp();

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{

	return current()->PID;
}

int ret_from_fork(){
	return 0;
}

int max(int a, int b, int c){
	if(a>b && a>c) return a;
	if(b>a && b>c) return b;
	if(c>a && c>b) return c;
	else return 256;
}

int sys_fork()
{
  int PID=-1;
	
  if(list_empty(&freequeue)) return -ENOMEM;

  struct list_head *lh1 = list_first(&freequeue);
	list_del(lh1);	
	struct task_struct *tshijo = list_head_to_task_struct(lh1);

	copy_data(current(),tshijo,sizeof(union task_union));
  
  allocate_DIR(tshijo);

	
	int numpags=0, paginas[NUM_PAG_DATA], pagocupada;
	for(numpags=0;numpags<NUM_PAG_DATA;numpags++){
		int pagina=alloc_frame();
		if(pagina==-1){
			//si no cabe
			for(pagocupada=0; pagocupada<numpags; ++pagocupada) {
				free_frame(paginas[pagocupada]);
			}
			list_add_tail(lh1,&freequeue);
			return -ENOMEM;

		}
		else paginas[numpags]= pagina;
	} 
	
	page_table_entry *pt_de_mi_bebe = get_PT(tshijo); 
	page_table_entry *pt_del_papa = get_PT(current()); 
	int maximo=max(NUM_PAG_KERNEL,NUM_PAG_CODE,NUM_PAG_DATA);
	
	for(int it=0; it<maximo;++it){
		if(it<NUM_PAG_KERNEL)set_ss_pag(pt_de_mi_bebe, it, get_frame(pt_del_papa,it));
		if(it<NUM_PAG_DATA)set_ss_pag(pt_de_mi_bebe, PAG_LOG_INIT_DATA + it, paginas[it]);
		if(it<NUM_PAG_CODE)set_ss_pag(pt_de_mi_bebe, PAG_LOG_INIT_CODE + it, get_frame(pt_del_papa,NUM_PAG_KERNEL+NUM_PAG_DATA+it));
		
	}
	
	int iniData = NUM_PAG_KERNEL;	//para hacerlo mas intuitivo                                          
	int finData = NUM_PAG_KERNEL+NUM_PAG_DATA;

	//En zeos el orden de la memoria logica es: kernel, data+stack, code. Segun el pdf.
	for(int posi=iniData; posi < finData;++posi){
		set_ss_pag(pt_del_papa,(posi+NUM_PAG_DATA+NUM_PAG_CODE),get_frame(pt_de_mi_bebe,posi));
		copy_data((void*)(posi<<12), (void*)((NUM_PAG_CODE+NUM_PAG_DATA+posi)<<12),PAGE_SIZE);
		del_ss_pag(pt_del_papa,NUM_PAG_DATA+NUM_PAG_CODE+posi);
	}
	
	set_cr3(get_DIR(current()));

	tshijo->PID = ++pidcounter;
	init_stats(&(tshijo->estatistiques));
	tshijo->quantum=15;
	
	union task_union *tuhijo = (union task_union*)tshijo;
	tuhijo->stack[KERNEL_STACK_SIZE-19]=0;
	tuhijo->stack[KERNEL_STACK_SIZE-18]=(long unsigned int)&ret_from_fork; //KERNEL_STACK_SIZE-18=1006
	tshijo->kernel_esp=(unsigned int)&(tuhijo->stack[KERNEL_STACK_SIZE-19]);
	

	tuhijo->task.estado = ST_READY;
	list_add_tail(&(tuhijo->task.list), &readyqueue);

	PID = tuhijo->task.PID;

 	return PID;
}

void sys_exit()
{  
	page_table_entry *pt_proceso = get_PT(current());
	for(int i=0;i< NUM_PAG_DATA;i++){
		free_frame(get_frame(pt_proceso,PAG_LOG_INIT_DATA+i));
		del_ss_pag(pt_proceso,PAG_LOG_INIT_DATA+i);
	}
	current()->PID = -1;
	list_add_tail(&(current()->list),&freequeue);
	sched_next_rr();
}

int sys_gettime(){
	return zeos_ticks;
}

	char myBuff[4096];

int sys_write(int fd, char * buffer, int size){

	int lol=check_fd(fd,ESCRIPTURA);
	if(lol!=0) return lol;	
	if(buffer == NULL) return -EFAULT;
	if(size<0) return -EINVAL;


	int escrits=0;

	while((size-escrits)>4096){
		copy_from_user(buffer+escrits,myBuff,4096);
		escrits = escrits + sys_write_console(myBuff,4096);
		
	}
	if((size-escrits)!=0){
		copy_from_user(buffer+escrits,myBuff,size-escrits);
		escrits=escrits+sys_write_console(myBuff,size-escrits);
	}
	return size;

}

int sys_getstats(int p, struct stats *st){

	if(!access_ok(VERIFY_WRITE,st,sizeof(struct stats))) {
		return -EFAULT;
	}
	if(p<0) return -EINVAL;
	for(int i = 0;i < NR_TASKS;i++){
		if(task[i].task.PID==p){	
			copy_to_user(&(task[i].task.estatistiques), st, sizeof(struct stats));
			return 0;
		}
	}
	return -ESRCH;
}