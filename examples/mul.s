	;; r0 = r0*r1
	mov r0,5
	mov r1,21
	mov r2,1
	mov r3,r0
	xor r0,r0
loop:	mov r4,r2
	and r4,r1
	br.z skip
	add r0,r3
skip:	asl r3,1
	asl r2,1
	br.cc loop
