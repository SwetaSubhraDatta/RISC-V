#ifndef SIM_OO_H_
#define SIM_OO_H_

#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>

using namespace std;

#define UNDEFINED 0xFFFFFFFF //constant used for initialization
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 24
#define NUM_STAGES 4
#define MAX_UNITS 10 
#define PROGRAM_SIZE 50 

// instructions supported
typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, AND, MULT, DIV, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, LWS, SWS, ADDS, SUBS, MULTS, DIVS} opcode_t;

// reservation stations types
typedef enum {INTEGER_RS, ADD_RS, MULT_RS, LOAD_B} res_station_t;

// execution units types
typedef enum {INTEGER, ADDER, MULTIPLIER, DIVIDER, MEMORY} exe_unit_t;

// stages names
typedef enum {ISSUE, EXECUTE, WRITE_RESULT, COMMIT} stage_t;

// instruction data type
typedef struct{
        opcode_t opcode; //opcode
        unsigned src1; //first source register in the assembly instruction (for SW, register to be written to memory)
        unsigned src2; //second source register in the assembly instruction
        unsigned dest; //destination register
        unsigned immediate; //immediate field
        string label; //for conditional branches, label of the target instruction - used only for parsing/debugging purposes
        bool already_issued=false;//checks if the instruction is already issud
        bool already_executed=false;//checks if the instrcution is already excuted
        bool already_written=false;
        bool already_commited=false;
} instruction_t;

// execution unit
typedef struct{
        exe_unit_t type;  // execution unit type
        unsigned latency; // execution unit latency
        unsigned busy;    // 0 if execution unit is free, otherwise number of clock cycles during
                          // which the execution unit will be busy. It should be initialized
                          // to the latency of the unit when the unit becomes busy, and decremented
                          // at each clock cycle
        unsigned pc; 	  // PC of the instruction using the functional unit
} unit_t;

// entry in the "instruction window"
typedef struct{
	unsigned pc=UNDEFINED;	// PC of the instruction
	unsigned issue=UNDEFINED;	// clock cycle when the instruction is issued
	unsigned exe=UNDEFINED;	// clock cycle when the instruction enters execution
	unsigned wr=UNDEFINED;	// clock cycle when the instruction enters write result
	unsigned commit=UNDEFINED;// clock cycle when the instruction commits (for stores, clock cycle when the store starts committing
} instr_window_entry_t;

// ROB entry
typedef struct{
	bool ready=false;	// ready field
	unsigned pc=UNDEFINED;  	// pc of corresponding instruction (set to UNDEFINED if ROB entry is available)
	stage_t state;	// state field
	unsigned destination=UNDEFINED; // destination field
	unsigned value=UNDEFINED;	      // value field
}rob_entry_t;

// reservation station entry
typedef struct{
	res_station_t type; // reservation station type
	unsigned name;	    // reservation station name (i.e., "Int", "Add", "Mult", "Load") for logging purposes
	unsigned pc=UNDEFINED;  	    // pc of corresponding instruction (set to UNDEFINED if reservation station is available)
	unsigned value1=UNDEFINED;    // Vj field
	unsigned value2=UNDEFINED;    // Vk field
	unsigned tag1=UNDEFINED;	    // Qj field
	unsigned tag2=UNDEFINED;	    // Qk field
	unsigned destination=UNDEFINED; // destination field
	unsigned address=UNDEFINED;     // address field (for loads and stores)
}res_station_entry_t;

//instruction window 
typedef struct{
	unsigned num_entries;
	vector<instr_window_entry_t> entries;//Changed here by Rob because Pointers was not helping me debug each res-station
} instr_window_t;

// ROB
typedef struct{
	unsigned num_entries;
	vector<rob_entry_t> entries;//Changed here because Pointers was not helping me debug each res-station
} rob_t;

// reservation stations
typedef struct{
	unsigned num_entries;
	vector<res_station_entry_t> entries;//Changed here because Pointers was not helping me debug each res-station
}res_stations_t;

