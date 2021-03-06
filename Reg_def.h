typedef unsigned long long REG;

struct IFID{
	unsigned int inst;
	int PC;
}IF_ID,IF_ID_old,IF_ID_bubble;


struct IDEX{
	int Rd,Rt;
	int PC;
	int Imm;
	REG Reg_Rs,Reg_Rt;

	char Ctrl_EX_ALUSrc;
	char Ctrl_EX_ALUOp;
	char Ctrl_EX_RegDst;

	char Ctrl_M_Branch;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}ID_EX,ID_EX_old, ID_EX_bubble;

struct EXMEM{
	int PC;
	int Reg_dst;
	REG ALU_out;
	int Zero;
	REG Reg_Rt;

	char Ctrl_M_Branch;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_MEM,EX_MEM_old,EX_MEM_bubble;

struct MEMWB{
	int PC;
	REG Mem_read;
	REG ALU_out;
	int Reg_dst;
		
	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}MEM_WB,MEM_WB_old,MEM_WB_bubble;