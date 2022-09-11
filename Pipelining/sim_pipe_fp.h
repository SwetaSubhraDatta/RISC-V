#ifndef SIM_PIPE_FP_H_
#define SIM_PIPE_FP_H_

#include <stdio.h>
#include <string>
#include <vector>
using namespace std;

#define PROGRAM_SIZE 50

#define UNDEFINED 0xFFFFFFFF
#define NUM_SP_REGISTERS 9
#define NUM_SP_INT_REGISTERS 15
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 22
#define NUM_STAGES 5
#define MAX_UNITS 10

typedef enum {PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP, LWS, SWS, ADDS, SUBS, MULTS, DIVS} opcode_t;

typedef enum {IF, ID, EXE, MEM, WB} stage_t;

typedef enum {INTEGER, ADDER, MULTIPLIER, DIVIDER} exe_unit_t;

// instruction
typedef struct{
        opcode_t opcode; //opcode
        unsigned src1; //first source register
        unsigned src2; //second source register
        unsigned dest; //destination register
        unsigned immediate; //immediate field;
        string label; //in case of branch, label of the target instruction;
} instruction_t;

// execution unit
typedef struct{
	exe_unit_t type;  // execution unit type
	unsigned latency; // execution unit latency
	unsigned busy;    // 0 if execution unit is free, otherwise number of clock cycles during
                          // which the execution unit will be busy. It should be initialized
			  // to the latency of the unit when the unit becomes busy, and decremented
			  // at each clock cycle
	instruction_t instruction; // instruction using the functional unit
} unit_t;

class sim_pipe_fp{

        //instruction memory 
        instruction_t instr_memory[PROGRAM_SIZE];
        unsigned PC_ADDER;//We can also use pc this is equivalent to the ADDEr in the 5 stage pipleine
        int instruction_no=0;//Instruction no to be updated
        int no_of_instruction_executed=0;//no of execution exceuted
        int clock_cycles=0;//Keeps track of no of cc elapsed
        bool runcycle=false;//Runs the cc when the first program stage is completed
        bool data_hazard=false;//Makes itself true when a dat hazard s detected

        bool control_hazard=false;//Makes itself true when a control hazrd is dtcted

        //base address in the instruction memory where the program is loaded
        unsigned instr_base_address;

	//data memory - should be initialize to all 0xFF
	unsigned char *data_memory;

	//memory size in bytes
	unsigned data_memory_size;
	
	//memory latency in clock cycles
	unsigned data_memory_latency;

	//execution units
	unit_t exec_units[MAX_UNITS];
	unsigned num_units;

/**
 * The IF_ID PIPELINE COLUMNS Defined as STruct
 */
    struct
    {
        unsigned NPC = (UNDEFINED);
        instruction_t IR;
    }IF_ID_PIPELINE_COLUMN;

    /**
     * THe ID_EX_PIPELINE COLUMN  defined as Struct
     */

    struct
    {
        unsigned NPC=(UNDEFINED);
        instruction_t IR;
        unsigned A=sp_register_t (UNDEFINED);
        unsigned  B=sp_register_t (UNDEFINED);
        unsigned IMM=sp_register_t (UNDEFINED);
    }ID_EX_PIPELINE_CLOUMN;

/*
 * THe EX_MEM_PIPELINE COLUMN defined as Struct
 */
    struct
    {
        instruction_t IR;
        unsigned COND=sp_register_t(UNDEFINED);
        unsigned ALU_OUTPUT=sp_register_t(UNDEFINED);
        unsigned B=sp_register_t (UNDEFINED);
    }   EX_MEM_PIPELINE_COLUMN;
/**
 * THe MEM_WB_REGISTER_PIPELINE_COLUMN Defined
 * as struct
 */
    struct
    {
        instruction_t IR;
        unsigned  LMD=(UNDEFINED);
        unsigned ALU_OUTPUT=sp_register_t (UNDEFINED);
    }MEM_WB_REGISTER_COLUMN;

public:

	//instantiates the simulator with a data memory of given size (in bytes) and latency (in clock cycles)
	/* Note: 
           - initialize the registers to UNDEFINED value 
	   - initialize the data memory to all 0xFF values
	*/
	sim_pipe_fp(unsigned data_mem_size, unsigned data_mem_latency);
	
	//de-allocates the simulator
	~sim_pipe_fp();

	// adds one or more execution units of a given type to the processor
        // - exec_unit: type of execution unit to be added
        // - latency: latency of the execution unit (in clock cycles)
        // - instances: number of execution units of this type to be added
        void init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances=1);

	//loads the assembly program in file "filename" in instruction memory at the specified address
	void load_program(const char *filename, unsigned base_address=0x0);

	//runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0) 
	void run(unsigned cycles=0);
	
	//resets the state of the simulator
        /* Note: 
	   - registers should be reset to UNDEFINED value 
	   - data memory should be reset to all 0xFF values
	*/
	void reset();

	// returns value of the specified special purpose register for a given stage (at the "entrance" of that stage)
        // if that special purpose register is not used in that stage, returns UNDEFINED
        //
        // Examples (refer to page C-37 in the 5th edition textbook, A-32 in 4th edition of textbook)::
        // - get_sp_register(PC, IF) returns the value of PC
        // - get_sp_register(NPC, ID) returns the value of IF/ID.NPC
        // - get_sp_register(NPC, EX) returns the value of ID/EX.NPC
        // - get_sp_register(ALU_OUTPUT, MEM) returns the value of EX/MEM.ALU_OUTPUT
        // - get_sp_register(ALU_OUTPUT, WB) returns the value of MEM/WB.ALU_OUTPUT
	// - get_sp_register(LMD, ID) returns UNDEFINED
	/* Note: you are allowed to use a custom format for the IR register.
           Therefore, the test cases won't check the value of IR using this method. 
	   You can add an extra method to retrieve the content of IR */
	unsigned get_sp_register(sp_register_t reg, stage_t stage);

        //returns value of the specified integer general purpose register
        int get_int_register(unsigned reg);

        //set the value of the given integer general purpose register to "value"
        void set_int_register(unsigned reg, int value);

        //returns value of the specified floating point general purpose register
        float get_fp_register(unsigned reg);

        //set the value of the given floating point general purpose register to "value"
        void set_fp_register(unsigned reg, float value);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of clock cycles 
	unsigned get_clock_cycles();

	//returns the number of stalls added by processor
	unsigned get_stalls();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);

	//prints the values of the registers 
	void print_registers();
    bool Detect_DataHazard();
    bool Detect_ControlHazard();
    bool handle_hazard();
    bool pipline_needs_stalling=false;
    bool count_stalls_inserted=0;
    bool there_is_a_branch(opcode_t);
    bool Hazard_Check=false;
    bool Detect_Branch(opcode_t);
    void read_memory(unsigned ,unsigned);
    bool end_of_program_reached=false;
    void Instruction_fetch(int instruction_index);
    void Instruction_decode(int instruction_index);
    void Instruction_execute(int instruction_index);
    void Instruction_memory(int instruction_index);
    void Instruction_writeback(int instruction_index);
    vector<unsigned>integer_register;
    vector<unsigned>create_integer_Point_registers();
    vector<unsigned >create_floating_point_registers();

private:

	// returns a free exec unit for the particular instruction type
	unsigned get_free_unit(opcode_t opcode);	
	
	//reduce execution unit busy time (to be invoked at every clock cycle 
	void decrement_units_busy_time();

	//debug units
	void debug_units();

};

#endif /*SIM_PIPE_FP_H_*/
