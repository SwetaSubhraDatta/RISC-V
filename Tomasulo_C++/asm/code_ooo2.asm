	XOR R0 R0 R0
	ADDI R1 R0 0xA000
	ADDI R2 R0 0x4
	LWS F2 0(R1)
LOOP:	LWS F3 0(R1)
	MULTS F1 F2 F3
	ADDS F1 F3 F3
	SUBI R2 R2 1
	ADDI R1 R1 0x4
	BNEZ R2 LOOP
	DIVS F4 F1 F1
	SUBS F1 F3 F1
	EOP