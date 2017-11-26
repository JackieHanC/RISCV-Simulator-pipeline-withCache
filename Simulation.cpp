#include "Simulation.h"
#include <string.h>
#include <iostream>
#include "stdio.h"
#include "cache.h"
#include "memory.h"

using namespace std;

extern void read_elf(char * elf_path, char * res_path);
extern unsigned int cadr;
extern unsigned int csize;
extern unsigned int vadr;
extern unsigned long long gp;
extern unsigned int madr;
extern unsigned int endPC;
extern unsigned int entry;
extern unsigned int result_addr;
extern unsigned int a_addr;
extern unsigned int b_addr;
extern unsigned int c_addr;
extern unsigned int sum_addr;
extern unsigned int temp_addr;

extern FILE *file;



//指令运行数
long long inst_num = 0;
long long cycle_num = 0;
long long delta_cycle = 0;

int data_hazard = 0;
int control_hazard = 0;

#define max(a, b)  a > b ? a : b

//系统调用退出指示
int exit_flag=0;

int p_flag = 0;

int IF_state = 1, ID_state = 1, EX_state = 1, MEM_state = 1, WB_state = 1;

Memory m;
Cache l1;
Cache l2;
Cache llc;

//加载代码段
//初始化PC
void load_memory()
{
	fseek(file,cadr,SEEK_SET);
	fread(&memory[vadr>>2],1,csize,file);

	vadr=vadr>>2;
	csize=csize>>2;
	fclose(file);
}
void init_cache() {
	m.init((char*)memory);
    l1.SetLower(&l2);
    l2.SetLower(&llc);
    llc.SetLower(&m);

    StorageStats s;
    s.access_time = 0;
    m.SetStats(s);
    l1.SetStats(s);
    l2.SetStats(s);
    llc.SetStats(s);

    StorageLatency ll1;
    ll1.bus_latency = 0;
    // 1 cpu cycle 
    ll1.hit_latency = 1;
    l1.SetLatency(ll1);

    StorageLatency ll2;
    ll2.bus_latency = 0;
    ll2.hit_latency = 8;
    l2.SetLatency(ll2);

    StorageLatency lllc;
    lllc.bus_latency = 0;
    lllc.hit_latency = 20;
    llc.SetLatency(lllc);

    StorageLatency ml;
    ml.bus_latency = 0;
    ml.hit_latency = 100;
    m.SetLatency(ml);

    CacheConfig_ l1config;
    l1config.size = 32 KB;
    l1config.associativity = 8;
    l1config.set_num = 64;
    l1config.write_through = 0;
    l1config.write_allocate = 1;
    l1.SetConfig(l1config);


    CacheConfig_ l2config;
    l2config.size = 256 KB;
    l2config.associativity = 8;
    l2config.set_num = 512;
    l2config.write_through = 0;
    l2config.write_allocate = 1;
    l2.SetConfig(l2config);

    CacheConfig_ llcConfig;
    llcConfig.size = 8 MB;
    llcConfig.associativity = 8;
    llcConfig.set_num = 16384;
    llcConfig.write_through = 0;
    llcConfig.write_allocate = 1;
    llc.SetConfig(llcConfig);



}
int main(int argc, char * argv[])
{
	if(argc < 4){
		printf("error! not enough args\n");
		return 1;
	}

	//解析elf文件
	read_elf(argv[1], argv[2]);

	if(strcmp(argv[3],"-a") == 0) {
		p_flag = 0;
	} else if (strcmp(argv[3],"-p") == 0) {
		p_flag = 1;
	} else {
		printf("invalid instruction\n");
		return 1;
	}
	
	//加载内存
	load_memory();

	init_cache();
	//设置入口地址
	PC=madr>>2;
	
	//设置全局数据段地址寄存器
	reg[3]=gp;

	printf("gp is %x\n",gp);
	
	reg[2]=MAX/2;//栈基址 （sp寄存器）

	simulate();

	cout <<"simulate over!"<<endl;


	return 0;
}

