	mov sp,0x7000
	pspw sp
	mov sp,0x8000
	mov r0,0x100    ; begin hardware initialization
	mov (r0),'\f'
loop:	mov r1,(r0+1)
	or r1,r1
	br.n loop       ; end hardware initialization
	push sp
	rtp