class sim_ooo
        {

	/* Add the data members required by your simulator's implementation here */
    vector<unsigned>int_regs;
    vector<unsigned >floating_regs;
    vector<unsigned >int_tags;
    vector<unsigned>floating_point_tags;
    unsigned Rob_head;//Head of the rob trying something new here instead of a circular_queue
    unsigned PC_ADDER;//Like in project1
    //instruction_t IR_issued;
    bool run_cycles=false;//like LOAD_B for load this is doen to prevent the same instruction being issued to different station
    bool flush_protocol_activated=false;







    /* end added data members */
    //issue width
    unsigned issue_width;

	//instruction window
	instr_window_t pending_instructions;

	//reorder buffer
	rob_t rob;

	//reservation stations
	res_stations_t reservation_stations;

	//execution units
        unit_t exec_units[MAX_UNITS];
        unsigned num_units;

	//instruction memory
	instruction_t instr_memory[PROGRAM_SIZE];

        //base address in the instruction memory where the program is loaded
        unsigned instr_base_address;

	//data memory - should be initialize to all 0xFF
	unsigned char *data_memory;

	//memory size in bytes
	unsigned data_memory_size;
	
	//instruction executed
	unsigned instructions_executed;

	//clock cycles
	unsigned clock_cycles;

	//execution log
	stringstream log;

public:


	/* Instantiates the simulator
          	Note: registers must be initialized to UNDEFINED value, and data memory to all 0xFF values
        */
	sim_ooo(unsigned mem_size, 		// size of data memory (in byte)
		unsigned rob_size, 		// number of ROB entries
                unsigned num_int_res_stations,	// number of integer reservation stations 
                unsigned num_add_res_stations,	// number of ADD reservation stations
                unsigned num_mul_res_stations, 	// number of MULT/DIV reservation stations
                unsigned num_load_buffers,// number of LOAD buffers
		unsigned issue_width=1		// issue width
        );	
	
	//de-allocates the simulator
	~sim_ooo();

        // adds one or more execution units of a given type to the processor
        // - exec_unit: type of execution unit to be added
        // - latency: latency of the execution unit (in clock cycles)
        // - instances: number of execution units of this type to be added
        void init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances=1);

	//related to functional unit
	unsigned get_free_unit(opcode_t opcode);

	//loads the assembly program in file "filename" in instruction memory at the specified address
	void load_program(const char *filename, unsigned base_address=0x0);

	//runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0) 
	void run(unsigned cycles=0);
	
	//resets the state of the simulator
        /* Note: 
	   - registers should be reset to UNDEFINED value 
	   - data memory should be reset to all 0xFF values
	   - instruction window, reservation stations and rob should be cleaned
	*/
	void reset();

       //returns value of the specified integer general purpose register
        int get_int_register(unsigned reg);

        //set the value of the given integer general purpose register to "value"
        void set_int_register(unsigned reg, int value);

        //returns value of the specified floating point general purpose register
        float get_fp_register(unsigned reg);

        //set the value of the given floating point general purpose register to "value"
        void set_fp_register(unsigned reg, float value);

	// returns the index of the ROB entry that will write this integer register (UNDEFINED if the value of the register is not pending
	unsigned get_int_register_tag(unsigned reg);

	// returns the index of the ROB entry that will write this floating point register (UNDEFINED if the value of the register is not pending
	unsigned get_fp_register_tag(unsigned reg);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of clock cycles 
	unsigned get_clock_cycles();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);

	//prints the values of the registers 
	void print_registers();

	//prints the status of processor excluding memory
	void print_status();

	// prints the content of the ROB
	void print_rob();

	//prints the content of the reservation stations
	void print_reservation_stations();

	//print the content of the instruction window
	void print_pending_instructions();

	//initialize the execution log
	void init_log();

	//commit an instruction to the log
	void commit_to_log(instr_window_entry_t iwe);

	//print log
	void print_log();

	void clean_res_station(unsigned entry)
    {
	    reservation_stations.entries[entry].pc=UNDEFINED;
	    reservation_stations.entries[entry].address=UNDEFINED;
	    reservation_stations.entries[entry].tag2=UNDEFINED;
	    reservation_stations.entries[entry].tag1=UNDEFINED;
	    reservation_stations.entries[entry].destination=UNDEFINED;
	    reservation_stations.entries[entry].value1=UNDEFINED;
	    reservation_stations.entries[entry].value2=UNDEFINED;
    }

    void clean_rob(unsigned entry)
    {
	    rob.entries[entry].destination=UNDEFINED;
	    rob.entries[entry].pc=UNDEFINED;
	    rob.entries[entry].state=ISSUE;
	    rob.entries[entry].ready=false;
	    rob.entries[entry].value=UNDEFINED;
    }


	/*
	 * This are the function declarations that I have used
	 */
	void  issue();
	void execute();
	void write_back();
	void commit();
	void update_restations_and_rob(unsigned restation_entry,instruction_t& IR_issued);
	void follow_flush_protocol(unsigned rob_index);


	/**
	 * This are for checking opcodes
	 */
	 //Mmemory operations
	 bool is_Integer_Load_and_Store(opcode_t op);
	 bool is_floating_load_and_store(opcode_t op);
	 //Memory operations
	 bool just_loads(opcode_t op);
	 bool just_store(opcode_t);
	 bool just_Division(opcode_t);
	 bool just_Mult(opcode_t);

	 //Integer operations
	 bool is_Integer_operation(opcode_t op);
	  //Floating Point perations
	  bool is_floating_point_operations_add_sub(opcode_t op);
	  bool is_floating_point_operations_mult_div(opcode_t);
	  void start_execution_operation(unsigned resstat_entry, unsigned execution_station_entry,
                                     instruction_t& instruction);
	  bool check_if_ADDER_MULTIPLIER_INTEGER(unsigned exec_unit_stat_entry);
	  bool Check_INTEGER_OR_FLOAT_OPERATIONS(opcode_t opcode);



};

#endif /*SIM_OOO_H_*/
