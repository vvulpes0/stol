#include <iso646.h>  /* and, not, or, xor */
#include <stdio.h>   /* fclose, fopen, fprintf, freopen, printf */
#include <stdlib.h>  /* atexit */
#include <string.h>  /* memcpy */
#include <termios.h> /* tcgetattr, tcsetattr */
#include <unistd.h>  /* read */

#define MSB(x) ((x)&0x8000U)
#define LSB(x) ((x)&0x0001U)

struct termios ogtermios;

enum {
	SP = 15,
	PSP = 15,
	SSP = 16,
	SR = 17,
	PC = 18,
};

struct cpu {
	unsigned short *mem; /** ^memory */
	FILE *screen; /** ^tty screen mapped at address 0x100 */
	int keyboard; /** ^keyboard filedes, at address 0x101 */
	unsigned int regs[19]; /** ^r0-r15, psp, ssp, sr, pc */
};

static unsigned short bootrom[] = {
	0xc4f0U, 0x7000U, /* mov sp,0x7000 */
	0x190fU,         /* pspw sp */
	0xc4f0U, 0x8000U, /* mov sp,0x8000 */
	0x110fU,         /* push sp */
	0x0d0fU          /* rtp */
};

static int fill(unsigned short *, FILE *);
static int fromHex(char);

static int cond(struct cpu, int);
static unsigned short mread(struct cpu *, int);
static int mwrite(struct cpu *, int, unsigned short);
static int dmode(unsigned short);
static int smode(unsigned short);
static char const * rname(int);
static unsigned short rread(struct cpu *, int);
static void rwrite(struct cpu *, int, unsigned short);
static int step(struct cpu *);
static void setuptty(void);
static void restoretty(void);

int
main(int argc, char **argv)
{
	unsigned short mem[65536] = {};
	struct cpu cpu = {mem, stdout, fileno(stdin), {}};
	cpu.regs[SR] = 0x8000U;
	int ok = 1;
	memcpy(mem, bootrom, sizeof(bootrom));
	if (argc == 2) {
		FILE *from = fopen(argv[1], "r");
		if (not from) {
			fprintf(stderr, "could not open %s\n", argv[1]);
			return 1;
		}
		if (not fill(mem, from)) ok = 0;
		fclose(from);
	} else if (argc == 1) {
		if (not fill(mem, stdin)) ok = 0;
		fclose(stdin);
		char buf[L_ctermid] = {};
		ctermid(buf);
		freopen(buf, "r", stdin);
		if (not stdin) {
			fprintf(stderr, "warning: no keyboard\n");
			freopen("/dev/null", "r", stdin);
		}
	}
	if (not ok) return 1;
	setuptty();
	while (step(&cpu));
	int status = cpu.regs[SR];
	int C = (status>>3)&1;
	int V = (status>>2)&1;
	int N = (status>>1)&1;
	int Z = (status   )&1;
	fprintf(stderr,"-------- status report ---------\n");
	fprintf(stderr,"%9s%-4c%-4c%-4c%-4c\n","",
	        C?'C':'.', V?'V':'.', N?'N':'.', Z?'Z':'.');
	for (int i = 0; i < 18; i++) {
		int r = cpu.regs[i];
		if (r) {
			fprintf(stderr,"%-3s: %04x", rname(i), r);
			fprintf(stderr," = %5lu", (long)(r));
			fprintf(stderr," = %6ld",
			        (long)(r&0x7FFFU)-(MSB(r)?32768L:0L));
			if (32 <= r && r < 127) {
				fprintf(stderr," = '%c'", r);
			}
			fprintf(stderr, "\n");
		}
	}
	return 0;
}

/** Fill initial memory from a text file.
 *
 * File follows the 3.0 hex word-addressed format of Logisim Evolution.
 * First line must be "v3.0 hex words addressed".
 * Subsequent lines consist of a four-digit hexadecimal address,
 * a colon, then up to eight four-digit hexadecimal words.
 */