void simulate()
{
	//结束PC的设置
	//printf("madr is 0x%x\n",madr );
	//printf("end pc is 0x%x\n",endPC);
	int end=(int)endPC/4-1;

	IF_ID=IF_ID_bubble;
	ID_EX=ID_EX_bubble;
	EX_MEM=EX_MEM_bubble;
	MEM_WB=MEM_WB_bubble;
	while(PC!=end)
	{
		IF_state = 1, ID_state = 1, EX_state = 1, MEM_state = 1, WB_state = 1;
		//运行
		WB();
		MEM();
		EX();
		ID();
		IF();
		
		//更新中间寄存器
		IF_ID=IF_ID_old;
		ID_EX=ID_EX_old;
		EX_MEM=EX_MEM_old;
		MEM_WB=MEM_WB_old;


		cycle_num += delta_cycle;
		delta_cycle = 0;
		//printf("\tdelta_cycle is %d\n",delta_cycle );
		

		if(p_flag == 1) {
			char c[10];
			while(cin >> c){
				if (strcmp("p", c) == 0)
					break;
				else if (strcmp("c", c) == 0){
					exit_flag = 1;
					break;
				}
				else if (strcmp("reg", c) == 0){
					for (int i = 0;i < 32;i++){
						printf("reg[%d] = 0x%x\n",
							i, reg[i] );
					}
				}
				else if (strcmp("mem", c) == 0){
					printf("Please enter address, 0x");
					long long adr;
					cin >>hex>> adr;
					printf("0x%08x ",(adr >> 2)<<2 );
					printf("%02x ", getbit(memory[adr >> 2],0,8));
					printf("%02x ", getbit(memory[adr >> 2],8,14));
					printf("%02x ", getbit(memory[adr >> 2],15,23));
					printf("%02x\n", getbit(memory[adr >> 2],24,31));

				}
    			else {
    				printf("invalid instruction, please retry.\n");
    			}
				
			}
		}
        if(exit_flag==1)
            break;

        reg[0]=0;//一直为零

        printf("\n");


        // unsigned char* addr = (unsigned char*)memory;
        // printf("result array \n");
        // for (int i = 0;i < 6;++i){
        // 	printf("%d ", *(int*)(addr+result_addr+i*4));
        // }
        // printf("\n");
        // printf("sum is %d\n",*(int*)(addr + sum_addr));
        // printf("\t a value is %d\n",*(int*)(addr + a_addr));
        // printf("\t b value is %d\n",*(int*)(addr + b_addr));
        // printf("\t c value is %d\n",*(int*)(addr + c_addr));
        // // printf("*******Result is  %d******\n", *(int*)(addr+result_addr));
        // printf("***temp is %d*****\n",*(int*)(addr+temp_addr) );

        printf("\n");


	}
	//
	WB();
	MEM();
	EX();
	ID();
	cycle_num += delta_cycle;
	delta_cycle = 0;
		

	IF_ID=IF_ID_old;
	ID_EX=ID_EX_old;
	EX_MEM=EX_MEM_old;
	MEM_WB=MEM_WB_old;
	//
	WB();
	MEM();
	EX();
	cycle_num += delta_cycle;
	delta_cycle = 0;
		
	IF_ID=IF_ID_old;
	ID_EX=ID_EX_old;
	EX_MEM=EX_MEM_old;
	MEM_WB=MEM_WB_old;
	//
	WB();
	MEM();
	cycle_num += delta_cycle;
	delta_cycle = 0;
		

	IF_ID=IF_ID_old;
	ID_EX=ID_EX_old;
	EX_MEM=EX_MEM_old;
	MEM_WB=MEM_WB_old;

	//
	WB();
	cycle_num += delta_cycle;
	delta_cycle = 0;
		
	IF_ID=IF_ID_old;
	ID_EX=ID_EX_old;
	EX_MEM=EX_MEM_old;
	MEM_WB=MEM_WB_old;

	l1.flush(&m);

	unsigned char* addr = (unsigned char*)memory;
    // printf("result array \n");
    // for (int i = 0;i < 6;++i){
    // 	printf("%d ", *(int*)(addr+result_addr+i*4));
    // }
    // printf("\n");
    // printf("sum is %d\n",*(int*)(addr + sum_addr));
    printf("\t a value is %d\n",*(int*)(addr + a_addr));
    printf("\t b value is %d\n",*(int*)(addr + b_addr));
    printf("\t c value is %d\n",*(int*)(addr + c_addr));
    // printf("*******Result is  %d******\n", *(int*)(addr+result_addr));
    printf("***temp is %d*****\n",*(int*)(addr+temp_addr) );

    printf("\n");

	printf("Data hazard %d\n",data_hazard);
	printf("control hazard %d\n",control_hazard);

	printf("\tCycle Number is %d\n", cycle_num);
	printf("\tInstruction Number is %d\n",inst_num);
	printf("\tCPI is %f\n",(float)cycle_num/inst_num);
}


//取指令
void IF()
{
	if(IF_state == 0)
		return;
	//write IF_ID_old
	//IF_ID_old.inst=memory[PC];
	IF_ID_old.inst=memory[PC];
	printf("PC is 0x%05x  inst 0x%08x\n",PC<<2,IF_ID_old.inst);
	//IDnextPC=PC+1;
	// IF_ID_old.PC=PC;
	PC = PC + 1;
	IF_ID_old.PC=PC;

	inst_num++;

	delta_cycle = max(delta_cycle, 1);
}

