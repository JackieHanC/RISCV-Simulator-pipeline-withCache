#include"Read_Elf.h"
#include <string.h>

FILE *elf=NULL;
Elf64_Ehdr elf64_hdr;

//Program headers
unsigned int padr=0;
unsigned int psize=0;
unsigned int pnum=0;

//Section Headers
unsigned int sadr=0;
unsigned int ssize=0;
unsigned int snum=0;

//Symbol table
unsigned int symnum=0;
unsigned int symadr=0;
unsigned int symsize=24;

//用于指示 包含节名称的字符串是第几个节（从零开始计数）
unsigned int _index=0;

//字符串表在文件中地址，其内容包括.symtab和.debug节中的符号表
unsigned long stradr=0;


bool open_file(char * elf_path)
{
	if ((file = fopen(elf_path,"rb")) == NULL){
		return false;
	}
	return true;
}

void read_elf(char * elf_path, char * res_path)
{
	if(!open_file(elf_path)) {
		printf(" file did not exist.\n");
		return ;
	}

	elf = fopen(res_path,"w");

	fprintf(elf,"ELF Header:\n");

	read_Elf_header();
	fprintf(elf,"\n\nSection Headers:\n");
	read_elf_sections();
	fprintf(elf,"\n\nProgram Headers:\n");
	read_Phdr();
	fprintf(elf,"\n\nSymbol table:\n");
	read_symtable();
	fclose(elf);
}

void read_Elf_header()
{
	//file should be relocated
	fread(&elf64_hdr,1,sizeof(elf64_hdr),file);

	fprintf(elf, " magic number:  ");

	for (int i = 0;i < 16;++i) {
		fprintf(elf, " %02x",elf64_hdr.e_ident[i]);
	}

	fprintf(elf, "\n");
		
	// fprintf(elf," magic number:  %16x", elf64_hdr.e_ident);

	if (elf64_hdr.e_ident[4] == 1) {
		fprintf(elf," Class:  ELFCLASS32\n");		
	} else if (elf64_hdr.e_ident[4] == 2) {
		fprintf(elf," Class:  ELFCLASS64\n");
	}


	
	if (elf64_hdr.e_ident[5] == 1) {
		fprintf(elf," Data:  little-endian\n");
	} else if (elf64_hdr.e_ident[5] == 2) {
		fprintf(elf," Data:  big-endian\n");
	}
	
		
	fprintf(elf," Version:   %u\n", elf64_hdr.e_version);


	fprintf(elf," OS/ABI:	 System V ABI\n");
	
	fprintf(elf," ABI Version: 0\n");

	unsigned short type = *(unsigned short *)(&elf64_hdr.e_type);

	if (type == 1) {
		fprintf(elf, " Type:   REL\n");
	} else if(type == 2) {
		fprintf(elf, " Type:   EXEC\n");
	} else if(type == 3) {
		fprintf(elf, " Type:   DYN\n");
	} else {
		fprintf(elf, " Type:   UNKONWN\n");
	}

	unsigned short machine = *(unsigned short *)(&elf64_hdr.e_machine);

	if (machine == 0xf3) {
		fprintf(elf," Machine:   RISC-V\n");
	} else if(machine == 0x3) {
		fprintf(elf," Machine:   INTEL x86\n");
	} else {
		fprintf(elf," Machine:   UNKONWN\n");
	}

	// printf("machine \n");

	fprintf(elf," Version:  \n");

	entry = (unsigned int)(*(unsigned long*)(&elf64_hdr.e_entry));


	fprintf(elf," Entry point address:  0x%x\n", elf64_hdr.e_entry);


	padr = (unsigned int)(*(unsigned long*)(&elf64_hdr.e_phoff));

	fprintf(elf," Start of program headers:   %ld bytes into  file\n", elf64_hdr.e_phoff);

	sadr = (unsigned int)(*(unsigned long*)(&elf64_hdr.e_shoff));

	fprintf(elf," Start of section headers:   %ld bytes into  file\n", elf64_hdr.e_shoff);


	fprintf(elf," Flags:  0x%x\n", elf64_hdr.e_flags);


	fprintf(elf," Size of this header:   %u Bytes\n", elf64_hdr.e_ehsize);

	psize = (unsigned int)(*((unsigned short*)(&elf64_hdr.e_phentsize)));

	fprintf(elf," Size of program headers:   %u Bytes\n", elf64_hdr.e_phentsize);

	pnum = (unsigned int)(*((unsigned short*)(&elf64_hdr.e_phnum)));
	
	fprintf(elf," Number of program headers:   %u \n", elf64_hdr.e_phnum);

	ssize = (unsigned int)(*((unsigned short*)(&elf64_hdr.e_shentsize)));

	fprintf(elf," Size of section headers:   %u Bytes\n", elf64_hdr.e_shentsize);

	snum = (unsigned int)(*((unsigned short*)(&elf64_hdr.e_shnum)));

	fprintf(elf," Number of section headers:  %u\n",elf64_hdr.e_shnum);

	_index = (unsigned int)(*((unsigned short*)(&elf64_hdr.e_shstrndx))); 
	
	fprintf(elf," Section header string table index:   %u\n",elf64_hdr.e_shstrndx);

	// pnum = 0;

	// snum = 0;


}

