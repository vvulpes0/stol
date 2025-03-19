	mov   r0,5   ; mul(5,21)
	mov   r1,21
	call  _mul
	halt
_mul:	              ; Function mul(x,y)
	push  r4
	mov   r2,1    ;   1 -> mask
	xor   r3,r3   ;   0 -> out
loop:	or    r1,r1   ;   While y != 0
	br.z  end
	mov   r4,r2   ;     If y & mask != 0 Then
	and   r4,r1
	br.z  skip
	add   r3,r0   ;       out + x -> out
	xor   r1,r2   ;       y ^ mask -> y
skip:	              ;     EndIf
	asl   r2,1    ;     mask * 2 -> mask
	asl   r0,1    ;     x * 2 -> x
	br    loop    ;   EndWhile
end:	mov   r0,r3   ;   Return out
	pop   r4
	ret           ; EndFunction