//译码
void ID()
{
	if(ID_state == 0)
		return;
	//Read IF_ID
	unsigned int inst=IF_ID.inst;
	// printf("decode inst is 0x%x\n",inst );

	int EXTop=0;
	unsigned int EXTsrc=0;

	int RegDst;
	char ALUop = ALUop_NULL,ALUSrc = 0;
	char Branch = 0,MemRead = 0,MemWrite = 0;
	char RegWrite = 0,MemtoReg = 0;

	unsigned int rd = getbit(inst,7,11);
	unsigned int fuc3 = getbit(inst,12,14);
	unsigned int imm;
	unsigned int fuc7;
	unsigned int rs1 = -1,rs2 = -1;
	//....
	unsigned int OP = getbit(inst, 0, 6);

	/*
	 *	EXTop           for imm sign ext

	 *  RegDst 			
	 * 	   1 rd 0 rt
	 *	ALUop  			0 for plus, 1 for mul 
	 *	ALUSrc 			0 for reg ,1 for imm
	 *	Branch 			0 for not branch, 1 for branch
	 *	MemRead			0 for not read, 1 for read
	 *	MemWrite		0 for not write, 1 for write
	 *	RegWrite		same as above
	 *	MemtoReg
	 */	

	/*
	 * EXTop at end
	 *  0  no signext
	 *  1  ext
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

	/* MemRead
	 * 0 do not read
	 * 1 read byte
	 * 2 read half
	 * 3 read word
	 * 4 read double word
	 */

	/* MemWrite
	 * 0 do not write
	 * 1 write byte
	 * 2 write half
	 * 3 write word
	 * 4 write double word
	 */

	/* Branchto
	 * 0 not branch
	 * 1 equal branch
	 * 2 not equal branch
	 * 3 less equal branch
	 * 4 not less than branch
	 * 5 jal
	 * 6 jalr
	 */

	/* MemtoReg
	 * 0 NULL
	 * 1 ALUout
	 * 2 Mem_read
	 */
	RegDst = 1;
	// printf("OP is 0x%x\n",OP);
	// printf("fuc3 is 0x%x\n",fuc3 );
	// printf("rd is %d\n",rd);
	if(OP == OP_R) {
		fuc7 = getbit(inst, 25, 31);
		rs1 = getbit(inst, 15, 19);
		rs2 = getbit(inst, 20, 24);

		EXTop = 0;



		// when OP = OP_R, their dest is rd 
		// there is no imm
		ALUSrc = 0;
		// there is no branch
		Branch = 0;
		// there is no mem read and write
		MemRead = 0;
		MemWrite = 0;
		// 
		MemtoReg = 1;

		RegWrite = 1;


		// add rd, rs1, rs2
		if(fuc3 == F3_ADD && fuc7 == F7_ADD)
		{
			ALUop = ALUop_Add;// +
		}
		// mul rd, rs1, rs2
		else if (fuc3 == F3_MUL && fuc7 == F7_MUL)
		{
		    ALUop = ALUop_Mul; 
		}
		// sub rd, rs1, rs2
		// rd = rs1 - rs2
		else if (fuc3 == F3_SUB && fuc7 == F7_SUB){
			ALUop = ALUop_Sub;
		}
		//sll rd, rs1, rs2
		//rd = rs1 << rs2
		else if (fuc3 == F3_SLL && fuc7 == F7_SLL){
			ALUop = ALUop_Sll;
		}
		// mulh
		else if (fuc3 == F3_MULH && fuc7 == F7_MULH){
			ALUop = ALUop_Mulh;
		}
		//slt rd, rs1, rs2
		//set less than
		// rd (rs1 < rs2)?1:0
		else if (fuc3 == F3_SLT && fuc7 == F7_SLT){
			ALUop = ALUop_Slt;
		}
		//xor rd, rs1,rs2
		// rd = rs1 ^ rs2
		else if (fuc3 == F3_XOR && fuc7 == F7_XOR){
			ALUop = ALUop_Xor;
		}
		// div rd, rs1, rs2
		// rd = rs1/ rs2
		else if (fuc3 == F3_DIV && fuc7 == F7_DIV){
			ALUop = ALUop_Div;
		}
		// srl rd, rs1 ,rs2
		// >>
		else if (fuc3 == F3_SRL && fuc7 == F7_SRL){
			ALUop = ALUop_Srl;
		}
		// >> 
		else if (fuc3 == F3_SRA && fuc7 == F7_SRA){
			ALUop = ALUop_Sra;
		}
		// or rd, rs1, rs2
		else if (fuc3 == F3_OR && fuc7 == F7_OR){
			ALUop = ALUop_Or;
		}
		// rd = rs1 % rs2
		else if (fuc3 == F3_REM && fuc7 == F7_REM) {
			ALUop = ALUop_rem;
		}
		// rd = rs1 & rs2
		else if (fuc3 == F3_AND && fuc7 == F7_AND) {
			ALUop = ALUop_And;
		}

	}
	else if (OP == OP_I_1) {
		rs1 = getbit(inst, 15, 19);
		imm = getbit(inst, 20, 31);

		EXTsrc = imm;
		// dest is rd
		// have offset 
		ALUSrc = 1;
		// no branch
		Branch = 0;

		MemWrite = 0;

		MemtoReg = 2;
		// do not use alu
		ALUop = ALUop_Add;

		RegWrite = 1;
		
		EXTop = 20;
		

		EXTsrc = imm;
		if (fuc3 == F3_LB) {
			MemRead = 1;

		}
		else if (fuc3 == F3_LH){
			MemRead = 2;
		}
		else if (fuc3 == F3_LW) {
			MemRead = 3;
		}
		else if (fuc3 == F3_LD) {
			MemRead = 4;
		}
	}
	else if (OP == OP_I_2){
		rs1 = getbit(inst, 15, 19);
		imm = getbit(inst, 20, 31);

		EXTsrc = imm;
    	// no ext
    
    	// to rd
    	//alu src  from imm
    	ALUSrc = 1;
    	Branch = 0;
    	MemRead = 0;
    	MemWrite = 0;
    	RegWrite = 1;
    	MemtoReg = 1;
    	

        if(fuc3 == F3_ADDI)
        {
            ALUop = ALUop_Add;
            EXTop = 20;
     
        }
        else if (fuc3 == F3_SLLI){
            ALUop = ALUop_Sll;
            EXTsrc = getbit(imm, 0 , 5);
            EXTop = 0;
        } 
        else if (fuc3 == F3_SLTI){
        	ALUop = ALUop_Slt;
        	EXTop = 20;
        }
        else if (fuc3 == F3_XORI){
        	ALUop = ALUop_Xor;
        	EXTop = 20;
        }
        else if (fuc3 == F3_SRLI){
        	ALUop = ALUop_Srl;
        	EXTsrc = getbit(imm, 0 , 5);
        	EXTop = 0;
        }
        else if (fuc3 == F3_SRAI){
        	ALUop = ALUop_Sra;
        	EXTsrc = getbit(imm, 0 , 5);
        	EXTop = 0;
        }
        else if (fuc3 == F3_ORI) {
        	ALUop = ALUop_Or;
        	EXTop = 20;
        }
        else if (fuc3 == F3_ANDI) {
        	ALUop = ALUop_And;
        	EXTop = 20;
        }
    }
    else if (OP == OP_I_3) {
    	rs1 = getbit(inst, 15, 19);
    	imm = getbit(inst, 20, 31);

    	EXTsrc = imm;
		ALUSrc = 1;
    	EXTop = 20;
    	if (fuc3 == F3_ADDIW) {
    		// word ext

    		printf("In addiw\n");

    		ALUop = ALUop_ADDIW;
    		
    		Branch = 0;
    		MemRead = 0;
    		MemWrite = 0;
    		MemtoReg = 1;
    		RegWrite = 1;
    	}
    	else if (fuc3 == F3_SLLIW) {
    		printf("This instruction is SLLIW\n");
    		//shamt
    		imm = getbit(inst,20,24);
    		ALUop = ALUop_Sll;
    		Branch = 0;
    		MemRead = 0;
    		MemWrite = 0;
    		MemtoReg = 1;
    		RegWrite = 1;
    	}
    	else if (fuc3 == F3_SRLIW) {
    		printf("This instruction is SRLIW\n");
    		imm = getbit(inst,20,24);
    		ALUop = ALUop_Srl;
    		Branch = 0;
    		MemRead = 0;
    		MemWrite = 0;
    		MemtoReg = 1;
    		RegWrite = 1;
    	}else {
    		printf("invalid fuc3 of OP_I_3\n");
    	}
    }
    // jalr
    else if (OP == OP_I_4) {

    	printf("it's jalr\n");
    	rs1 = getbit(inst, 15, 19);

    	imm = getbit(inst, 20, 31);

    	EXTsrc = imm;

    	Branch = 6;

    	EXTop = 20;

    	// use ALUout
    	MemtoReg = 1;

    	MemRead = 0;

    	RegWrite = 1;

    	ALUSrc = 1;

    	ALUop = ALUop_JALR;

    	MemWrite = 0;
    }
    else if(OP == OP_S)
    {
    	rs1 = getbit(inst, 15, 19);
    	rs2 = getbit(inst, 20, 24);

    	imm = getbit(inst, 7, 11) | (getbit(inst, 25, 31) << 5);

		EXTsrc = imm;

    	EXTop = 20;
    	ALUop = ALUop_Add;
    	// no reg dest 
    	// RegDst = 
    	// ALUSrc =
    	Branch = 0;
    	RegWrite = 0;
    	MemRead = 0;
    	// offset 
    	ALUSrc = 1;
    	//do not write reg
    	MemtoReg = 0;
        if(fuc3==F3_SB)
        {
            MemWrite = 1;
        }
        else if (fuc3 == F3_SH)
        {
            MemWrite = 2;
        }
        else if (fuc3 == F3_SW) {
            MemWrite = 3;
        }
        else if (fuc3 == F3_SD) {
        	MemWrite = 4;
        	printf("rs2 is %d\n", rs2);
        }
    }
    else if(OP==OP_SB)
    {
    	rs1 = getbit(inst, 15, 19);
    	rs2 = getbit(inst, 20, 24);

    	imm = (getbit(inst, 8, 11) << 1 ) |
    		(getbit(inst, 25, 30) << 5) |
    		(getbit(inst, 31, 31) << 12) |
    		(getbit(inst, 7, 7) << 11);
    	EXTsrc = imm;
    	ALUSrc = 0;

    	EXTop = 19;
    	//no reg dest

    	// do not write reg
    	MemtoReg = 0;

        if(fuc3==F3_BEQ)
        {
			Branch = 1;
			ALUop = ALUop_BEQ;
        }
        else if (fuc3 == F3_BNE)
        {
        	Branch = 2;
        	ALUop = ALUop_BNE;
        }

        else if (fuc3 == F3_BLT) {
        	Branch = 3;
        	ALUop = ALUop_BLT;
        }

        else if (fuc3 == F3_BGE) {
        	Branch = 4;
        	ALUop = ALUop_BGE;
        }
    }
    else if(OP==OP_U_1)
    {
        imm = getbit(inst, 12, 31) << 12;
        EXTsrc = imm;

        EXTop = 0;

        ALUop = ALUop_AUIPC;

        RegDst = 1;

        MemRead = 0;
        MemWrite = 0;
        RegWrite = 1;
        ALUSrc = 1;
        // use ALUout
        MemtoReg = 1;
    }
    else if (OP == OP_U_2){
    	imm = getbit(inst, 12, 31) << 12;
    	EXTsrc = imm;
    	ALUSrc = 1;

    	RegDst = 1;
    	EXTop = 0;
    	ALUop = ALUop_LUI;

    	RegWrite = 1;
    	// MemRead = 0;
    	// MemWrite = 0;
    	// use ALUout
    	MemtoReg = 1;
    }
    else if(OP==OP_JAL)
    {
    	printf(" JAL instruction\n");
        imm = (getbit(inst, 12, 19) << 12) |
        	(getbit(inst, 20,20) << 11) |
        	(getbit(inst, 21, 30) << 1) |
        	(getbit(inst, 31, 31) << 20);
        EXTsrc = imm;
        RegWrite = 1;
        EXTop = 11;
        ALUop = ALUop_JAL;
        ALUSrc = 1;
        // use ALUout
        MemtoReg = 1;

        Branch = 5;
    }
    else if (OP==OP_RW) {
    	rs1 = getbit(inst, 15, 19);
    	rs2 = getbit(inst, 20, 24);
    	fuc7 = getbit(inst, 25, 31);
    	EXTop = 0;

    	ALUSrc = 0;
		// there is no branch
		Branch = 0;
		// there is no mem read and write
		MemRead = 0;
		MemWrite = 0;
		// 
		MemtoReg = 1;

		RegWrite = 1;
    	if (fuc3 == F3_ADDW && fuc7 == F7_ADDW){
    		ALUop = ALUop_ADDW;
    	}else if (fuc3 == F3_MULW && fuc7 == F7_MULW){
    		ALUop = ALUop_Mulw;
    	}else if (fuc3 == F3_DIVW && fuc7 == F7_DIVW){
    		ALUop = ALUop_DIVW;
    	}else if (fuc3 == F3_SUBW && fuc7 == F7_SUBW){
    		ALUop = ALUop_SUBW;
    	}
    	else{
    		printf("Unknown fuc3 and fuc7 of OP_RW\n");
    	}
    }
    else
    {
		printf("Unkonwn instruction\n");
    }

    if((ID_EX.Ctrl_WB_RegWrite == 1&&ID_EX.Rd==rs1) || 
    	(EX_MEM.Ctrl_WB_RegWrite == 1&&EX_MEM.Reg_dst==rs1) || 
    	(MEM_WB.Ctrl_WB_RegWrite == 1&&MEM_WB.Reg_dst==rs1) ||
    	(ID_EX.Ctrl_WB_RegWrite == 1&&ID_EX.Rd==rs2) || 
    	(EX_MEM.Ctrl_WB_RegWrite == 1&&EX_MEM.Reg_dst==rs2) || 
    	(MEM_WB.Ctrl_WB_RegWrite == 1&&MEM_WB.Reg_dst==rs2))
    {
    	ID_EX_old = ID_EX_bubble;
    	IF_state = 0;
    	data_hazard++;
    	return ;
    }


	// //write ID_EX_old
	// ID_EX_old.Rd=rd;
	// ID_EX_old.Rt=rt;
	// ID_EX_old.Imm=ext_signed(EXTsrc,EXTop);

	// printf("rs1 is %d\nrs2 is %d\n",rs1, rs2);

	// ID_EX_old.Reg_Rs = reg[rs1];
	// ID_EX_old.Reg_Rt = reg[rs2];
	// ID_EX_old.Ctrl_EX_ALUSrc = ALUSrc;
	// ID_EX_old.Ctrl_EX_RegDst = RegDst;


	// //ID_EX_old.Ctrl_M_MemReadSize = 

	// ID_EX_old.Ctrl_M_Branch = Branch;
	// ID_EX_old.Ctrl_M_MemWrite = MemWrite;
	// ID_EX_old.Ctrl_M_MemRead = MemRead;
	// ID_EX_old.Ctrl_WB_RegWrite = RegWrite;
	// ID_EX_old.Ctrl_WB_MemtoReg = MemtoReg;

	
	// //...

	// ID_EX_old.Ctrl_EX_ALUOp=ALUop;
	
	//write ID_EX_old
	ID_EX_old.Rd=rd;
	ID_EX_old.Rt=rt;
	ID_EX_old.Imm=ext_signed(EXTsrc,EXTop);

	printf("Imm is %d\n",ID_EX.Imm);

	printf("rs1 is %d\nrs2 is %d\n",rs1, rs2);
	if(rs1 == -1)
		ID_EX_old.Reg_Rs = 0;
	else
		ID_EX_old.Reg_Rs = reg[rs1];
	if(rs2 == -1)
		ID_EX_old.Reg_Rt = 0;
	else
		ID_EX_old.Reg_Rt = reg[rs2];
	printf("Reg_Rt is %d\n", ID_EX.Reg_Rt);
	ID_EX_old.Ctrl_EX_ALUSrc = ALUSrc;
	ID_EX_old.Ctrl_EX_RegDst = RegDst;


	ID_EX_old.Ctrl_M_Branch = Branch;
	ID_EX_old.Ctrl_M_MemWrite = MemWrite;
	ID_EX_old.Ctrl_M_MemRead = MemRead;
	ID_EX_old.Ctrl_WB_RegWrite = RegWrite;
	ID_EX_old.Ctrl_WB_MemtoReg = MemtoReg;

	ID_EX_old.PC = IF_ID.PC;

	
	//...

	ID_EX_old.Ctrl_EX_ALUOp=ALUop;
	delta_cycle = max(delta_cycle, 1);

}