void read_elf_sections()
{

	Elf64_Shdr elf64_shdr;

	if (fseek(file, sadr, SEEK_SET) == -1) {
		printf("fseek error\n");
		return;
	}

	char name[20] = {};

	unsigned long shstradr;

	for(int c=0;c<snum;c++)
	{
		fread(&elf64_shdr,1,sizeof(elf64_shdr),file);
		if(c == _index) {
			shstradr=*(unsigned long*)&elf64_shdr.sh_addr
				+*(unsigned long*) &elf64_shdr.sh_offset;
		}
	}

	printf("get str adr\n");

	int cnt = 0;

	// fseek(file, sadr, SEEK_SET);

	for(int c=0;c<snum;c++)
	{
		fprintf(elf," [%3d]\n",c);
		
		//file should be relocated
		fseek(file, sadr + c * sizeof(elf64_shdr), SEEK_SET);

		fread(&elf64_shdr,1,sizeof(elf64_shdr),file);

		unsigned long str_adr = shstradr + *(unsigned int*)&elf64_shdr.sh_name;

		fseek(file, str_adr, SEEK_SET);

		fread(&name[cnt], 1, 1, file);

		// printf("get here\n");
		while(name[cnt] != '\0') {

			// printf("%c\n",name[cnt] );
			cnt++;
			fread(&name[cnt], 1, 1, file);
		}
		cnt = 0;

		// printf("Get name\n");

		if (strcmp(name, ".symtab") == 0) {


			symadr = *(unsigned long *)&elf64_shdr.sh_addr +
				*(unsigned long*)&elf64_shdr.sh_offset;
			symnum = (*(unsigned long*)&elf64_shdr.sh_size)/symsize;

			printf("get symadr \n");
		}

		if (strcmp(name, ".strtab") == 0) {
			stradr = *(unsigned long *)&elf64_shdr.sh_addr +
				*(unsigned long*)&elf64_shdr.sh_offset;
		}


		// if (strcmp(name, ".data") == 0) {
		// 	gp = *(unsigned long *)&elf64_phdr.sh_addr;
		// }

		// if (strcmp(name, ".text") == 0) {
		// 	endPC = *(unsigned long *)&elf64_shdr.sh_addr + *(unsigned long *)&elf64_shdr.sh_size;
		// }

		fprintf(elf," Name: %10s",name);

		fprintf(elf," Type: %x", elf64_shdr.sh_type);

		fprintf(elf," Address:  %x", elf64_shdr.sh_addr);

		fprintf(elf," Offest:  %d\n",elf64_shdr.sh_offset);

		fprintf(elf," Size:  %d", elf64_shdr.sh_size);

		fprintf(elf," Entsize:  %d",elf64_shdr.sh_entsize);

		fprintf(elf," Flags:   %x",elf64_shdr.sh_flags);
		
		fprintf(elf," Link:  %x", elf64_shdr.sh_link);

		fprintf(elf," Info:  %x", elf64_shdr.sh_info);

		fprintf(elf," Align: %x\n", elf64_shdr.sh_addralign);

 	}
}

void read_Phdr()
{
	Elf64_Phdr elf64_phdr;


	if (fseek(file, padr, SEEK_SET) == -1) {
		printf("fseek error\n");
		return;
	}
	bool first_adr = false;
	unsigned long end_offset;
	for(int c=0;c<pnum;c++)
	{
		fprintf(elf," [%3d]\n",c);
			
		//file should be relocated
		fread(&elf64_phdr,1,sizeof(elf64_phdr),file);


		if(*(int*)elf64_phdr.p_type.b==0){
			fprintf(elf," Type:   NULL");
		}
		else if(*(int*)elf64_phdr.p_type.b==1){
			fprintf(elf," Type:   LOAD");

			if (!first_adr) {
				cadr = (unsigned int)*(unsigned long*)&elf64_phdr.p_offset;
				vadr = (unsigned int)*(unsigned long*)&elf64_phdr.p_vaddr;
				first_adr = true;
			}

			end_offset = *(unsigned long*)&elf64_phdr.p_offset;
			csize = end_offset - cadr + *(unsigned long*)&elf64_phdr.p_filesz;
		}
		else if(*(int*)elf64_phdr.p_type.b==2){
			fprintf(elf," Type:   DYNAMIC");
		}
		else if(*(int*)elf64_phdr.p_type.b==3){
			fprintf(elf," Type:   INTERP");
		}
		else if(*(int*)elf64_phdr.p_type.b==4){
			fprintf(elf," Type:   NOTE");
		}
		else if(*(int*)elf64_phdr.p_type.b==5){
			fprintf(elf," Type:   SHLIB");
		}
		else if(*(int*)elf64_phdr.p_type.b==6){
			fprintf(elf," Type:   PHDR");
		}
		else if(*(int*)elf64_phdr.p_type.b==7){
			fprintf(elf," Type:   TLS");
		}
		else if(*(int*)elf64_phdr.p_type.b==8){
			fprintf(elf," Type:   NUM");
		} else {
			fprintf(elf, " Type:   UNKONWN");
		}
 
		// fprintf(elf," Type:   %d", elf64_phdr.p_type);
		
		fprintf(elf," Flags:   0x%x", elf64_phdr.p_flags);
		
		fprintf(elf," Offset:   %ld", elf64_phdr.p_offset);
		
		fprintf(elf," VirtAddr:  %ld", elf64_phdr.p_vaddr);
		
		fprintf(elf," PhysAddr:   %ld", elf64_phdr.p_paddr);

		fprintf(elf," FileSiz:   %ld", elf64_phdr.p_filesz);

		fprintf(elf," MemSiz:   %ld", elf64_phdr.p_memsz);
		
		fprintf(elf," Align:   %ld\n", elf64_phdr.p_align);
	}
}


