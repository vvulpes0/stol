	mov r0,7            ; a(7)
	push r0             ; // provide parameter
	sub sp,2            ; // open room for links
	call _a             ; // call
	add sp,2            ; // skip links
	pop r0              ; // fetch return value
	halt                ; // end program

_a:	                    ; Procedure a(n)
	mov (sp+1),fp       ;   // set up dynamic link
	mov fp,sp
	mov r0,(fp+3)       ;   Return c(n)
	push r0             ;   // provide parameter
	sub sp,2            ;   // open room for links
	mov (sp+1),fp       ;   // static link [c is inside a]
	call c
	add sp,2            ;   // skip links
	pop r0              ;   // fetch return value
	mov (fp+3),r0       ;   // copy it into return slot
                            ; EndProcedure [inner definitions follow]
	mov fp,(fp+1)       ; // restore dynamic link
	ret

b:                          ; Procedure b(m) // inside a(n)
	mov (sp+1),fp       ;   // set up dynamic link
	mov fp,sp
	mov r0,0x100
	xor r1,r1           ;   Local i
	                    ;   0 -> i
	                    ;   // reading n from a into R2
	mov r2,(fp+2)       ;   // first get a's FP
	mov r2,(r2+3)       ;   // and fetch the first parameter, n
	sub r2,(fp+3)       ;   // register R2 holds n-m
top:	cmp r1,r2           ;   While i < n - m
	br.s>= topx
	mov (r0),' '        ;     "  " -> Terminal
	mov (r0),' '
	add r1,1            ;     i + 1 -> i
	br top              ;   EndWhile
topx:	xor r1,r1           ;   0 -> i
	mov r2,(fp+3)       ;   // register R2 holds m
bot:	cmp r1,r2           ;   While i < m
	br.s>= botx
	mov (r0),'['        ;     "[]" -> Terminal
	mov (r0),']'
	add r1,1            ;     i + 1 -> i
	br bot              ;   EndWhile
botx:	mov (r0),'\n'       ;   '\n' -> Terminal
	                    ;   Return m // already there
	                    ; EndProcedure
	mov fp,(fp+1)       ; // restore dynamic link
	ret

c:	                    ; Procedure c(n) // inside a(n)
	mov (sp+1),fp       ;   // set up dynamic link
	mov fp,sp
	xor r0,r0           ;   Local s
	push r0             ;   0 -> s
	cmp (fp+3),0        ;   If n >= 0 Then
	br.s<= exit
	mov r0,(fp+3)       ;     s + b(n) -> s
	push r0             ;     // provide parameter
	sub sp,2            ;     // open room for links
	mov (sp+1),(fp+2)   ;     // static link [b is inside a]
	call b
	add sp,2            ;     // skip links
	pop r1              ;     // fetch return value
	add (fp-1),r1
	mov r0,(fp+3)       ;     s + c(n - 2) -> s
	sub r0,2
	push r0             ;     // provide parameter
	sub sp,2            ;     // open room for links
	mov (sp+1),(fp+2)   ;     // static link [c is inside a]
	call c
	add sp,2            ;     // skip links
	pop r1              ;     // fetch return value
	add (fp-1),r1
	mov r0,(fp+3)       ;     s + b(n) -> s
	push r0             ;     // provide parameter
	sub sp,2            ;     // open room for links
	mov (sp+1),(fp+2)   ;     // static link [b is inside a]
	call b
	add sp,2            ;     // skip links
	pop r1              ;     // fetch return value
	add (fp-1),r1
exit:                       ;   EndIf
	pop r0
	mov (fp+3),r0       ;   Return s
	                    ; EndProcedure
	mov fp,(fp+1)       ; // restore dynamic link
	ret
