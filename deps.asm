	default rel
	
	section .text
	global switch_context
	
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
	

	section .data
hello:	db "Hello, world!\n", 0
