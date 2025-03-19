	mov   r0,21         ; mul(5,21)
	push  r0
	mov   r0,5
	push  r0
	call  _mul
	add   sp,1
	pop   r0
	halt
_mul:	                    ; Function mul(x,y)
	mov   r0,1          ;   1 -> mask
	xor   r1,r1         ;   0 -> out
loop:	or    (sp+2),(sp+2) ;   While y != 0
	br.z  end
	mov   r2,r0         ;     If y & mask != 0 Then
	and   r2,(sp+2)
	br.z  skip
	add   r1,(sp+1)     ;       out + x -> out
	xor   (sp+2),r0     ;       y ^ mask -> y
skip:	                    ;     EndIf
	asl   r0,1          ;     mask * 2 -> mask
	add   (sp+1),(sp+1) ;     x * 2 -> x
	br    loop          ;   EndWhile
end:	mov   (sp+2),r1     ;   Return out
	ret                 ; EndFunction