static int
fill(unsigned short *mem, FILE *from)
{
	static char const firstLine[] = "v3.0 hex words addressed";
	static unsigned int const rombase = 0x8000U;
	int i = 0;
	int c = 0;
	int address = 0;
	int value = 0;
	while ((c = fgetc(from))!='\n' and not feof(from)) {
		if (i >= sizeof(firstLine)) return 0;
		if (c == '\r') continue;
		if (c != firstLine[i++]) return 0;
	}
	if (feof(from)) return 0;
	/* henceforth, i is state */
	i = 0;
	while ((c = fgetc(from)) and not feof(from)) {
		switch (i) {
		case 0: /* reading address then colon */
			if (fromHex(c) >= 0) {
				address *= 16;
				address += fromHex(c);
			} else if (c == ':') {
				address += rombase;
				i = 1;
				value = 0;
			} else if (c != ' ') {
				fprintf(stderr, "unknown character"
				        " in address field: %c\n", c);
				return 0;
			}
			break;
		case 1: /* reading separating spaces */
			if (fromHex(c) >= 0) {
				value = fromHex(c);
				i = 2;
			} else if (c == '\n') {
				address = 0;
				i = 0;
			} else if (c != ' ' and c != '\r') {
				fprintf(stderr, "unknown character"
				        " between fields: %c\n", c);
			}
			break;
		case 2: /* reading a value */
			if (fromHex(c) >= 0) {
				value *= 16;
				value += fromHex(c);
			} else if (c == ' ' or c == '\n') {
				if (address >= 0 && address < 65536) {
					mem[address++] = value;
				} else {
					fprintf(stderr,
					        "address %d "
					        "out of range\n",
					        address);
					return 0;
				}
				if (c == '\n') {
					address = 0;
					i = 0;
				} else {
					i = 1;
				}
			} else {
				fprintf(stderr, "unknown character"
				        " in value field: %c\n", c);
				return 0;
			}
			break;
		default:
			fprintf(stderr, "unknown state: %d\n", i);
			return 0;
			break;
		}
	}
	return 1;
}

/** returns -1 if not a valid hex digit */
static int
fromHex(char c) {
	if (c >= '0' and c <= '9') {
		return c - '0';
	}
	if (c >= 'A' and c <= 'F') {
		return c - 'A' + 10;
	}
	if (c >= 'a' and c <= 'f') {
		return c - 'a' + 10;
	}
	return -1;
}



static unsigned short
mread(struct cpu *cpu, int address)
{
	if (!cpu) return 0;
	address &= 0xffffU;
	if (address == 0x100) {
		return 0;
	}
	if (address == 0x101) {
		unsigned char b = 0;
		unsigned short c = 0;
		if (read(cpu->keyboard, &b, 1) < 1) {
			c = 0xff80U;
		} else {
			c = b;
		}
		return c;
	}
	if (address >= 0x100 and address < 0x200) {
		return 0; /* other I/O ranges */
	}
	return cpu->mem[address];
}

static int
mwrite(struct cpu *cpu, int address, unsigned short value)
{
	/* fprintf(stderr, "%04x: %04x -> %04x\n", */
	/*         cpu->regs[PC], value, address); */
	if (!cpu) return 0;
	address &= 0xffffU;
	if (address >= 0x200 and address < 0x8000U) {
		cpu->mem[address] = value;
		return 1;
	}
	if (address == 0x100) {
		fprintf(cpu->screen, "%c", (unsigned char)(value&0x7f));
		fflush(cpu->screen);
		return 1;
	}
	if (address >= 0x100 and address < 0x200) {
		/* writes eaten by I/O range */
		return 1;
	}
	/* writes eaten by ROM */
	return 1;
}

static unsigned short
rread(struct cpu *cpu, int reg) {
	if (!cpu) return 0;
	if (reg < 15) return cpu->regs[reg];
	if (cpu->regs[SR] & 0x8000U) return cpu->regs[SSP];
	return cpu->regs[PSP];
}

static void
rwrite(struct cpu *cpu, int reg, unsigned short value) {
	if (not cpu) return;
	if (reg < 15) { cpu->regs[reg] = value; return; }
	if (cpu->regs[SR] & 0x8000U) { cpu->regs[SSP] = value; return; }
	cpu->regs[PSP] = value;
}

