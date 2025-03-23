	mov r0,27183
	call _print
	halt

	;; _print : print 16-bit number as decimal
	;; inputs:
	;; r0 = n
_print:
	call _print_num
	mov r0,0x100
	mov (r0),'\n'
	ret
_print_num:
	mov r1,10
	call _udiv
	push r1
	or r0,r0
	br.z L000
	call _print_num
L000:	pop r1
	add r1,'0'
	mov r0,0x100
	mov (r0),r1
	ret

	;; _udiv : divide 16-bit unsigned numbers
	;; inputs:
	;; r0 = numerator
	;; r1 = denominator
	;; outputs:
	;; r0 = quotient
	;; r1 = remainder
	;; clobbers: r0-r3
_udiv:	push r4
	mov r4,16
	xor r2,r2 ; remainder
	xor r3,r3 ; quotient
L001:	asl r0,1
	rlc r2,1
	asl r3,1
	cmp r2,r1
	br.u< L002
	sub r2,r1
	add r3,1
L002:	sub r4,1
	br.nz L001
	mov r0,r3
	mov r1,r2
	pop r4
	ret
