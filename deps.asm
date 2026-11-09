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

	sub     rsp, 8
	stmxcsr [rsp]           ; MXCSR: 32 bits at offset 0
	fnstcw  [rsp + 4]       ; x87 control word: 16 bits at offset 4

	mov [rdi], rsp 		; save current rsp for later resumption
	mov rsp, rsi		; resume other coroutine

	ldmxcsr [rsp]
	fldcw   [rsp + 4]
	add     rsp, 8	
	
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