//执行
void EX()
{
	if(EX_state == 0)
		return;
	//read ID_EX
	int temp_PC = ID_EX.PC;
	char RegDst = ID_EX.Ctrl_EX_RegDst;
	char ALUOp = ID_EX.Ctrl_EX_ALUOp;
	char ALUSrc = ID_EX.Ctrl_EX_ALUSrc;
	char Branch = ID_EX.Ctrl_M_Branch;
	int imm = ID_EX.Imm;
	long long input1;
	long long input2;


	//choose ALU input number
	//...
	input1 = ID_EX.Reg_Rs;
	if (ALUSrc == 0){
		input2 = ID_EX.Reg_Rt;
	} else if (ALUSrc == 1){
		input2 = ID_EX.Imm;
	} else {
		printf("invalid ALUSrc number\n");
	}



	//alu calculate
	int Zero;
	long long ALUout;
	switch(ALUOp)
	{

	case ALUop_NULL:break;
	case ALUop_Add:
		ALUout = input1 + input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Mul:
		ALUout = input1*input2;

		//cycle_num += 2;
		delta_cycle = max(delta_cycle, 2);

		break;
	case ALUop_Sub:
		ALUout = input1 - input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Sll:
		ALUout = input1 << input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Mulh:{
		// to do
		REG A_High = ((input1 & 0xFFFFFFFF00000000) >> 32) & 0xFFFFFFFF;

        REG A_Low = input1 & 0x00000000FFFFFFFF;

        REG B_High = ((input2 & 0xFFFFFFFF00000000) >> 32) & 0xFFFFFFFF;

   		REG B_Low = input2 & 0x00000000FFFFFFFF;

   		REG AHBH = A_High*B_High;

   		REG AHBL = ((A_High*B_Low) >> 32) & 0xFFFFFFFF;

   		REG ALBH = ((A_Low*B_High) >> 32) & 0xFFFFFFFF;

   		ALUout = AHBH + AHBL + ALBH;

   		//cycle_num++;
   		delta_cycle = max(delta_cycle, 1);

		break;
	}
	case ALUop_Slt:
		ALUout = (input1 < input2) ? 1:0;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Xor:
		ALUout = input1 ^ input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Div:
		ALUout = input1 / input2;
		
		//cycle_num += 40;
		delta_cycle = max(delta_cycle, 40);

		break;
	case ALUop_Srl:
		ALUout = (input1) >> input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Sra:
		ALUout = input1 >> input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Or:
		ALUout = input1 | input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_rem:
		
		ALUout = input1 % input2;
		
		//cycle_num += 40;
		delta_cycle = max(delta_cycle, 40);

		break;
	case ALUop_And:
		ALUout = input1 & input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_BEQ:
		ALUout = (input1 == input2) ? 1 : 0;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_BNE:
		printf("input1 is %d\n",input1);
		printf("input2 is %d\n",input2);
		ALUout = (input1 != input2) ? 1 : 0;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_BLT:
		ALUout = (input1 < input2) ? 1 : 0;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_BGE:
		ALUout = (input1 >= input2) ? 1 : 0;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_ADDIW:
		ALUout = ext_signed(
			getbit(input1 + input2, 0, 31),32);
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_AUIPC:
		ALUout = temp_PC - 1 + (input2>>2);
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		printf("temp_PC is 0x%x\n", temp_PC);
		printf("input2 is %d\n", input2);
		printf("ALUout is 0x%x\n", ALUout);
		break;
	case ALUop_LUI:
		ALUout = input2;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_JAL:
		ALUout = temp_PC;
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_JALR:
		ALUout = temp_PC;
		printf("jalr ALUout is 0x%x\n",ALUout);
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;

	case ALUop_ADDW:
		ALUout = (long long)((int)input1 + (int)input2);
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_Mulw:
		ALUout = (long long)((int)input1*(int)input2);
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	case ALUop_DIVW:
		ALUout = (long long)((int)input1/(int)input2);
		//cycle_num += 40;
		delta_cycle = max(delta_cycle, 40);
		break;
	case ALUop_SUBW:
		ALUout = (long long)((int)input1 - (int)input2);
		//cycle_num++;
		delta_cycle = max(delta_cycle, 1);
		break;
	default:
		printf("Unkonwn ALUop\n");
		break;
	}

	//Branch PC calulate
	//...
	int bubble = 0;
	switch (Branch)
	{
	case 0:
		break;
	case 1://beq
		if(ALUout==1){
			printf("beq success\n");
			temp_PC += (imm/4 -1);
			PC = temp_PC;
			bubble=1;
			control_hazard++;
		}
		break;
	case 2://bne
		if(ALUout==1){
			printf("bne success\n");
			temp_PC += (imm/4 -1);
			PC = temp_PC;
			bubble=1;
			inst_num--;
			control_hazard++;
		}
		break;
	case 3://blt
		if(ALUout==1){
			printf("blt success\n");
			temp_PC += (imm/4 -1);
			PC = temp_PC;
			bubble=1;
			inst_num--;
			control_hazard++;
		}
		break;
	case 4://bge
		if(ALUout==1){
			printf("bge success\n");
			temp_PC += (imm/4 -1);
			PC = temp_PC;
			bubble=1;
			inst_num--;
			control_hazard++;
		}
		break;
	case 5://jal
		temp_PC += (imm/4 -1);
		PC = temp_PC;
		bubble=1;
		inst_num--;
		control_hazard++;
		printf("  JAL address 0x%x\n",temp_PC<<2);
		break;
	case 6://jalr
		temp_PC =  input1 + (input2>>2);
		PC = temp_PC;
		bubble=1;
		inst_num--;
		control_hazard++;
		printf("rs is 0x%x\n",input1);
		printf("imm is 0x%d\n", input2);
		printf("jalr pc is 0x%x\n", temp_PC<<2);
		break;
	default:
		printf("Unkonwn Branch value\n");
		printf("value is %d\n",Branch );
		break;
	}



	//choose reg dst address
	int Reg_Dst;
	if(RegDst)
	{
		Reg_Dst = ID_EX.Rd;
	}
	else
	{
		Reg_Dst = ID_EX.Rt;
	}

	//write EX_MEM_old
	// EX_MEM_old.ALU_out=ALUout;
	// EX_MEM_old.PC=temp_PC;
 //    EX_MEM_old.Reg_dst = Reg_Dst;
 //    EX_MEM_old.Reg_Rt = ID_EX.Reg_Rt;
 //    EX_MEM_old.Ctrl_M_Branch = ID_EX.Ctrl_M_Branch;
 //    EX_MEM_old.Ctrl_M_MemWrite = ID_EX.Ctrl_M_MemWrite;
 //    EX_MEM_old.Ctrl_M_MemRead = ID_EX.Ctrl_M_MemRead;
 //    EX_MEM_old.Ctrl_WB_RegWrite = ID_EX.Ctrl_WB_RegWrite;
 //    EX_MEM_old.Ctrl_WB_MemtoReg = ID_EX.Ctrl_WB_MemtoReg;
	EX_MEM_old.ALU_out=ALUout;
	
	//EX_MEM_old.PC=temp_PC;
    
    EX_MEM_old.Reg_dst = Reg_Dst;
    EX_MEM_old.Reg_Rt = ID_EX.Reg_Rt;

    printf("EX_MEM.Reg_Rt is %d\n", EX_MEM.Reg_Rt);

    EX_MEM_old.Ctrl_M_Branch = ID_EX.Ctrl_M_Branch;
    EX_MEM_old.Ctrl_M_MemWrite = ID_EX.Ctrl_M_MemWrite;
    EX_MEM_old.Ctrl_M_MemRead = ID_EX.Ctrl_M_MemRead;
    EX_MEM_old.Ctrl_WB_RegWrite = ID_EX.Ctrl_WB_RegWrite;
    EX_MEM_old.Ctrl_WB_MemtoReg = ID_EX.Ctrl_WB_MemtoReg;

    if(bubble==1) {
    	ID_EX=ID_EX_bubble;
    }
}

