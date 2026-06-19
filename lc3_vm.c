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

//TRAP codes
enum
{
	TRAP_GETC = 0x20,   //Read a single character from keyboard - not echoed
	TRAP_OUT = 0x21,    //Write a character to console
	TRAP_PUTS = 0x22,   //Write a string to console - word form
	TRAP_IN = 0x23,     //Print a prompt and read a character - not echoed
	TRAP_PUTSP = 0x24,  //Write a string to console - byte form
	TRAP_HALT = 0x25,   //Halt execution and print to console
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
					
					//If the add is immediate, we sign extend the number before we add
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
				break;
			case OP_AND:
				{
					uint16_t r0 = (instruction >> 9) & 0x7; //storing the destination register into r0
					uint16_t r1 = (instruction >> 6) & 0x7; //register storing the first number 
					uint16_t imm_flag = (instruction >> 5) & 0x1; //checking if the imm flag is set
					
					//if the imm flag is set, we treat second operand as an immediate value, otherwise we pull from sr2
					if (imm_flag) {
						uint16_t number = (instruction & 0x1F);
						uint16_t extendedNumber = sign_extend(number, 5);
						reg[r0] = reg[r1] & extendedNumber;
					} else {
						uint16_t r2 = (instruction & 0x7);
						reg[r0] = reg[r1] & reg[r2];
					}
				
					update_flags(r0);
				}
				break
			case OP_NOT:
				{
					//Grabbing the destination register and the source register
					uint16_t dr = (instruction >> 9) & 0x7;
					uint16_t sr = (instruction >> 6) & 0x7;
					
					//Storing the bitwise complement of reg[sr] into dr and setting condition codes
					reg[dr] = ~(reg[sr]);
					update_flags(dr);
				}
				break;
			case OP_BR:
				{
					//Grabbing all the condition codes 
					uint16_t n = (instruction >> 11) & 0x1;
					uint16_t z = (instruction >> 10) & 0x1;
					uint16_t p = (instruction >> 9) & 0x1;
					
					//checking if any of them are set - if they are, check what cond flag is set to 
					if ((n && (reg[R_COND] & FL_NEG) || (z && (reg[R_COND] & FL_ZRO)) || (p && (reg[R_COND] & FL_POS)) {
						uint16_t number = (instruction & 0x1FF);
						uint16_t offset = sign_extend(number, 9);
						reg[R_PC] += offset;
					} 		
				}
				break;
			case OP_JMP:
				{
					//Check if we are doing a normal jump or ret
					uint16_t baseR = (instructions >> 6) & 0x7;

					//If baseR is 7 then we jump to address in R7, otherwise jump to address in BaseR
					if (baseR == 0x7) {
						reg[R_PC] = reg[R_R7];
					} else {
						reg[R_PC] =  reg[baseR];
					}
				}
				break;
			case OP_JR:
				{
					//Check if we are in JSR or JSRR
					uint16_t mode = (instructions >> 11) & 0x1;
					//Store the PC into R7
					reg[R_R7] = reg[R_PC];

					//If mode is set, we are in JSR - add the pcoffset11 value to pc
					//Otherwise we are in JSRR - set the PC to the address stored in Base Register
					if (mode) {
						uint16_t number = (instructions & 0x07FF);
						uint16_t offset = sign_extend(number, 11);
						reg[R_PC] += offset;
					} else {
						uint16_t baseR = (instructions >> 6) & 0x7;
						reg[R_PC] += reg[baseR];
					}
				}
				break;
			case OP_LD:
				{
					//Destination register
					uint16_t dr = (instructions >> 9) & 0x7;
					//Offset - sign extended
					uint16_t number = (instructions & 0x1FF);
					uint16_t offset = sign_extend(number, 9);
					
					reg[dr] = memory_read(reg[R_PC] + offset);
					update_flags(dr);	
				break;
			case OP_LDI:
				{
					uint16_t dr = (instruction >> 9) & 0x7; //Destination register 
					uint16_t number = (instruction & 0x1FF); //immediate value masked from instruction
					uint16_t offset = sign_extend(number, 9); //the offset we add to pc 
					
					reg[dr] = memory_read(memory_read(reg[R_PC] + offset));
					update_flags(dr);
				}
				break;
			case OP_LDR:
				{
					//Grabbing destination, base, and sign extending the offset
					uint16_t dr = (instructions >> 9) & 0x7;
					uint16_t baseR = (instructions >> 6) & 0x7;
					uint16_t number = (instructions & 0x3F);
					uint16_t offset = sign_extend(number, 6)
					
					//Load the value from baseR + offset into DR, set condition codes
					reg[dr] = memory_read(reg[baseR] + offset);
					update_flags(dr);	
				}
				break;
			case OP_LEA:
				{
					//Grabbing destination register
					uint16_t dr = (instructions >> 9) & 0x7;
					//Grabbing the offset 
					uint16_t number = (instructions & 0x1FF);
					uint16_t offset = sign_extend(number, 9);
					
					//store the address at PC + offset into dr and set condition codes
					reg[dr] = reg[R_PC] + offset;
					update_flags(dr);
				}
				break;
			case OP_ST:
				{
					//Grabbing the source register
					uint16_t sr = (instructions >> 9) & 0x7;
					
					//Calculating the destination in memory
					uint16_t number = (instructions & 0x1FF);
					uint16_t offset = sign_extend(number, 9);
					uint16_t location = reg[R_PC] + offset;
				
					//Store contents of SR into location
					memory_write(location, reg[sr]);	
				}
				break;
			case OP_STI:
				{
					//Grabbing the source register
					uint16_t sr = (instructions >> 9) & 0x7;
				
					//Calculating the destination of the address
					uint16_t number = (instructions & 0x1FF);
					uint16_t offset = sign_extend(number, 9);
					uint16_t adrLocation = reg[R_PC] + offset;

					//Store SR contents into the address found in adrLocation
					memory_write(memory_read(adrLocation), reg[sr]);
				}		
				break;
			case OP_STR:
				{
					//Grabbing the source register and base register
					uint16_t sr = (instructions >> 9) & 0x7;
					uint16_t br = (instructions >> 6) & 0x7;
					
					//Calculating the destination address
					uint16_t number = (instructions & 0x3F);
					uint16_t offset = sign_extend(number, 6);
					uint16_t location = reg[br] + offset;
					
					//Storing sr contents into location
					memory_write(location, reg[sr]);
				}
				break;	
			case OP_TRAP:
				{
					//Storing PC into R7 and grabbing the trapvect8
					reg[R_R7] = reg[R_PC];
					uint16_t tv8 = (instructions & 0xFF);
					
					switch (tv8)
					{
						case TRAP_GETC:
							break;
						case TRAP_OUT:
							break;
						case TRAP_PUTS:
							{
								//each char is a word
								uint16_t* c = memory + reg[R_R0];
							
								//while c is a valid memory address, cast and print out each value
								while (*c) 
								{
									putc((char)*c, stdout);
									++c;
								}
								fflush(stdout);
							}
							break;
						case TRAP_IN:
							break;
						case TRAP_PUTSP:
							break;
						case TRAP_HALT:
							break;
					}
					//Loading the tv8 address into PC
					reg[R_PC] = tv8;
				}
				break;
			case OP_RES:
			case OP_RTI:
			default:
				break;
		}
	}
}




