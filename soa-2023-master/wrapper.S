#include <asm.h>
#include <segment.h>

ENTRY(write)
	pushl %ebp
	mov %esp,%ebp
	pushl %edx
	pushl %ecx
	movl $4, %eax
	mov 8(%ebp), %ebx
	mov 12(%ebp), %ecx
	mov 16(%ebp), %edx
	
	
	push $retuser
	pushl %ebp
	mov %esp,%ebp

	sysenter
	
retuser:

	
	popl %ebp
	addl $4, %esp #Al carrer l'adreça retuser
	popl %ecx
	popl %edx

	cmpl $0, %eax
	jge fin
	
	negl %eax
	movl %eax,errno

	movl $-1, %eax

fin:
	movl %ebp,%esp
	popl %ebp

	ret


ENTRY (gettime)
	
	pushl %ebp
	mov %esp,%ebp
	pushl %edx
	pushl %ecx
	movl $10, %eax
	
	
	
	push $reto
	pushl %ebp
	mov %esp,%ebp

	sysenter
	
reto:

	
	popl %ebp
	addl $4, %esp #Al carrer l'adreça reto
	popl %ecx
	popl %edx

	cmpl $0, %eax
	jge fin2
	negl %eax
	movl %eax,errno

	movl $-1, %eax

fin2:
	movl %ebp,%esp
	popl %ebp

	ret

ENTRY (getpid)

	pushl %ebp
	mov %esp,%ebp
	pushl %edx
	pushl %ecx
	movl $20, %eax
	
	
	
	push $devuelve
	pushl %ebp
	mov %esp,%ebp

	sysenter
	
devuelve:

	
	popl %ebp
	addl $4, %esp #Al carrer l'adreça devuelve
	popl %ecx
	popl %edx

	cmpl $0, %eax
	jge fin3
	negl %eax
	movl %eax,errno

	movl $-1, %eax

fin3:
	movl %ebp,%esp
	popl %ebp

	ret

ENTRY (fork)

	pushl %ebp
	mov %esp,%ebp
	pushl %edx
	pushl %ecx
	movl $2, %eax
	
	
	
	push $retorna
	pushl %ebp
	mov %esp,%ebp

	sysenter
	
retorna:

	
	popl %ebp
	addl $4, %esp #Al carrer l'adreça retorna
	popl %ecx
	popl %edx

	cmpl $0, %eax
	jge fin4
	negl %eax
	movl %eax,errno

	movl $-1, %eax

fin4:
	movl %ebp,%esp
	popl %ebp

	ret

ENTRY (exit)

	pushl %ebp
	mov %esp,%ebp
	pushl %edx
	pushl %ecx
	movl $1, %eax
	
	
	
	push $retorna2
	pushl %ebp
	mov %esp,%ebp

	sysenter
	
retorna2:

	
	popl %ebp
	addl $4, %esp #Al carrer l'adreça retorna
	popl %ecx
	popl %edx

	cmpl $0, %eax
	jge fin5
	negl %eax
	movl %eax,errno

	movl $-1, %eax

fin5:
	movl %ebp,%esp
	popl %ebp

	ret



ENTRY (getstats)

	pushl %ebp
	mov %esp,%ebp
	pushl %ebx
	pushl %edx
	pushl %ecx
	movl $35, %eax
	mov 8(%ebp), %ebx
	mov 12(%ebp), %ecx
	
	
	push $retorna3
	pushl %ebp
	mov %esp,%ebp

	sysenter
	
retorna3:

	
	popl %ebp
	addl $4, %esp #Al carrer l'adreça retuser
	popl %ecx
	popl %edx
	popl %ebx

	cmpl $0, %eax
	jge fin6
	
	negl %eax
	movl %eax,errno

	movl $-1, %eax

fin6:
	movl %ebp,%esp
	popl %ebp

	ret