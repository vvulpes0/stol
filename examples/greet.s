	call _greet         ; greet()
	halt
_greet:                     ; Procedure greet()
	mov r0,0x100
	mov r3,sp           ;   SP -> R3 // backup
	mov sp,str	    ;   "hello" -> SP
	pop r1              ;   (SP+) -> R1 // length
loop:	or r1,r1            ;   While R1 != 0
	br.z end
	pop r2              ;     (SP+) -> (Terminal)
	mov (r0),r2
	sub r1,1            ;     R1 - 1 -> R1
	br loop             ;   EndWhile
end:	mov sp,r3           ;   R3 -> SP // restore
	ret                 ; EndProcedure
str:	dw 6,"hello\n"      ; // length-prefixed string