/* return 0 if HALT */
static int
step(struct cpu *cpu) {
	if (!cpu) return 0;
	unsigned short instr = mread(cpu, (cpu->regs[PC])++);
	unsigned short sex = 0;
	unsigned short dex = 0;
	int hadsex = 0;
	int dreg = (instr>>4) & 0xf;
	int sreg = instr & 0xf;
	unsigned short a = rread(cpu, dreg);
	unsigned short b = rread(cpu, sreg);
	unsigned short result = 0;
	unsigned short status = cpu->regs[SR];
	int swrite = 1;
	int dwrite = 1;
	int C = (status>>3)&1;
	int V = (status>>2)&1;
	int N = (status>>1)&1;
	int Z = (status   )&1;
	int halted = 0;
	int nrots = 0;
	int ogmsb = 0;
	/* set b for source */
	switch (smode(instr)) {
	case 0: /* immediate */
		b = instr & 0xf;
		if (b == 0) {
			b = sex = mread(cpu, (cpu->regs[PC])++);
			hadsex = 1;
		}
		break;
	case 2: /* register indirect */
		b = mread(cpu, rread(cpu,sreg));
		break;
	case 3: /* register+offset indirect */
		hadsex = 1;
		sex = mread(cpu, (cpu->regs[PC])++);
		b = mread(cpu, rread(cpu,sreg) + sex);
		break;
	default: /* register on 1 */
		break;
	}
	cpu->regs[PC] &= 0xffffU;
	/* set d for dest */
	switch (dmode(instr)) {
	case 2: /* register indirect */
		a = mread(cpu, rread(cpu,dreg));
		break;
	case 3: /* register+offset indirect */
		dex = mread(cpu, (cpu->regs[PC])++);
		a = mread(cpu, rread(cpu,dreg) + dex);
		break;
	default: /* register on 1 */
		break;
	}
	cpu->regs[PC] &= 0xffffU;
	/* do operation */
	switch ((instr >> 12) & 0xf) {
	case 0: /* branches */
		dwrite = 0;
		if (instr == 0 && sex == 0) halted = 1;
		if (not cond(*cpu, instr>>4)) break;
		switch ((instr>>10)&0x3) {
		case 0: /* BR */
			cpu->regs[PC] = cpu->regs[PC] + b - 1 - hadsex;
			break;
		case 1: /* BA */
			cpu->regs[PC] = b;
			break;
		case 2: /* CALL */
			rwrite(cpu, SP, rread(cpu, SP) - 1);
			mwrite(cpu, rread(cpu, SP), cpu->regs[PC]);
			cpu->regs[PC] = b;
			break;
		default: /* RET and RTP */
			if ((instr&0xf) == 15) {
				int n = status & 0x0f00U;
				n -= 0x0100U;
				n &= 0x0f00U;
				status = (status & 0xf0ffU) | n;
			}
			cpu->regs[PC] = mread(cpu, rread(cpu, SP));
			rwrite(cpu, SP, rread(cpu, SP) + 1);
			if ((instr&0xf) == 15) {
				if ((status & 0x0f00U) == 0x0f00U) {
					status &= 0x7fffU;
				}
			}
			break;
		}
		break;
	case 1: /* stack ops */
		switch ((instr>>8)&0xf) {
		case 1: /* PUSH */
			dwrite = 0;
			rwrite(cpu, SP, rread(cpu, SP) - 1);
			mwrite(cpu, rread(cpu, SP), b);
			break;
		case 5: /* POP */
			result = mread(cpu, rread(cpu, SP));
			rwrite(cpu, SP, rread(cpu, SP) + 1);
			break;
		case 9: /* PSPW */
			cpu->regs[PSP] = b;
			break;
		case 10: /* PSPR */
			result = cpu->regs[PSP];
			break;
		case 11: /* PSPRW */
			result = cpu->regs[PSP];
			cpu->regs[PSP] = b;
			break;
		default:
			fprintf(stderr, "bad stack op: %04x\n", instr);
			halted = 1;
			break;
		}
		break;
	case 2: /* status ops */
		switch ((instr>>10)&3) {
		case 0: /* MOVSR */
			status = b;
			break;
		case 1: /* ORSR */
			status |= b;
			break;
		case 2: /* ANDSR */
			status &= b;
			break;
		default: /* XORSR on 3 */
			status ^= b;
			break;
		}
		C = (status>>3)&1;
		V = (status>>2)&1;
		N = (status>>1)&1;
		Z = (status   )&1;
		break;
	case 3: /* shifts and rotations */
		switch ((instr>>9)&7) {
		case 0: /* NEG -- surprise, not a shift */
			result = (-b)&0xffffU;
			C = b != 0;
			V = b == 0x8000;
			break;
		case 1: /* LSR */
			result = a>>b;
			C = ((a<<(16-b))&0xffffU) != 0;
			V = 0;
			break;
		case 2: /* ASL */
			nrots = b>15 ? 16 : b;
			ogmsb = MSB(a);
			C = V = 0;
			for (int i = 0; i < nrots; i++) {
				if (MSB(a)) C = 1;
				a <<= 1;
				if (MSB(a) != ogmsb) V = 1;
			}
			result = a;
			break;
		case 3: /* ASR */
			nrots = b>15 ? 16 : b;
			ogmsb = a&0x8000U;
			C = V = 0;
			for (int i = 0; i < nrots; i++) {
				if (LSB(a)) C = 1;
				a >>= 1;
				a |= ogmsb;
			}
			result = a;
			break;
		case 4: /* ROL */
			nrots = b&0xf;
			C = V = 0;
			for (int i = 0; i < nrots; i++) {
				a = (a<<1) | ((a&0x8000)>>15);
			}
			result = a;
			break;
		case 5: /* ROR */
			nrots = b&0xf;
			C = V = 0;
			for (int i = 0; i < nrots; i++) {
				a = (a>>1) | ((a&1)<<15);
			}
			result = a;
			break;
		case 6: /* RLC */
			nrots = b%17;
			V = 0;
			for (int i = 0; i < nrots; i++) {
				b = !!C;
				C = !!MSB(a);
				a = (a<<1)|b;
			}
			result = a;
			break;
		default: /* RRC on 7 */
			nrots = b%17;
			V = 0;
			for (int i = 0; i < nrots; i++) {
				b = !!C;
				C = !!MSB(a);
				a = (a>>1)|(b<<15);
			}
			result = a;
			break;
		}
		N = MSB(result);
		Z = result == 0;
		break;
	case 4: /* SUB */
		result = (a - b)&0xffffU;
		C = (long)(a) - (long)(b) < 0;
		V = (MSB(a) != MSB(b)) and (MSB(a) != MSB(result));
		N = MSB(result);
		Z = result == 0;
		break;
	case 5: /* SUBB */
		result = (a - b - C)&0xffffU;
		C = (long)(a) - (long)(b) < 0;
		V = (MSB(a) != MSB(b)) and (MSB(a) != MSB(result));
		N = MSB(result);
		Z = result == 0;
		break;
	case 6: /* ADD */
		result = (a + b)&0xffffU;
		C = (long)(a) + (long)(b) > 65535;
		V = (MSB(a) == MSB(b)) and (MSB(a) != MSB(result));
		N = MSB(result);
		Z = result == 0;
		break;
	case 7: /* ADDC */
		result = (a + b + C)&0xffffU;
		C = (long)(a) + (long)(b) + (long)(C) > 65535;
		V = (MSB(a) == MSB(b)) and (MSB(a) != MSB(result));
		N = MSB(result);
		Z = result == 0;
		break;
	case 8: /* CMP */
		result = (a - b) & 0xffffU;
		C = (long)(a) - (long)(b) < 0;
		V = (MSB(a) != MSB(b)) and (MSB(a) != MSB(result));
		N = MSB(result);
		Z = result == 0;
		result = a;
		break;
	case 9: /* OR */
		result = (a|b)&0xffffU;
		N = MSB(result);
		Z = result == 0;
		break;
	case 10: /* AND */
		result = (a&b)&0xffffU;
		N = MSB(result);
		Z = result == 0;
		break;
	case 11: /* XOR */
		result = (a ^ b) & 0xffffU;
		N = MSB(result);
		Z = result == 0;
		break;
	case 12: /* MOV */
		result = b;
		break;
	case 13: /* TRAP */
		dwrite = 0;
		b = status & 0x0f00U;
		b += 1;
		b &= 0x0f00U;
		status = (status & 0xf0ffU) | 0x8000U | b;
		cpu->regs[SR] |= 0x8000U; /* enter supervisor mode! */
		rwrite(cpu, SP, rread(cpu, SP) - 1);
		mwrite(cpu, rread(cpu, SP), cpu->regs[PC]);
		break;
	case 14: /* reserved */
		dwrite = swrite = 0;
		fprintf(stderr, "reserved instruction: %04x\n", instr);
		halted = 1;
		break;
	case 15: /* reserved */
		dwrite = swrite = 0;
		fprintf(stderr, "reserved instruction: %04x\n", instr);
		halted = 1;
		break;
	default: /* how did we get here? */
		fprintf(stderr, "impossible instruction: %04x\n", instr);
		halted = 1;
		break;
	}
	if (swrite) {
		status &= 0xfff0U;
		status |= ((!!C)<<3)|((!!V)<<2)|((!!N)<<1)|(!!Z);
		if (MSB(cpu->regs[SR])) {
			cpu->regs[SR] = status;
		} else {
			cpu->regs[SR] &= 0xfff0U;
			cpu->regs[SR] |= status&0xf;
		}
	}
	if (dwrite) {
		result &= 0xffffU;
		switch (dmode(instr)) {
		case 1:
			rwrite(cpu, dreg, result);
			break;
		case 2:
			mwrite(cpu, rread(cpu,dreg), result);
			break;
		case 3:
			mwrite(cpu, rread(cpu,dreg)+dex, result);
			break;
		default:
			break;
		}
	}
	return not halted;
}

