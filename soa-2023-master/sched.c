/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>


union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));


struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
  //return (struct task_struct*)((int)l&0xFFFFF000);
}

void cont_inner_ts ();

int cuantos_quedan = 0;

extern struct list_head blocked;

struct list_head freequeue;
struct list_head readyqueue;
struct task_struct * idle_task;
struct task_struct * init_task;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{;
	printk(" estoy dentro del idle \n");
	}
}


void init_stats(struct stats *estad){
	estad->user_ticks = 0;
	estad->system_ticks = 0;
	estad->blocked_ticks = 0;
	estad->ready_ticks = 0;
	estad->elapsed_total_ticks = get_ticks();
	estad->total_trans = 0;
	estad->remaining_ticks = get_ticks();
}


void update_users(void){

	current()->estatistiques.user_ticks += (get_ticks() - current()->estatistiques.elapsed_total_ticks);
	current()->estatistiques.elapsed_total_ticks = get_ticks();
	
}

void update_system(void){
	
	current()->estatistiques.system_ticks += (get_ticks() - current()->estatistiques.elapsed_total_ticks);
	current()->estatistiques.elapsed_total_ticks = get_ticks();
	
}

void init_idle (void)
{
	struct list_head *lh0 = list_first(&freequeue);
	list_del(lh0);
	struct task_struct *ts0 = list_head_to_task_struct(lh0);
	ts0->PID = 0;
	ts0->quantum = 15;
	//stats
	init_stats(&ts0->estatistiques);
	allocate_DIR(ts0);
	((union task_union*)ts0) -> stack[1023] = (unsigned long)&cpu_idle;
	((union task_union*)ts0) -> stack[1022] = 0;
	ts0->kernel_esp = &((union task_union*)ts0) -> stack[1022];
	idle_task = ts0;

}

void inner_task_switch(union task_union *new_task){
	tss.esp0 = &(((union task_union*)new_task)->stack[1024]);
	wrMSR(0x175,0,tss.esp0);
	
	set_cr3(get_DIR(&(new_task)->task));
	
	cont_inner_ts(&current()->kernel_esp, new_task->task.kernel_esp);	
}

void task_switch (union task_union *new);


void init_task1(void)
{
	struct list_head *lh1 = list_first(&freequeue);
	list_del(lh1);
	
	struct task_struct *ts1 = list_head_to_task_struct(lh1);
	ts1->quantum=15;
	ts1->estado=ST_RUN;
	ts1->PID = 1;
	cuantos_quedan=ts1->quantum;//15
	//stats
	init_stats(&ts1->estatistiques);
	allocate_DIR(ts1);
	set_user_pages(ts1);
	
	tss.esp0 = &(((union task_union*)ts1)->stack[1024]);//KERNEL_ESP((union task_union*)ts1);
	wrMSR(0x175,0,tss.esp0);
	set_cr3(ts1->dir_pages_baseAddr);
	init_task = ts1;
}

void sched(){

	update_sched_data_rr();
	
	if(needs_sched_rr()){
		
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}

void init_sched()
{
	
	INIT_LIST_HEAD(&freequeue);
	for(int i=0;i<NR_TASKS;i++) list_add_tail(&(task[i].task.list),&freequeue);
	
	INIT_LIST_HEAD(&readyqueue);

}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

int get_quantum(struct task_struct *t){
	return t->quantum;
}

void set_quantum(struct task_struct *t, int new_quantum){
	t->quantum = new_quantum;
}

void update_sched_data_rr(void){
	cuantos_quedan=cuantos_quedan-1;
	current()->estatistiques.remaining_ticks = cuantos_quedan;
}

int needs_sched_rr(void){
	//devuelve 1 si cal cambiar de proceso 
	//devuelve 0 otherwise
	if(cuantos_quedan==0) {
		
		if (!list_empty(&readyqueue)){
				
				return 1; 

		} 
		else {
			cuantos_quedan = get_quantum(current());
			return 0;
		}
	}
	else return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue){
	//actualiza estado proceso
	//lo borra de la cola actgual y lo inserta en la nueva
	//si el proc actual esta running la cola es null
	
	struct list_head *list = &t->list;
	if(list->prev && list->next){
		list_del(list);
	}
	if(dst_queue!=NULL) {
		list_add_tail(list,dst_queue); 
		t->estatistiques.system_ticks += get_ticks()-t->estatistiques.elapsed_total_ticks;
		t->estado=ST_READY;
	}
}

void sched_next_rr(void){
	//pilla el siguiente proc a ejec y lo quita dela readyqueue para invocar el cambio de contexto
	//siempre la ejecutamos dsps del proceso actual (dsps de llamar al update_process, justo arriba)
	struct list_head *lista_cabeza;
	struct task_struct *tarea_estructura;

	if (!list_empty(&readyqueue)){
		lista_cabeza= list_first(&readyqueue);
		list_del(lista_cabeza); //Aixo no cal ja que en el update ja es treu de la ready el proces current. Tenint en compte que de ready nomes pasa a run i viceversa
		tarea_estructura = list_head_to_task_struct(lista_cabeza);
	}
	else{
		tarea_estructura = idle_task;	
	} 
	
	tarea_estructura -> estado = ST_RUN;

	cuantos_quedan = get_quantum(tarea_estructura);

	//update estadisticas
	current()->estatistiques.system_ticks += get_ticks() - current()->estatistiques.elapsed_total_ticks;
	current()->estatistiques.elapsed_total_ticks = get_ticks();

	tarea_estructura->estatistiques.ready_ticks += get_ticks() - tarea_estructura->estatistiques.elapsed_total_ticks;
	tarea_estructura->estatistiques.elapsed_total_ticks = get_ticks();
	tarea_estructura->estatistiques.total_trans += 1;
	task_switch((union task_union*) tarea_estructura);
}
	