//访问存储器
void MEM()
{
	if(MEM_state == 0)
		return;
	//read EX_MEM
	char MemRead  = EX_MEM.Ctrl_M_MemRead;
	long long ReadOut;
	long long ALUout = EX_MEM.ALU_out;

	//unsigned char* pos;
	int hit;
	int time;
	switch (MemRead){
	case 0:
		break;
	case 1: {// read byte

		char c;
		l1.HandleRequest(ALUout,1,1,(char*)&c, hit, time);
		delta_cycle = max(time, 1);
		ReadOut = (long long)c;
		

		break;

	}
	case 2:{// load half

		short c;
		l1.HandleRequest(ALUout, 2, 1, (char*)&c, hit, time);
		delta_cycle = max(time, 1);
		ReadOut = (long long)c;

		break;
	}
	case 3:{//load word

		int c;
		l1.HandleRequest(ALUout, 4, 1, (char*)&c, hit, time);
		delta_cycle = max(time, 1);
		ReadOut = (long long)c;
		printf("Load Word %ld\n",ReadOut);
		break;
	
	}
	case 4:{//load double word
		
		// mem1 = memory[ALUout >> 2];
		// // mem2 = memory[(ALUout>>2) + 1];
		// printf("ALUout is %d\n",ALUout);
		// printf("mem_loc is %d\n",ALUout>>2 );
		// printf("memory loc is %x\n",(unsigned char*)memory+ALUout);
		// ReadOut = *(long long*)(  (unsigned char *)memory + ALUout);
		
		l1.HandleRequest(ALUout, 8, 1, (char*)&ReadOut, hit, time);
		delta_cycle = max(time, 1);
		// l1.HandleRequest(ALUout+8, 8,1, (char*)&ReadOut + 8, hit, time);
		// delta_cycle = max(time, 1);
		break;


	}
	default:break;
	}


	//complete Branch instruction PC change

	//read / write memory

	char MemWrite = EX_MEM.Ctrl_M_MemWrite;
	REG rs2 = EX_MEM.Reg_Rt;

	
	switch(MemWrite)
	{
	case 0:
		break;
	case 1:{// write byte
		// pos = (unsigned char*)memory + ALUout;
		// *pos = (char)rs2;
		// printf("store %d\n", rs2);
		char c = rs2;
		l1.HandleRequest(ALUout, 1, 0, &c, hit, time);
		delta_cycle = max(time, 1);
		
		break;
	}
	case 2:{//write half
		// pos = (unsigned char*)memory + ALUout;
		// *(short*)pos = (short)rs2;
		// printf("store %d\n", rs2);
		short c = rs2;
		l1.HandleRequest(ALUout, 2, 0, (char*)&c, hit, time);
		delta_cycle = max(time, 1);
		
		break;
	}
	case 3:{
		// pos = (unsigned char *)memory + ALUout;
		// *(int*)pos = (int)rs2;
		// printf("store %d at 0x%x\n", rs2,ALUout);
		// printf("In memory: %d\n",*(int*)pos);
		int c = rs2;
		l1.HandleRequest(ALUout, 4, 0, (char*)&c, hit, time);
		delta_cycle = max(time, 1);
		printf("Store Word %d\n",c);

		break;
	}
	case 4:{
		// printf("sd\n");
		// printf("store %d\n", rs2);
		// fflush(stdout);
		// pos = ((unsigned char *)memory) + ALUout;
		// *(long long*)pos = rs2;
		long long c = rs2;
		l1.HandleRequest(ALUout, 8, 0, (char*)&c, hit, time);
		delta_cycle = max(time, 1);

		break;

		
	} default:break;
	}
	


	// //write MEM_WB_old
	// MEM_WB_old.PC = EX_MEM.PC;
	// MEM_WB_old.Mem_read = ReadOut;
	// MEM_WB_old.Reg_dst = EX_MEM.Reg_dst;
	// MEM_WB_old.ALU_out = ALUout;
	// MEM_WB_old.Ctrl_WB_RegWrite = EX_MEM.Ctrl_WB_RegWrite;
	// MEM_WB_old.Ctrl_WB_MemtoReg = EX_MEM.Ctrl_WB_MemtoReg;
	//write MEM_WB_old

	// MEM_WB_old.PC = EX_MEM.PC;
	MEM_WB_old.Mem_read = ReadOut;
	MEM_WB_old.Reg_dst = EX_MEM.Reg_dst;
	MEM_WB_old.ALU_out = ALUout;
	MEM_WB_old.Ctrl_WB_RegWrite = EX_MEM.Ctrl_WB_RegWrite;
	MEM_WB_old.Ctrl_WB_MemtoReg = EX_MEM.Ctrl_WB_MemtoReg;

	
	
}


//写回
void WB()
{
	if(WB_state == 0) {
		printf("WB_state = 0\n");
		return;
	}
	//read MEM_WB
	char RegWrite, MemtoReg;
	REG MemRead, ALUout;
	int Reg_dst;

	// PC = MEM_WB.PC;
	//printf("write back PC is 0x%x\n", PC);
	ALUout = MEM_WB.ALU_out;
	RegWrite = MEM_WB.Ctrl_WB_RegWrite;
	MemtoReg = MEM_WB.Ctrl_WB_MemtoReg;
	MemRead = MEM_WB.Mem_read;
	Reg_dst = MEM_WB.Reg_dst;

	long long res;

	if (RegWrite == 0) {
		printf("No RegWrite\n");
		return;
	}

	switch (MemtoReg) {
	case 0:break;
	case 1:res = ALUout;break;
	case 2:res = MemRead;break;
	default:break;
	}
	//write reg
	printf("write reg[%d]  %d\n",Reg_dst, res);
	reg[Reg_dst] = res;

	//cycle_num++;
	delta_cycle = max(delta_cycle, 1);




}