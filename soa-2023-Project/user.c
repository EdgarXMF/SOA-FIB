#include <libc.h>

char buff[24]="fps:";
char buff2[24]= " ";
int pid;
int o=0;
char bufferrr[24];
int __attribute__ ((__section__(".text.main")))
  main(void)
{ 
  //init_starto();
  //write_fps();
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  
  /*read(bufferrr,2);
  write(1,bufferrr,3);
  read(buff,2);
  write(1,buff,3);*/
  
  //gotoxy(40,23);
  //set_color(6,5);
  //write(1,buff2,6);
  //getpid();
  //shmrm(0);
  //void *p = shmat(0,1245184);
  //shmdt(p);
  //int k=fork();
  //if(k==0){
    //exit();
    
    set_color(2,0);
    int nombre = 7;
    int k=fork();
    if(k==0){
     //pid=10;
      o = 4;
      //itoa(o,buff);
      //write(1,buff,6);
      
    }else{
      o=6;
      //itoa(o,buff2);
      //write(1,buff2,6);
    }
    
     //pid=5;
      //write(1,buff,6);
      //o = 4;*/
  //}
  while(1) { 
    //write(1,buff,6);
  //read(buff2,6);
  //write_fps();  
  }
  
}


void init_starto(){
  for(int i=0;i<80;i++){
    for(int j=0;j<25;j++){
      gotoxy(i,j);
      set_color(0,0);
      write(1,buff2,1);
    }
  }
  set_color(2,0);
  gotoxy(0,0);
  write(1,buff,4);
}



float frames = 0;

float fps(){
  return frames/(gettime()/18);
}

void write_fps(){
  gotoxy(4,0);
  itoa((int)fps(),bufferrr);
  write(1,bufferrr,strlen(bufferrr));
  gotoxy(25,22);
}
