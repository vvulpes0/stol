	;; read and echo one line of text
	mov r0,0x100
loop:	xor r1,r1
	or r1,(r0+1)
	br.n loop
end:	mov (r0),r1
	cmp r1,'\n'
	br.!= loop
