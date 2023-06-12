#include <asm.h>
#include <segment.h>

ENTRY(wrMSR)
		push %ebp
      	mov %esp, %ebp
      	movl 8(%ebp), %ecx
      	movl 12(%ebp), %edx
      	movl 16(%ebp), %eax
      	wrmsr
      	movl %ebp,%esp
      	pop %ebp
      	ret
