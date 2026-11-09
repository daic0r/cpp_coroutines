	default rel
	
	section .text
	global switch_context
	global co_finish_trampoline
	extern co_finish
	
switch_context:
	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	mov [rdi], rsp 		; save current rsp for later resumption
	mov rsp, rsi		; resume other coroutine
	
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	ret
	
co_finish_trampoline:

	call co_finish wrt ..plt
	ud2

section .note.GNU-stack noalloc noexec nowrite progbits
