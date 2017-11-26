#include <iostream>
#include <stdio.h>
#include <math.h>
// #include <io.h>
//#include <process.h>
#include <time.h>
#include <stdlib.h>
#include "Reg_def.h"


#define OP_R 51

#define F3_ADD 0
#define F7_ADD 0

#define F3_MUL 0
#define F7_MUL 1

#define F3_SUB 0x0
#define F7_SUB 0x20

#define F3_SLL 0x1
#define F7_SLL 0x00

#define F3_MULH 0x1
#define F7_MULH 0x01

#define F3_SLT 0x2
#define F7_SLT 0x00

#define F3_XOR 0x4
#define F7_XOR 0x00

#define F3_DIV 0x4
#define F7_DIV 0x01

#define F3_SRL 0x5
#define F7_SRL 0x00

#define F3_SRA 0x5
#define F7_SRA 0x20

#define F3_OR 0x6
#define F7_OR 0x01

#define F3_REM 0x6
#define F7_REM 0x01

#define F3_AND 0x7
#define F7_AND 0x00

#define OP_I_1 0x03

#define F3_LB 0x0
#define F3_LH 0x1
#define F3_LW 0x2
#define F3_LD 0x3



#define OP_I_2 0x13
#define F3_ADDI 0

#define F3_SLLI 0x1
#define F7_SLLI 0x00

#define F3_SLTI 0x2

#define F3_XORI 0x4

#define F3_SRLI 0x5
#define F7_SRLI 0x00

#define F3_SRAI 0x5
#define F7_SRAI 0x01

#define F3_ORI 0x6

#define F3_ANDI 0x7

#define OP_I_3 0x1B
#define F3_ADDIW 0x0
#define F3_SLLIW 0x1
#define F3_SRLIW 0x5


#define OP_I_4 0x67

#define OP_I_5 0x73



#define OP_S 0x23
#define F3_SB 0
#define F3_SH 0x1
#define F3_SW 0x2
#define F3_SD 0x3

#define OP_SB 0x63
#define F3_BEQ 0x0
#define F3_BNE 0x1
#define F3_BLT 0x4
#define F3_BGE 0x5

#define OP_U_1 0x17
#define OP_U_2 0x37

#define OP_JAL 0x6f


// #define OP_LW 3
// #define F3_LB 0

// #define OP_BEQ 99
// #define F3_BEQ 0



#define OP_RW 0x3b
#define F3_ADDW 0
#define F7_ADDW 0

#define F3_MULW 0
#define F7_MULW 1

#define F3_DIVW 4
#define F7_DIVW 1

#define F3_SUBW 0
#define F7_SUBW 0x20

// #define F7_ADDW 0


// #define OP_SCALL 115
// #define F3_SCALL 0
// #define F7_SCALL 0

#define MAX 100000000


/*
	 * EXTop
	 *  0  no signext
	 *  1  byte
	 *  2  half
	 *  3  word
	 * ALUop
	 * 0 NULL
	 * 1 +
	 * 2 *
	 * 3 -
	 * 4 <<
	 * 5 mulh
	 * 6 set less than
	 * 7 ^ xor
	 * 8 div  /
	 * 9 srl >>
	 * 10 rsa >>
	 * 11 or |
	 * 12 rem %
	 * 13 and &
	 */


#define ALUop_NULL 0
#define ALUop_Add 1
#define ALUop_Mul 2
#define ALUop_Sub 3
#define ALUop_Sll 4
#define ALUop_Mulh 5
#define ALUop_Slt 6
#define ALUop_Xor 7
#define ALUop_Div 8
#define ALUop_Srl 9
#define ALUop_Sra 10
#define ALUop_Or 11
#define ALUop_rem 12
#define ALUop_And 13
#define ALUop_BEQ 14
#define ALUop_BNE 15
#define ALUop_BLT 16
#define ALUop_BGE 17
#define ALUop_ADDIW 18
#define ALUop_AUIPC 19
#define ALUop_LUI 20
#define ALUop_JAL 21
#define ALUop_JALR 22
#define ALUop_ADDW 23
#define ALUop_Mulw 24
#define ALUop_DIVW 25
#define ALUop_SUBW 26



//主存
unsigned int memory[MAX]={0};
//寄存器堆
REG reg[32]={0};
//PC
int PC=0;


//各个指令解析段
unsigned int OP=0;
unsigned int fuc3=0,fuc7=0;
int shamt=0;
int rs=0,rt=0,rd=0;
unsigned int imm12=0;
unsigned int imm20=0;
unsigned int imm7=0;
unsigned int imm5=0;



//加载内存
void load_memory();

void simulate();

void IF();

void ID();

void EX();

void MEM();

void WB();


//符号扩展
int ext_signed(unsigned int src,int bit);

//获取指定位
// unsigned int getbit(int s,int e);

unsigned int getbit(unsigned inst,int s,int e)
{
	unsigned int mask = 0;

	for(int i = s;i <= e;i++){
		mask |= (1 << i);
	}

	return (inst & mask) >> s;
}
// use rs[0:7]
// to set mem[s, s+7]
unsigned int setbit(unsigned int mem, int s,
					unsigned int  rs){
	unsigned int res = mem;

	for (int i = s;i < s + 8;i++) {
		res &= ~(1 << i);
		res |=  ((1 << (i - s)) & rs ) << s;
	}
	return res;
}

int ext_signed(unsigned int src,int bit)
{

    return (((int)src) << bit)>>bit;
}