void read_symtable()
{
	Elf64_Sym elf64_sym;

	char name[40] = {};


	int cnt = 0;

	for(int c=0;c<symnum;c++)
	{
		fprintf(elf," [%3d]   ",c);

		fseek(file, symadr + c * sizeof(elf64_sym), SEEK_SET);
		
		//file should be relocated
		fread(&elf64_sym,1,sizeof(elf64_sym),file);

		unsigned long adr = stradr + *(unsigned int*)&elf64_sym.st_name;

		fseek(file, adr, SEEK_SET);

		fread(&name[cnt], 1, 1, file);

		// printf("get here\n");
		while(name[cnt] != '\0') {

			// printf("%c\n",name[cnt] );
			cnt++;
			fread(&name[cnt], 1, 1, file);
		}
		cnt = 0;

		if (strcmp(name, "__global_pointer$") == 0) {
			gp = *(unsigned long*)&elf64_sym.st_value;
		}

		if(strcmp(name, "_gp") == 0 ){
			gp = *(unsigned long*)&elf64_sym.st_value;
		}

		if(strcmp(name, "a") == 0) {
			a_addr = *(unsigned long*)&elf64_sym.st_value;
		}

		if (strcmp(name, "b") == 0) {
			b_addr = *(unsigned long*)&elf64_sym.st_value;
		}

		if (strcmp(name, "c") == 0) {
			c_addr = *(unsigned long*)&elf64_sym.st_value;
		}

		if (strcmp(name, "sum") == 0) {
			sum_addr = *(unsigned long*)&elf64_sym.st_value;
		}

		if (strcmp(name, "temp") == 0) {
			temp_addr = *(unsigned long*)&elf64_sym.st_value;
		}

		if (strcmp(name, "main") == 0) {
			madr = *(unsigned long*)&elf64_sym.st_value;
		}

		if(strcmp(name, "atexit") == 0) {
			endPC = *(unsigned long*)&elf64_sym.st_value;
		}

		if(strcmp(name,"result") == 0) {
			result_addr = *(unsigned long*)&elf64_sym.st_value;
		}

		fprintf(elf," Name:  %40s   \n", name);


		unsigned int type = (unsigned int)(elf64_sym.st_info&0xF);
		unsigned int bind = (unsigned int)elf64_sym.st_info>>4;

		if(bind == 0){
			fprintf(elf," Bind:   LOCAL");
		}
		else if(bind == 1) {
			fprintf(elf," Bind:   GLOBAL");
		}
		else if(bind == 2){
			fprintf(elf," Bind:   WEAK");
		}
		else if(bind == 3){
			fprintf(elf," Bind:   NUM");
		}else {
			fprintf(elf," Bind:   UNKONWN");
		}


		if(type == 0){
			fprintf(elf," Type:   NOTYPE");
		}
		else if(type == 1){
			fprintf(elf," Type:   OBJECT");
		}
		else if(type == 2){
			fprintf(elf," Type:   FUNC");
		}
		else if(type == 3){
			fprintf(elf," Type:   SECTION");
		}
		else if(type == 4){
			fprintf(elf," Type:   FILE");
		}
		else if(type == 5){
			fprintf(elf," Type:   COMMOM");
		}
		else if(type == 6){
			fprintf(elf," Type:   TLS");
		}
		else if(type == 7){
			fprintf(elf," Type:   NUM");
		}else {
			fprintf(elf," Type:   UNKONWN");
		}

		
		fprintf(elf," NDX:   %hd", elf64_sym.st_shndx);
		
		fprintf(elf," Size:   %ld", elf64_sym.st_size);

		fprintf(elf," Value:   %ld\n", elf64_sym.st_value);

		fflush(elf);

	}

}

// int main(int argc, char * argv[]) {
// 	read_elf(argv[1], argv[2]);
// }


