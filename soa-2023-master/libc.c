/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>
#include<errno.h>

#define NUM_COLUMNS 80
#define NUM_ROWS    25
Byte x, y=19;

int errno;





void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror() {

  
  if (errno == EACCES) write(1,"Permission denied\n",18);
  else if (errno == EBADF)  write(1,"Bad file number\n",18);
  else if (errno == EFAULT) write(1,"Bad address\n",13);
  else if (errno == EINVAL) write(1,"Invalid argument\n",18);
  else if (errno == ENOSYS) write(1,"Function not implemented\n",26);
  if (errno == ENOMEM) write(1,"Out of memory\n",14);
  else{
      write(1,"Unknown error\n",18);
  }
}

