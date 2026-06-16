#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

//LC3 has 65536 memory locations (2^16)
#define MEMORY_MAX (1 << 16)
//Memory addresses will be stored in arrays
uint16_t memory[MEMORY_MAX];

//Registers
enum
{
	R_R0 = 0,
	R_R1,
	R_R2,
	R_R3,
	R_R4,
	R_R5,
	R_R6,
	R_R7,
	R_PC,
	R_COND,
	R_COUNT,
};
//Register array                                                                                                                             
uint16_t reg[R_COUNT];                                                                                                        

//Opcodes - LC3 has 16 
enum                                                                                                                          
{                                                                                                                             
        OP_BR = 0, //branch                                                                                                   
        OP_ADD,    //add                                                                                                      
        OP_LD,     //load                                                                                                     
        OP_ST,     //store                                                                                                    
        OP_JR,     //jump register                                                                                            
        OP_AND,    //bitwise and                                                                                              
        OP_LDR,    //load register                                                                                            
        OP_STR,    //store register                                                                                           
        OP_UN,     //unused                                                                                                   
        OP_NOT,    //bitwise not                                                                                              
        OP_LDI,    //load indirect                                                                                            
        OP_STI,    //store indirect                                                                                           
        OP_JMP,    //jump                                                                                                     
        OP_RES,    //reserved (unused)                                                                                        
        OP_LEA,    //load effective address                                                                                   
        OP_TRAP,   //execute trap                                                                                             
};                                                                                                                            
//Conditional flags for the COND register                                                                                     
enum                                                                                                                          
{                                                                                                                             
        FL_POS = 1 << 0,   //positive                                                                                         
        FL_ZRO = 1 << 1,    //zero                                                                                            
        FL_NEG = 1 << 2,    //negative                                                                                        
};                                                

int main(int argc, const char* argv[])
{
	//Usage check
	if (argc < 2)
	{
		printf("Usage: lc3 <image-file1> ... <image-fileN>");
		exit(2);
	}
	//Checking if we can run the input image filese
	for (int j = 1; j < argc; j++) {
		if (!read_image(argv[j])) 
		{
			printf("Failed to load image: %s\n", argv[j]);
			exit(1);
		}
	}

	//Set condition flag initially to zero
	reg[R_COND] = FL_ZRO;
	
	//Initializing the PC
	enum { PC_START = 0x3000 };
	reg[R_PC] = PC_START;
	
	//Helper function to preserve signed numbers 
	uint16_t sign_extend(uint16_t x, int bitCount)
	{
		//If the sign bit is 1, we shift all 1's to fill in the rest of the leading bits
		if ((x >> (bitCount -1) & 1) {
			x |= (0xFFFF << bitCount);
		}
		return x;
	}

	//Setting the cond flag after a value is written to a register
	void update_flags(uint16_t register) 
	{
		//if the used register is 0, set flag to 0
		//if the sign bit is set, the register carries a negative number
		//otherwise the number is positive 
		if (reg[register] = 0) {
			reg[R_COND] = FL_ZRO;
		} else if (reg[register] >> 15) { 
			reg[R_COND] = FL_NEG;
		} else {
			reg[R_COND] = FL_POS;
		}
	}
	int running = 1;
	while (running) 
	{
		//Grab the first instruction from program counter
		uint16_t instruction = memory_read(reg[R_PC]++);
		uint16_t op = instruction >> 12; //The op code is stored in the 4 left most bits, we need to bring them to the right
		
		//Check the opcode and run the instruction 
		switch (op) 
		{
			case OP_ADD:
				{
					uint16_t r0 = (instruction >> 9) & 0x7; //storing the destination register into r0
					uint16_t r1 = (instruction >> 6) & 0x7; //first number to be added
					uint16_t imm_flag = (instruction >> 5) & 0x1; //checks to see what add mode we are in
					
					if (imm_flag) {
						uint16_t number = (instruction & 0x1F);	
						uint16_t extendedNumber = sign_extend(number, 5);
						reg[r0] = reg[r1] + extendedNumber;
					} else {
						uint16_t r2 = (instruction & 0x7);
						reg[r0] = reg[r1] + reg[r2];
					}
					
					update_flags(r0);
				}
			case OP_AND:
				break;
			case OP_NOT:
				break;
			case OP_BR:
				break;
			case OP_JMP:
				break;
			case OP_JR:
				break;
			case OP_LD:
				break;
			case OP_LDI:
				break;
			case OP_LDR:
				break;
			case OP_LEA:
				break;
			case OP_ST:
				break;
			case OP_STI:
				break;
			case OP_STR:
				break;	
			case OP_TRAP:
				break;
			case OP_RES:
			case OP_RTI:
			default:
				break;
		}
	}
}