static int
dmode(unsigned short instr) {
	/* 0 = impossible */
	/* 1 = register  */
	/* 2 = register indirect */
	/* 3 = register+offset indirect */
	int op    = (instr >> 12) & 0xf;
	int mode  = (instr >> 10) & 0x3;
	if (op >= 4 and op <= 12) return mode;
	return 1;
}

static int
smode(unsigned short instr) {
	/* 0 = immediate: extension word if nonzero base */
	/* 1 = register  */
	/* 2 = register indirect */
	/* 3 = register+offset indirect */
	int op    = (instr >> 12) & 0xf;
	int mode  = (instr >>  8) & 0x3;
	int base  = instr & 0xf;
	if (op >= 13) return 0;
	if (op ==  3) return (mode & 1);
	if (op ==  1) return 1;
	return mode;
}

static int
cond(struct cpu cpu, int cc)
{
	unsigned short status = cpu.regs[SR];
	int C = (status>>3)&1;
	int V = (status>>2)&1;
	int N = (status>>1)&1;
	int Z = (status   )&1;
	cc &= 0xf;
	switch(cc) {
	case  0: return 1;
	case  1: return C;
	case  2: return V;
	case  3: return N;
	case  4: return Z;
	case  5: return C or Z;
	case  6: return N xor V;
	case  7: return (N xor V) or Z;
	default: return not (cond(cpu, cc & 7));
	}
}

static char const *
rname(int i)
{
	if (i ==  0) return "R0";
	if (i ==  1) return "R1";
	if (i ==  2) return "R2";
	if (i ==  3) return "R3";
	if (i ==  4) return "R4";
	if (i ==  5) return "R5";
	if (i ==  6) return "R6";
	if (i ==  7) return "R7";
	if (i ==  8) return "R8";
	if (i ==  9) return "R9";
	if (i == 10) return "R10";
	if (i == 11) return "R11";
	if (i == 12) return "R12";
	if (i == 13) return "R13";
	if (i == 14) return "R14";
	if (i == 15) return "PSP";
	if (i == 16) return "SSP";
	if (i == 17) return "SR";
	if (i == 18) return "PC";
	return "N/A";
}

static void
setuptty(void)
{
	tcgetattr(fileno(stdin), &ogtermios);
	atexit(restoretty);
	struct termios raw = ogtermios;
	raw.c_iflag &= ~(BRKINT | INPCK | IXON);
	raw.c_iflag |= (ICRNL | ISTRIP);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	tcsetattr(fileno(stdin), TCSAFLUSH, &raw);
}

static void
restoretty(void)
{
	tcsetattr(fileno(stdin), TCSAFLUSH, &ogtermios);
}
