/define terminal 0x100
	mov r0,terminal
	mov r3,msg
	mov r1,(r3)
	add r3,1
loop:	or r1,r1
	br.z end
	mov (r0),(r3)
	add r3,1
	sub r1,1
	br loop
end:	halt
msg:	dw 288
	dw "                                "
	dw "  Welcome to the STOL computer  "
	dw "                                "
	dw "  Check out the ./examples      "
	dw "  $ stolas <file.s >file.lcode  "
	dw "    right click ProgROM         "
	dw "    Load Image...               "
	dw "    select your file.lcode      "
	dw "                                "
