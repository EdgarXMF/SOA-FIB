#include <asm.h>
#include <segment.h>
#include <errno.h>

ENTRY(task_switch)
	pushl %ebp
	movl %esp, %ebp
	
	pushl %esi
	pushl %edi
	pushl %ebx
	
	pushl 8(%ebp)
	
	call inner_task_switch
	
	addl $4, %esp //fora el union
	
	popl %ebx
	popl %edi
	popl %esi
	
	
	popl %ebp
	ret

ENTRY (cont_inner_ts)
	
	movl 4(%esp), %eax
	movl %ebp, (%eax)
	movl 8(%esp), %esp
	popl %ebp
	ret

ENTRY(get_ebp)
	movl %ebp, %eax
	ret
	



