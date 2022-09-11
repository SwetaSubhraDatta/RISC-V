/**
 * @file sim_ooo.cc
 * @author Sweta Subhra Datta
 * @brief 
 * @version 0.1
 * @date 2022-09-11
 * 
 * @copyright MIT License 
 * 
 */
#include "sim_ooo.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

using namespace std;

//used for debugging purposes
///NOT using them pointers were a headache
static const char *stage_names[NUM_STAGES] = {"ISSUE", "EXE", "WR", "COMMIT"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "AND", "MULT", "DIV", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "LWS", "SWS", "ADDS", "SUBS", "MULTS", "DIVS"};
static const char *res_station_names[5]={"Int", "Add", "Mult", "Load"};

/* =============================================================

   HELPER FUNCTIONS (misc)

   ============================================================= */


/* convert a float into an unsigned */
inline unsigned float2unsigned(float value){
	unsigned result;
	memcpy(&result, &value, sizeof value);
	return result;
}

/* convert an unsigned into a float */
inline float unsigned2float(unsigned value){
	float result;
	memcpy(&result, &value, sizeof value);
	return result;
}

/* convert integer into array of unsigned char - little indian */
inline void unsigned2char(unsigned value, unsigned char *buffer){
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = (value >> 16) & 0xFF;
        buffer[3] = (value >> 24) & 0xFF;
}

/* convert array of char into integer - little indian */
inline unsigned char2unsigned(unsigned char *buffer){
       return buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
}

/* the following six functions return the kind of the considered opcdoe */

bool is_branch(opcode_t opcode){
        return (opcode == BEQZ || opcode == BNEZ || opcode == BLTZ || opcode == BLEZ || opcode == BGTZ || opcode == BGEZ || opcode == JUMP);
}

bool is_memory(opcode_t opcode){
        return (opcode == LW || opcode == SW || opcode == LWS || opcode == SWS);
}

bool is_int_r(opcode_t opcode){
        return (opcode == ADD || opcode == SUB || opcode == XOR || opcode == AND);
}

bool is_int_imm(opcode_t opcode){
        return (opcode == ADDI || opcode == SUBI );
}

bool is_int(opcode_t opcode){
        return (is_int_r(opcode) || is_int_imm(opcode));
}

bool is_fp_alu(opcode_t opcode){
        return (opcode == ADDS || opcode == SUBS || opcode == MULTS || opcode == DIVS);
}

/* clears a ROB entry */
void clean_rob(rob_entry_t *entry){
        entry->ready=false;
        entry->pc=UNDEFINED;
        entry->state=ISSUE;
        entry->destination=UNDEFINED;
        entry->value=UNDEFINED;
}

/* clears a reservation station */
void clean_res_station(res_station_entry_t *entry){

        entry->pc=UNDEFINED;
        entry->value1=UNDEFINED;
        entry->value2=UNDEFINED;
        entry->tag1=UNDEFINED;
        entry->tag2=UNDEFINED;
        entry->destination=UNDEFINED;
        entry->address=UNDEFINED;
}



/* clears an entry if the instruction window */
void clean_instr_window(instr_window_entry_t *entry){
        entry->pc=UNDEFINED;
        entry->issue=UNDEFINED;
        entry->exe=UNDEFINED;
        entry->wr=UNDEFINED;
        entry->commit=UNDEFINED;
}

/* implements the ALU operation 
   NOTE: this function does not cover LOADS and STORES!
*/
unsigned alu(opcode_t opcode, unsigned value1, unsigned value2, unsigned immediate, unsigned pc){
	unsigned result;
	switch(opcode){
			case ADD:
			case ADDI:
				result = value1+value2;
				break;
			case SUB:
			case SUBI:
				result = value1-value2;
				break;
			case XOR:
				result = value1 ^ value2;
				break;
			case AND:
				result = value1 & value2;
				break;
			case MULT:
				result = value1 * value2;
				break;
			case DIV:
				result = value1 / value2;
				break;
			case ADDS:
				result = float2unsigned(unsigned2float(value1) + unsigned2float(value2));
				break;
			case SUBS:
				result = float2unsigned(unsigned2float(value1) - unsigned2float(value2));
				break;
			case MULTS:
				result = float2unsigned(unsigned2float(value1) * unsigned2float(value2));
				break;
			case DIVS:
				result = float2unsigned(unsigned2float(value1) / unsigned2float(value2));
				break;
			case JUMP:
				result = pc + 4 + immediate;
				break;
			default: //branches
				int reg = (int) value1;
				bool condition = ((opcode == BEQZ && reg==0) ||
				(opcode == BNEZ && reg!=0) ||
  				(opcode == BGEZ && reg>=0) ||
  				(opcode == BLEZ && reg<=0) ||      
  				(opcode == BGTZ && reg>0) ||       
  				(opcode == BLTZ && reg<0));
				if (condition)
	 				result = pc+4+immediate;
				else 
					result = pc+4;
				break;
	}
	return 	result;
}

/* writes the data memory at the specified address */
void sim_ooo::write_memory(unsigned address, unsigned value){
	unsigned2char(value,data_memory+address);
}

/* =============================================================

   Handling of FUNCTIONAL UNITS

   ============================================================= */

/* initializes an execution unit */
void sim_ooo::init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances){
        for (unsigned i=0; i<instances; i++){
                exec_units[num_units].type = exec_unit;
                exec_units[num_units].latency = latency;
                exec_units[num_units].busy = 0;
                exec_units[num_units].pc = UNDEFINED;
                num_units++;
        }
}

/* returns a free unit for that particular operation or UNDEFINED if no unit is currently available */
unsigned sim_ooo::get_free_unit(opcode_t opcode){
	if (num_units == 0){
		cout << "ERROR:: simulator does not have any execution units!\n";
		exit(-1);
	}
	for (unsigned u=0; u<num_units; u++){
		switch(opcode){
			//Integer unit
			case ADD:
			case ADDI:
			case SUB:
			case SUBI:
			case XOR:
			case AND:
			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
			case JUMP:
				if (exec_units[u].type==INTEGER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			//memory unit
			case LW:
			case SW:
			case LWS: 
			case SWS:
				if (exec_units[u].type==MEMORY && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			// FP adder
			case ADDS:
			case SUBS:
				if (exec_units[u].type==ADDER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			// Multiplier
			case MULT:
			case MULTS:
				if (exec_units[u].type==MULTIPLIER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			// Divider
			case DIV:
			case DIVS:
				if (exec_units[u].type==DIVIDER && exec_units[u].busy==0 && exec_units[u].pc==UNDEFINED) return u;
				break;
			default:
				cout << "ERROR:: operations not requiring exec unit!\n";
				exit(-1);
		}
	}
	return UNDEFINED;
}



/* ============================================================================

   Primitives used to print out the state of each component of the processor:
   	- registers
	- data memory
	- instruction window
        - reservation stations and load buffers
        - (cycle-by-cycle) execution log
	- execution statistics (CPI, # instructions executed, # clock cycles) 

   =========================================================================== */
 

/* prints the content of the data memory */
void sim_ooo::print_memory(unsigned start_address, unsigned end_address){
	cout << "DATA MEMORY[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	for (unsigned i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3){
			cout << endl;
		}
	} 
}

/* prints the value of the registers */
void sim_ooo::print_registers(){
        unsigned i;
	cout << "GENERAL PURPOSE REGISTERS" << endl;
	cout << setfill(' ') << setw(8) << "Register" << setw(22) << "Value" << setw(5) << "ROB" << endl;
        for (i=0; i< NUM_GP_REGISTERS; i++){
                if (get_int_register_tag(i)!=UNDEFINED) 
			cout << setfill(' ') << setw(7) << "R" << dec << i << setw(22) << "-" << setw(5) << get_int_register_tag(i) << endl;
                else if (get_int_register(i)!=(int)UNDEFINED) 
			cout << setfill(' ') << setw(7) << "R" << dec << i << setw(11) << get_int_register(i) << hex << "/0x" << setw(8) << setfill('0') << get_int_register(i) << setfill(' ') << setw(5) << "-" << endl;
        }
	for (i=0; i< NUM_GP_REGISTERS; i++){
                if (get_fp_register_tag(i)!=UNDEFINED) 
			cout << setfill(' ') << setw(7) << "F" << dec << i << setw(22) << "-" << setw(5) << get_fp_register_tag(i) << endl;
                else if (get_fp_register(i)!=UNDEFINED) 
			cout << setfill(' ') << setw(7) << "F" << dec << i << setw(11) << get_fp_register(i) << hex << "/0x" << setw(8) << setfill('0') << float2unsigned(get_fp_register(i)) << setfill(' ') << setw(5) << "-" << endl;
	}
	cout << endl;
}

/* prints the content of the ROB */
void sim_ooo::print_rob(){
	cout << "REORDER BUFFER" << endl;
	cout << setfill(' ') << setw(5) << "Entry" << setw(6) << "Busy" << setw(7) << "Ready" << setw(12) << "PC" << setw(10) << "State" << setw(6) << "Dest" << setw(12) << "Value" << endl;
	for(unsigned i=0; i< rob.num_entries;i++){
		rob_entry_t entry = rob.entries[i];
		instruction_t instruction;
		if (entry.pc != UNDEFINED) instruction = instr_memory[(entry.pc-instr_base_address)>>2]; 
		cout << setfill(' ');
		cout << setw(5) << i;
		cout << setw(6);
		if (entry.pc==UNDEFINED) cout << "no"; else cout << "yes";
		cout << setw(7);
		if (entry.ready) cout << "yes"; else cout << "no";	
		if (entry.pc!= UNDEFINED ) cout << "  0x" << hex << setfill('0') << setw(8) << entry.pc;
		else	cout << setw(12) << "-";
		cout << setfill(' ') << setw(10);
		if (entry.pc==UNDEFINED) cout << "-";		
		else cout << stage_names[entry.state];
		if (entry.destination==UNDEFINED) cout << setw(6) << "-";
		else{
			if (instruction.opcode == SW || instruction.opcode == SWS)
				cout << setw(6) << dec << entry.destination; 
			else if (entry.destination < NUM_GP_REGISTERS)
				cout << setw(5) << "R" << dec << entry.destination;
			else
				cout << setw(5) << "F" << dec << entry.destination-NUM_GP_REGISTERS;
		}
		if (entry.value!=UNDEFINED) cout << "  0x" << hex << setw(8) << setfill('0') << entry.value << endl;	
		else cout << setw(12) << setfill(' ') << "-" << endl;
	}
	cout << endl;
}

/* prints the content of the reservation stations */
void sim_ooo::print_reservation_stations(){
	cout << "RESERVATION STATIONS" << endl;
	cout  << setfill(' ');
	cout << setw(7) << "Name" << setw(6) << "Busy" << setw(12) << "PC" << setw(12) << "Vj" << setw(12) << "Vk" << setw(6) << "Qj" << setw(6) << "Qk" << setw(6) << "Dest" << setw(12) << "Address" << endl; 
	for(unsigned i=0; i< reservation_stations.num_entries;i++){
		res_station_entry_t entry = reservation_stations.entries[i];
	 	cout  << setfill(' ');
		cout << setw(6); 
		cout << res_station_names[entry.type];
		cout << entry.name + 1;
		cout << setw(6);
		if (entry.pc==UNDEFINED) cout << "no"; else cout << "yes";
		if (entry.pc!= UNDEFINED ) cout << setw(4) << "  0x" << hex << setfill('0') << setw(8) << entry.pc;
		else	cout << setfill(' ') << setw(12) <<  "-";			
		if (entry.value1!= UNDEFINED ) cout << "  0x" << setfill('0') << setw(8) << hex << entry.value1;
		else	cout << setfill(' ') << setw(12) << "-";			
		if (entry.value2!= UNDEFINED ) cout << "  0x" << setfill('0') << setw(8) << hex << entry.value2;
		else	cout << setfill(' ') << setw(12) << "-";			
		cout << setfill(' ');
		cout <<setw(6);
		if (entry.tag1!= UNDEFINED ) cout << dec << entry.tag1;
		else	cout << "-";			
		cout <<setw(6);
		if (entry.tag2!= UNDEFINED ) cout << dec << entry.tag2;
		else	cout << "-";			
		cout <<setw(6);
		if (entry.destination!= UNDEFINED ) cout << dec << entry.destination;
		else	cout << "-";			
		if (entry.address != UNDEFINED ) cout <<setw(4) << "  0x" << setfill('0') << setw(8) << hex << entry.address;
		else	cout << setfill(' ') << setw(12) <<  "-";			
		cout << endl;	
	}
	cout << endl;
}

/* prints the state of the pending instructions */
void sim_ooo::print_pending_instructions(){
	cout << "PENDING INSTRUCTIONS STATUS" << endl;
	cout << setfill(' ');
	cout << setw(10) << "PC" << setw(7) << "Issue" << setw(7) << "Exe" << setw(7) << "WR" << setw(7) << "Commit";
	cout << endl;
	for(unsigned i=0; i< pending_instructions.num_entries;i++){
		instr_window_entry_t entry = pending_instructions.entries[i];
		if (entry.pc!= UNDEFINED ) cout << "0x" << setfill('0') << setw(8) << hex << entry.pc;
		else	cout << setfill(' ') << setw(10)  << "-";
		cout << setfill(' ');
		cout << setw(7);			
		if (entry.issue!= UNDEFINED ) cout << dec << entry.issue;
		else	cout << "-";			
		cout << setw(7);			
		if (entry.exe!= UNDEFINED ) cout << dec << entry.exe;
		else	cout << "-";			
		cout << setw(7);			
		if (entry.wr!= UNDEFINED ) cout << dec << entry.wr;
		else	cout << "-";			
		cout << setw(7);			
		if (entry.commit!= UNDEFINED ) cout << dec << entry.commit;
		else	cout << "-";
		cout << endl;			
	}
	cout << endl;
}


/* initializes the execution log */
void sim_ooo::init_log(){
	log << "EXECUTION LOG" << endl;
	log << setfill(' ');
	log << setw(10) << "PC" << setw(7) << "Issue" << setw(7) << "Exe" << setw(7) << "WR" << setw(7) << "Commit";
	log << endl;
}

/* adds an instruction to the log */
void sim_ooo::commit_to_log(instr_window_entry_t entry){
                if (entry.pc!= UNDEFINED ) log << "0x" << setfill('0') << setw(8) << hex << entry.pc;
                else    log << setfill(' ') << setw(10)  << "-";
                log << setfill(' ');
                log << setw(7);
                if (entry.issue!= UNDEFINED ) log << dec << entry.issue;
                else    log << "-";
                log << setw(7);
                if (entry.exe!= UNDEFINED ) log << dec << entry.exe;
                else    log << "-";
                log << setw(7);
                if (entry.wr!= UNDEFINED ) log << dec << entry.wr;
                else    log << "-";
                log << setw(7);
                if (entry.commit!= UNDEFINED ) log << dec << entry.commit;
                else    log << "-";
                log << endl;
}

/* prints the content of the log */
void sim_ooo::print_log(){
	cout << log.str();
}

/* prints the state of the pending instruction, the content of the ROB, the content of the reservation stations and of the registers */
void sim_ooo::print_status(){
	print_pending_instructions();
	print_rob();
	print_reservation_stations();
	print_registers();
}

/* execution statistics */

float sim_ooo::get_IPC(){return (float)instructions_executed/clock_cycles;}

unsigned sim_ooo::get_instructions_executed(){return instructions_executed;}

unsigned sim_ooo::get_clock_cycles(){return clock_cycles;}



/* ============================================================================

   PARSER

   =========================================================================== */


void sim_ooo::load_program(const char *filename, unsigned base_address){

   /* initializing the base instruction address */
   instr_base_address = base_address;

   /* creating a map with the valid opcodes and with the valid labels */
   map<string, opcode_t> opcodes; //for opcodes
   map<string, unsigned> labels;  //for branches
   for (int i=0; i<NUM_OPCODES; i++)
	 opcodes[string(instr_names[i])]=(opcode_t)i;

   /* opening the assembly file */
   ifstream fin(filename, ios::in | ios::binary);
   if (!fin.is_open()) {
      cerr << "error: open file " << filename << " failed!" << endl;
      exit(-1);
   }

   /* parsing the assembly file line by line */
   string line;
   unsigned instruction_nr = 0;
   while (getline(fin,line)){
	
	// set the instruction field
	char *str = const_cast<char*>(line.c_str());

  	// tokenize the instruction
	char *token = strtok (str," \t");
	map<string, opcode_t>::iterator search = opcodes.find(token);
        if (search == opcodes.end()){
		// this is a label for a branch - extract it and save it in the labels map
		string label = string(token).substr(0, string(token).length() - 1);
		labels[label]=instruction_nr;
		// move to next token, which must be the instruction opcode
		token = strtok (NULL, " \t");
		search = opcodes.find(token);
		if (search == opcodes.end()) cout << "ERROR: invalid opcode: " << token << " !" << endl;
	}

	instr_memory[instruction_nr].opcode = search->second;

	//reading remaining parameters
	char *par1;
	char *par2;
	char *par3;
	switch(instr_memory[instruction_nr].opcode){
		case ADD:
		case SUB:
		case XOR:
		case AND:
		case MULT:
		case DIV:
		case ADDS:
		case SUBS:
		case MULTS:
		case DIVS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "RF"));
			instr_memory[instruction_nr].src2 = atoi(strtok(par3, "RF"));
			break;
		case ADDI:
		case SUBI:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
			instr_memory[instruction_nr].immediate = strtoul (par3, NULL, 0); 
			break;
		case LW:
		case LWS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
			break;
		case SW:
		case SWS:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "RF"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src2 = atoi(strtok(NULL, "R"));
			break;
		case BEQZ:
		case BNEZ:
		case BLTZ:
		case BGTZ:
		case BLEZ:
		case BGEZ:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].label = par2;
			break;
		case JUMP:
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].label = par2;
		default:
			break;

	} 

	/* increment instruction number before moving to next line */
	instruction_nr++;
   }
   //reconstructing the labels of the branch operations
   int i = 0;
   while(true){
   	instruction_t instr = instr_memory[i];
	if (instr.opcode == EOP) break;
	if (instr.opcode == BLTZ || instr.opcode == BNEZ ||
            instr.opcode == BGTZ || instr.opcode == BEQZ ||
            instr.opcode == BGEZ || instr.opcode == BLEZ ||
            instr.opcode == JUMP
	 ){
		instr_memory[i].immediate = (labels[instr.label] - i - 1) << 2;
	}
        i++;
   }

}

/* ============================================================================

   Simulator creation, initialization and deallocation 

   =========================================================================== */

sim_ooo::sim_ooo(unsigned mem_size,
                unsigned rob_size,
                unsigned num_int_res_stations,
                unsigned num_add_res_stations,
                unsigned num_mul_res_stations,
                unsigned num_load_res_stations,
		unsigned max_issue)
		{
    //The parts of added in constructor
    //Initialises the size of the int and Floating point registers
    int_regs=vector<unsigned >(NUM_GP_REGISTERS,UNDEFINED);
    floating_regs=vector<unsigned >(NUM_GP_REGISTERS,UNDEFINED);
    int_tags=vector<unsigned >(NUM_GP_REGISTERS,UNDEFINED);
    floating_point_tags=vector<unsigned>(NUM_GP_REGISTERS,UNDEFINED);
    //end of added functionalities to the constructor by me

	//memory
	data_memory_size = mem_size;
	data_memory = new unsigned char[data_memory_size];

	//issue width
	issue_width = max_issue;

	//rob, instruction window, reservation stations
	rob.num_entries=rob_size;
	pending_instructions.num_entries=rob_size;
	reservation_stations.num_entries= num_int_res_stations+num_load_res_stations+num_add_res_stations+num_mul_res_stations;
	rob.entries =vector<rob_entry_t>(rob.num_entries);//New additons instead of pointers vectors
	pending_instructions.entries = vector<instr_window_entry_t>(pending_instructions.num_entries);//New additons instead of pointers vectors
	reservation_stations.entries = vector<res_station_entry_t>(reservation_stations.num_entries);//New additons instead of pointers vectors
	unsigned n=0;
	for (unsigned i=0; i<num_int_res_stations; i++,n++){
		reservation_stations.entries[n].type=INTEGER_RS;
		reservation_stations.entries[n].name=i;
	}
	for (unsigned i=0; i<num_load_res_stations; i++,n++){
		reservation_stations.entries[n].type=LOAD_B;
		reservation_stations.entries[n].name=i;
	}
	for (unsigned i=0; i<num_add_res_stations; i++,n++){
		reservation_stations.entries[n].type=ADD_RS;
		reservation_stations.entries[n].name=i;
	}
	for (unsigned i=0; i<num_mul_res_stations; i++,n++){
		reservation_stations.entries[n].type=MULT_RS;
		reservation_stations.entries[n].name=i;
	}
	//execution units
	num_units = 0;
	reset();
}
	
sim_ooo::~sim_ooo(){
	delete [] data_memory;
    rob.entries.clear();
    pending_instructions.entries.clear();
	reservation_stations.entries.clear();
}

/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */

/* core of the simulator */
void sim_ooo::run(unsigned cycles)
{
    int single_cycles=0;

    if(cycles==0)
    {
        run_cycles=true;
    }
    while(single_cycles<cycles ||run_cycles)
    {

        issue();
        execute();
        write_back();
        commit();
        if (clock_cycles > 0) {
            for (int i = 0; i < num_units; i++)
                if (exec_units[i].busy != UNDEFINED && exec_units[i].busy != 0) {
                    exec_units[i].busy = exec_units[i].busy - 1;
                }
        }
        unsigned pending_window_empty=0;
        if (run_cycles) {
            for (unsigned int l = 0; l < pending_instructions.num_entries; l++) {
                if (pending_instructions.entries[l].pc == UNDEFINED)
                {pending_window_empty++;}
            }

            if (pending_window_empty == pending_instructions.num_entries &&
                !flush_protocol_activated) {//no pending instructions!
                run_cycles = false;
            }
        }
        single_cycles=single_cycles+1;
        clock_cycles = clock_cycles + 1;;

    }


}

//reset the state of the simulator - please complete
void sim_ooo::reset(){

	//init instruction log
	init_log();	

	// data memory
	for (unsigned i=0; i<data_memory_size; i++) data_memory[i]=0xFF;
	
	//instr memory
	for (int i=0; i<PROGRAM_SIZE;i++){
		instr_memory[i].opcode=(opcode_t)EOP;
		instr_memory[i].src1=UNDEFINED;
		instr_memory[i].src2=UNDEFINED;
		instr_memory[i].dest=UNDEFINED;
		instr_memory[i].immediate=UNDEFINED;
	}

	//general purpose registers
	for(int i=0;i<int_regs.size();i++)
    {
	    int_regs[i]=UNDEFINED;
    }
	for(int j=0;j<floating_regs.size();j++)
    {
	    floating_regs[j]=UNDEFINED;
    }
	for(int clear_tags=0;clear_tags<NUM_GP_REGISTERS;clear_tags++)
    {
	    floating_point_tags[clear_tags]=UNDEFINED;
	    floating_point_tags[clear_tags]=UNDEFINED;
    }

	//pending_instructions
	pending_instructions.entries.clear();



	//rob
	rob.entries.clear();

	//reservation_stations
	reservation_stations.entries.clear();

	//execution statistics
	clock_cycles = 0;
	instructions_executed = 0;

	//other required initializations
}

/* registers related */

int sim_ooo::get_int_register(unsigned reg){
	return int_regs[reg]; //please modify
}

void sim_ooo::set_int_register(unsigned reg, int value)
{
    int_regs[reg]=value;
}

float sim_ooo::get_fp_register(unsigned reg)
{
	return floating_regs[reg]; //please modify
}

void sim_ooo::set_fp_register(unsigned reg, float value)
{
    floating_regs[reg]=value;
}

unsigned sim_ooo::get_int_register_tag(unsigned reg)
{
	return int_tags[reg]; //please modify
}

unsigned sim_ooo::get_fp_register_tag(unsigned reg)
{

	return floating_point_tags[reg]; //please modify
}

/*
 * The functions definations that I have used
 */

void sim_ooo::issue()
{
 /*Step1: First see how many issues are happening
 * if issue width>1 than its a multiple issue processor
  * than update reservation stations and rob follwoing some rules
  */
 for(unsigned width_of_Issue=0;width_of_Issue<issue_width;width_of_Issue++)
 {
     unsigned PC=(PC_ADDER-instr_base_address)/4;//You can use the offset as an hexa 0x4 to simulate real addressing
     instruction_t IR_issued=instr_memory[PC];//Step2:Get ypur first instruction
     unsigned prevent_PC=PC_ADDER;
//     cout<<IR_issued.opcode<<endl;
    // unsigned bottle_neck_check=PC_ADDER;
     //What happens if the instruCtion is LWS AND SWS

      for(unsigned int i=0;i<reservation_stations.num_entries;i++)
      {
          res_station_t type=reservation_stations.entries[i].type;//debugging
          update_restations_and_rob(i,IR_issued);

      }


    //Increment the adder by 4
     if(prevent_PC==PC_ADDER)
     {
         break;
     }

 }

}
void sim_ooo::execute()
{

    if(clock_cycles>=1)
    {
        unsigned issue_track;//Prevent an issue and excute happening together in same cc
        unsigned execute_track=0;
        for(int i=0;i<reservation_stations.num_entries;i++)
        {
            for (unsigned int k=0; k<pending_instructions.num_entries; k++) {//find instr window entry
                if (pending_instructions.entries[k].pc == reservation_stations.entries[i].pc) {
                    issue_track = pending_instructions.entries[k].issue; //if instruction was issued in the same clk cycle do not proceed to execute
                    execute_track = pending_instructions.entries[k].exe;
                }
            }

                //if reservation station is filled with the instruction and both tags are available
                //prevent UNDEFEINED Undefined pc macthing
                if(reservation_stations.entries[i].pc!=UNDEFINED && reservation_stations.entries[i].tag1==UNDEFINED && reservation_stations.entries[i].tag2==UNDEFINED && issue_track!=clock_cycles && execute_track==UNDEFINED)
                {

                    for(int j=0;j<num_units;j++)
                    {
                        instruction_t current_executed_instruction = instr_memory[(reservation_stations.entries[i].pc - instr_base_address)/4];
                        if(!current_executed_instruction.already_executed)
                            start_execution_operation(i,j,current_executed_instruction);

                    }

                }

        }
    }
}

void sim_ooo::write_back()
{
if(clock_cycles>=2)
{
for(int exec_enter_loop_1=0;exec_enter_loop_1<num_units;exec_enter_loop_1++)
{
    unsigned alu_Calc;
    unsigned int excution_clock_clycle_track;
    for (unsigned int k=0; k<pending_instructions.num_entries; k++){
        if(pending_instructions.entries[k].pc ==exec_units[exec_enter_loop_1].pc)
        {
            unsigned pc=pending_instructions.entries[k].pc;//debug
            unsigned pc_exec=exec_units[exec_enter_loop_1].pc;//debug
            excution_clock_clycle_track= pending_instructions.entries[k].exe;
        }  //if instruction was executed in the same clk cycle do not proceed to write
    }

    if(exec_units[exec_enter_loop_1].pc!=UNDEFINED && exec_units[exec_enter_loop_1].busy==0 && excution_clock_clycle_track!=clock_cycles)
    {
        for(int res_stat_entry_loop_1=0;res_stat_entry_loop_1<reservation_stations.num_entries;res_stat_entry_loop_1++)
        {
            if(exec_units[exec_enter_loop_1].pc==reservation_stations.entries[res_stat_entry_loop_1].pc)
            {
                instruction_t current_instruction = instr_memory[(exec_units[exec_enter_loop_1].pc - instr_base_address)/4];
                switch (current_instruction.opcode)
                {
                    //see book how each values are passed and the alu_func above
                    case ADD:
                        alu_Calc=alu(ADD,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case SUB:
                        alu_Calc=alu(SUB,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case XOR:
                        alu_Calc=alu(XOR,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case AND:
                        alu_Calc=alu(AND,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case MULT:
                        alu_Calc=alu(MULT,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case DIV:
                        alu_Calc=alu(DIV,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case ADDI:
                        alu_Calc=alu(ADDI,reservation_stations.entries[res_stat_entry_loop_1].value1,current_instruction.immediate,0,0);
                        break;
                    case SUBI:
                        alu_Calc=alu(SUBI,reservation_stations.entries[res_stat_entry_loop_1].value1,current_instruction.immediate,0,0);
                        break;
                    case ADDS:
                        alu_Calc=alu(ADDS,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case SUBS:
                        alu_Calc=alu(SUBS,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case MULTS:
                        alu_Calc=alu(MULTS,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;
                    case DIVS:
                        alu_Calc=alu(DIVS,reservation_stations.entries[res_stat_entry_loop_1].value1,reservation_stations.entries[res_stat_entry_loop_1].value2,0,0);
                        break;

                        case LWS:
                    case LW:
                        alu_Calc = char2unsigned(data_memory + reservation_stations.entries[res_stat_entry_loop_1].address);
                        break;

                    case SW:
                    case SWS://DO nothing
                        break;

                    case BEQZ:
                        if(reservation_stations.entries[res_stat_entry_loop_1].value1==0) alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4 +current_instruction.immediate;
                        else alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4;//Fall through
                        break;
                    case BNEZ:
                        if(reservation_stations.entries[res_stat_entry_loop_1].value1!=0) alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4 +current_instruction.immediate;
                        else alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4;//Fall through
                        break;
                    case BLTZ:
                        if(reservation_stations.entries[res_stat_entry_loop_1].value1<0) alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4 +current_instruction.immediate;
                        else alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4;//Fall through
                        break;
                    case BGTZ:
                        if(reservation_stations.entries[res_stat_entry_loop_1].value1>0) alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4 +current_instruction.immediate;
                        else alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4;//Fall through
                        break;
                    case BLEZ:
                        if(reservation_stations.entries[res_stat_entry_loop_1].value1<=0) alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4 +current_instruction.immediate;
                        else alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4;//Fall through
                        break;
                    case BGEZ:
                        if(reservation_stations.entries[res_stat_entry_loop_1].value1>=0) alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4 +current_instruction.immediate;
                        else alu_Calc = reservation_stations.entries[res_stat_entry_loop_1].pc+0x4;//Fall through
                        break;
                    default:
                        break;

                }
                //update rob
                for(int rob_index=0;rob_index<rob.num_entries;rob_index++)
                {
                    if(rob.entries[rob_index].pc==exec_units[exec_enter_loop_1].pc)
                    {
                        rob.entries[rob_index].value = alu_Calc;
                        rob.entries[rob_index].state = WRITE_RESULT;
                        rob.entries[rob_index].ready = true;


                        for (unsigned int res_entry_loop_2 = 0; res_entry_loop_2 < reservation_stations.num_entries; res_entry_loop_2++) {
                            if (reservation_stations.entries[res_entry_loop_2].tag1 == rob_index) {

                                reservation_stations.entries[res_entry_loop_2].value1 = alu_Calc;
                                reservation_stations.entries[res_entry_loop_2].tag1 = UNDEFINED;


                            }
                            if (reservation_stations.entries[res_entry_loop_2].tag2 == rob_index) {

                                reservation_stations.entries[res_entry_loop_2].value2 = alu_Calc;
                                reservation_stations.entries[res_entry_loop_2].tag2 = UNDEFINED;

                            }

                        }
                    }
                }


                for(int instruction_window_index=0;instruction_window_index<=pending_instructions.num_entries;instruction_window_index++)
                {
                    if(pending_instructions.entries[instruction_window_index].pc==exec_units[exec_enter_loop_1].pc)
                    {
                        pending_instructions.entries[instruction_window_index].wr=clock_cycles;

                    }
                }
                //res_station_entry_t clear_entry = reservation_stations.entries[res_stat_entry_loop_1];
               // reservation_stations.entries[i].value1=UNDEFINED;
                //reservation_stations.entries[i].destination=UNDEFINED;
                clean_res_station(res_stat_entry_loop_1);
               //print_reservation_stations();
                exec_units[exec_enter_loop_1].pc=UNDEFINED;
                break;

            }
            }
        }
        //update Rob and clear reservation station entry


}

}
}

void sim_ooo::commit() {

    if (clock_cycles >= 3) {


        unsigned commit_cycle_track;
        unsigned commit_per_cc = 0;
        bool flush= false;
        for (unsigned i = 0; i < rob.num_entries; i++)
        {
            //get the current instruction

            //first we need to ensure that if writeback and commit takes place in the same cc it must wait for 1 cc
            for (unsigned int j = 0; j < pending_instructions.num_entries; j++)
            {
                if (pending_instructions.entries[j].pc == rob.entries[i].pc)
                {
                    commit_cycle_track = pending_instructions.entries[j].wr;
                }
            }
            //Start commiting and flushing now
            if (i == Rob_head && commit_cycle_track != clock_cycles && commit_per_cc==0) {
                if (rob.entries[i].ready) {
                    instruction_t Ir_current=instr_memory[(rob.entries[i].pc-instr_base_address)/4];
//                    if(!flush_protocol_activated &&run_cycles && Ir_current.opcode==EOP)
//                    {
//                        run_cycles=false;
//                            break;
//                    }
                    switch (Ir_current.opcode)
                    {
                        case ADD:
                        case ADDI:
                        case SUB:
                        case SUBI:
                        case LW:
                        case MULT:
                        case XOR:
                        case DIV:
                        case AND: {
                            set_int_register(rob.entries[i].destination, rob.entries[i].value);
                            //Dont forget to clear the tags
                            if(get_int_register_tag(rob.entries[i].destination) == i)
                            {
                                int_tags[rob.entries[i].destination] = UNDEFINED;
                            }
                            break;
                        }
                        case ADDS:
                        case SUBS:
                        case MULTS:
                        case DIVS:
                        case LWS: {
                            set_fp_register(rob.entries[i].destination - NUM_GP_REGISTERS,
                                            unsigned2float(rob.entries[i].value));
                            if (get_fp_register_tag(rob.entries[i].destination - NUM_GP_REGISTERS) == i) {
                                floating_point_tags[rob.entries[i].destination - NUM_GP_REGISTERS] = UNDEFINED;

                            }
                            break;

                        }
                        case SW:
                        case SWS:
                            //As told by the professor we dont have to handle memory bypassing operations right now
                            //so break
                            break;
                        case BEQZ:
                        case BGEZ:
                        case BLEZ:
                        case BGTZ:
                        case BLTZ:
                        case BNEZ:
                            {
                            //Step 1 -check if branch is mispredicted or not
                            if (rob.entries[i].value != rob.entries[i].pc + 0x4) {
                                flush = true;
                            }
                                break;
                        }
                        default:break;


                    }
                    if(flush)
                    {
                        follow_flush_protocol(i);
                        flush_protocol_activated=true;
                        commit_per_cc=1;
                        instructions_executed=instructions_executed+1;
                    }
                    else {
                        //fall through protocol
                        flush_protocol_activated= false;
                        for (unsigned int pendng = 0; pendng < pending_instructions.num_entries; pendng++) {
                            if (pending_instructions.entries[pendng].pc == rob.entries[i].pc) {
                                pending_instructions.entries[pendng].commit = clock_cycles;

                                commit_to_log(pending_instructions.entries[pendng]);

                                //clear pending instructions
                                pending_instructions.entries[pendng].pc = UNDEFINED;
                                pending_instructions.entries[pendng].wr = UNDEFINED;
                                pending_instructions.entries[pendng].exe = UNDEFINED;
                                pending_instructions.entries[pendng].issue = UNDEFINED;
                                pending_instructions.entries[pendng].commit = UNDEFINED;

                            }
                        }
                        instructions_executed=instructions_executed+1;
                        for(unsigned int res=0;res<reservation_stations.num_entries;res++)
                        {
                            if(reservation_stations.entries[res].tag1==i)
                            {
                                reservation_stations.entries[res].value1=rob.entries[i].value;
                                reservation_stations.entries[res].tag1=UNDEFINED;
                            }
                            if(reservation_stations.entries[res].tag2==i)
                            {
                                reservation_stations.entries[res].value2=rob.entries[i].value;
                                reservation_stations.entries[res].tag2=UNDEFINED;
                            }
                        }
                        //clear ROB
                        rob_entry_t clear=rob.entries[i];
                        clean_rob(i);
                        commit_per_cc=1;
                        //move rob forward from discussions
                        if(Rob_head==rob.num_entries-1)
                        {
                            Rob_head=0;
                        }
                        else
                        {
                            Rob_head++;
                            //print_reservation_stations();
                           // print_pending_instructions();
                        }


                    }

                }
            }
        }
    }
}

/**
 * Utility functions to check for opcodes
 */
bool sim_ooo::is_Integer_Load_and_Store(opcode_t op)
{
    switch (op)
    {
        case LW:
        case SW:return true;
        default:return false;

    }
}

bool sim_ooo::is_floating_point_operations_add_sub(opcode_t op) {
    switch(op)
    {
        case ADDS:
        case SUBS:return true;
        default:return false;
    }
}

bool sim_ooo::is_floating_point_operations_mult_div(opcode_t op)
{
    switch(op)
    {
        case MULTS:
        case DIVS:return true;
        default:return false;
    }

}
bool sim_ooo::is_Integer_operation(opcode_t op)
{
    switch(op)
    {
        case ADD:
        case SUB:
        case XOR:
        case DIV:
        case BEQZ:
        case BGTZ:
        case BLEZ:
        case BNEZ:
        case BLTZ:
        case BGEZ:
        case ADDI:
        case JUMP:
        case AND:
        case MULT:
        case SUBI:return true;
        default:return false;

    }
}

bool sim_ooo::is_floating_load_and_store(opcode_t op)
{
    switch(op)
    {
        case LWS:
        case SWS:return true;
        default:return false;
    }
}

bool sim_ooo::just_loads(opcode_t op)
{
    switch(op)
    {
        case LWS:
        case LW:return true;
        default:return false;
    }
}

bool sim_ooo::just_store(opcode_t op)
{
    switch(op)
    {
        case SWS:
        case SW:return true;
        default:return false;
    }
}

bool sim_ooo::just_Division(opcode_t op)
{
    switch(op)
    {
        case DIVS:return true;
        default:return false;
    }
}

bool sim_ooo::just_Mult(opcode_t op)
{
    switch(op)
    {
        case MULTS:return true;
        default:return false;
    }
}
/*
 * This functions helps to check whther a rob or a reservation station is free or not
 * it also implements the concept of circular queue in Rob
 * References for this issue especially circular queue are taken from
 * https://stackoverflow.com/questions/59097741/reorder-buffer-pointer-values-when-it-is-full
 */
void sim_ooo::update_restations_and_rob( unsigned res_station_entry,instruction_t& IR_issued) {
///////CASE LWS AND SWS////////////////////////
    bool iterate_rob_again=false;
    if (reservation_stations.entries[res_station_entry].pc == UNDEFINED &&
        reservation_stations.entries[res_station_entry].type == LOAD_B && !IR_issued.already_issued && is_floating_load_and_store(IR_issued.opcode)) {

        //There is an empty res-station available find an empty ROB now
        for (unsigned int j = Rob_head; j < rob.num_entries; j++)
        {
            if (rob.entries[j].pc == UNDEFINED)//Free entry available
            {
                reservation_stations.entries[res_station_entry].pc = PC_ADDER;
                IR_issued.already_issued = true;
                if (IR_issued.src1 != UNDEFINED)//src1 is present
                {
                    //TODO find a better way to do this
                    //Check the rename map table
                    if (get_int_register_tag(IR_issued.src1 == UNDEFINED)) {
                        //no mapping available
                        reservation_stations.entries[res_station_entry].value1 = get_int_register(IR_issued.src1);
                        reservation_stations.entries[res_station_entry].tag1 = UNDEFINED;
                    } else {
                        reservation_stations.entries[res_station_entry].value1 = UNDEFINED;
                        reservation_stations.entries[res_station_entry].tag1 = get_int_register_tag(IR_issued.src1);
                    }

                }

                if (IR_issued.src2 != UNDEFINED) {//src2 present
                    if (get_int_register_tag(IR_issued.src2) == UNDEFINED) {
                        reservation_stations.entries[res_station_entry].value2 = (get_int_register(IR_issued.src2));
                        reservation_stations.entries[res_station_entry].tag2 = UNDEFINED;
                    } else {//tag entry present for second src reg
                        reservation_stations.entries[res_station_entry].value2 = UNDEFINED;
                        reservation_stations.entries[res_station_entry].tag2 = get_int_register_tag(IR_issued.src2);
                    }
                }

                reservation_stations.entries[res_station_entry].destination = j;
                //Now loads are done its time for store

                reservation_stations.entries[res_station_entry].address = IR_issued.immediate;

                //update RMT
                if (IR_issued.dest != UNDEFINED)
                {
                    floating_point_tags[IR_issued.dest] = j;
                    rob.entries[j].destination = IR_issued.dest + NUM_GP_REGISTERS;
                }
                //update ROB
                rob.entries[j].pc = PC_ADDER;
                rob.entries[j].state = ISSUE;
                rob.entries[j].value = UNDEFINED;
                rob.entries[j].ready=false;
                pending_instructions.entries[j].pc = PC_ADDER;
                pending_instructions.entries[j].issue = clock_cycles;
                PC_ADDER=PC_ADDER+4;
                break;
            }
            if(Rob_head && j==rob.num_entries-1 &&!iterate_rob_again)
            {
                j=-1;
                iterate_rob_again=true;//need to do it only once
                //Circular queue implementation not needed anymore
                //thanks to https://stackoverflow.com/questions/59097741/reorder-buffer-pointer-values-when-it-is-full
            }
        }
    }
        ///////CASE LWS AND SWS FINISHED/////////////
        ///Case INTEGRS
    if (reservation_stations.entries[res_station_entry].pc == UNDEFINED &&
             reservation_stations.entries[res_station_entry].type == INTEGER_RS && !IR_issued.already_issued && is_Integer_operation(IR_issued.opcode)) {
        for (unsigned int j = Rob_head; j < rob.num_entries; j++) {
            //Lets check if there is a Rob available
            if (rob.entries[j].pc == UNDEFINED) {
                reservation_stations.entries[res_station_entry].pc = PC_ADDER;
                IR_issued.already_issued = true;

                    if (get_int_register_tag(IR_issued.src1) ==UNDEFINED)//if src1 in RMT is undefined try to get the value from register files
                    {
                        reservation_stations.entries[res_station_entry].value1 = get_int_register(IR_issued.src1);
                        reservation_stations.entries[res_station_entry].tag1 = UNDEFINED;
                    }

                    else {
                        if (rob.entries[get_int_register_tag(IR_issued.src1)].value ==UNDEFINED)//if undefined try the tag or value from rob entry //TODO try to optimise this more
                        {
                            reservation_stations.entries[res_station_entry].value1 = UNDEFINED;
                            reservation_stations.entries[res_station_entry].tag1 = get_int_register_tag(IR_issued.src1);
                        } else {
                            //when both are present
                            reservation_stations.entries[res_station_entry].value1 = rob.entries[get_int_register_tag(
                                    IR_issued.src1)].value;//else get value
                        }
                    }

                if (IR_issued.src2 !=
                    UNDEFINED) ///Only for Branches jump is not handled because no testcases has JUMP in them
                {
                    if (get_int_register_tag(IR_issued.src2) ==
                        UNDEFINED)//src2 is avaliable and no tag is presnet in RMT
                    {
                        reservation_stations.entries[res_station_entry].value2 = get_int_register(IR_issued.src2);
                        reservation_stations.entries[res_station_entry].tag2 = UNDEFINED;
                    } else {
                        if (rob.entries[get_int_register_tag(IR_issued.src2)].value ==
                            UNDEFINED)//no rob entry present
                        {
                            reservation_stations.entries[res_station_entry].value2 = UNDEFINED;
                            reservation_stations.entries[res_station_entry].tag2 = get_int_register_tag(IR_issued.src2);
                        } else {
                            //when both are present
                            reservation_stations.entries[res_station_entry].value2 = rob.entries[get_int_register_tag(
                                    IR_issued.src2)].value;
                        }
                    }
                }
                //update Resstation
                reservation_stations.entries[res_station_entry].destination = j;
                reservation_stations.entries[res_station_entry].address = UNDEFINED;//no address other than Lw.lws sw and sws
                if (IR_issued.dest != UNDEFINED) {
                    int_tags[IR_issued.dest] = j;//update tags with rob entry
                }
                //update rob and instruction window
                rob.entries[j].pc = PC_ADDER;
                rob.entries[j].state = ISSUE;
                rob.entries[j].ready=false;
                if (IR_issued.dest != UNDEFINED) rob.entries[j].destination = IR_issued.dest;
                rob.entries[j].value = UNDEFINED;
                pending_instructions.entries[j].pc = PC_ADDER;
                pending_instructions.entries[j].issue = clock_cycles;
                PC_ADDER=PC_ADDER+4;
                break;
            }
            if(Rob_head && j==rob.num_entries-1 &&!iterate_rob_again)
            {
                j=-1;
                iterate_rob_again=true;
            }
        }
    }
    ///Case Multiplications and Divisions
     if (reservation_stations.entries[res_station_entry].pc == UNDEFINED &&
               reservation_stations.entries[res_station_entry].type == MULT_RS && !IR_issued.already_issued && is_floating_point_operations_mult_div(IR_issued.opcode)) {
        for (unsigned int j = Rob_head; j < rob.num_entries; j++) {
            //Check for empty rob entry
            if (rob.entries[j].pc == UNDEFINED) {
                reservation_stations.entries[res_station_entry].pc = PC_ADDER;
                IR_issued.already_issued = true;
                if(get_fp_register_tag(IR_issued.src1)==UNDEFINED)//if no tags present read from the register
                {
                    reservation_stations.entries[res_station_entry].value1=float2unsigned(get_fp_register(IR_issued.src1));//TODO expliciltly converting to unsigned not working
                    reservation_stations.entries[res_station_entry].tag1=UNDEFINED;
                }
                else { //else try to get the value / tag from rob entry
                    if (rob.entries[get_fp_register_tag(IR_issued.src1)].value == UNDEFINED)//rob tag is not present
                    {
                        reservation_stations.entries[res_station_entry].value1 = UNDEFINED;
                        reservation_stations.entries[res_station_entry].tag1 = get_fp_register_tag(IR_issued.src1);
                    }
                    else
                    {
                        reservation_stations.entries[res_station_entry].value1 = rob.entries[get_fp_register_tag(
                                IR_issued.src1)].value;
                    }
                }


                if(get_fp_register_tag(IR_issued.src2)==UNDEFINED)//No tags present no raw
                {
                    reservation_stations.entries[res_station_entry].value2=float2unsigned(get_fp_register(IR_issued.src2));//TODO expliciltly converting to unsigned not working
                    reservation_stations.entries[res_station_entry].tag2=UNDEFINED;
                }
                else {
                    if (rob.entries[get_fp_register_tag(IR_issued.src2)].value == UNDEFINED)//rob tag is not present
                    {
                        reservation_stations.entries[res_station_entry].value2 = UNDEFINED;
                        reservation_stations.entries[res_station_entry].tag2 = get_fp_register_tag(IR_issued.src2);
                    }
                    else
                    {
                        reservation_stations.entries[res_station_entry].value2 = rob.entries[get_fp_register_tag(
                                IR_issued.src2)].value;
                    }
                }

                reservation_stations.entries[res_station_entry].destination = j;
                reservation_stations.entries[res_station_entry].address = UNDEFINED;//no address other than Lw.lws sw and sws
                if (IR_issued.dest != UNDEFINED) {
                    floating_point_tags[IR_issued.dest] = j;//update tags with rob entry
                }
                rob.entries[j].pc = PC_ADDER;
                rob.entries[j].state = ISSUE;
                rob.entries[j].ready=false;
                if (IR_issued.dest != UNDEFINED) rob.entries[j].destination = IR_issued.dest + NUM_GP_REGISTERS;
                rob.entries[j].value = UNDEFINED;
                pending_instructions.entries[j].pc = PC_ADDER;
                pending_instructions.entries[j].issue = clock_cycles;
                PC_ADDER=PC_ADDER+4;
                break;
            }
            if(Rob_head && j==rob.num_entries-1 &&!iterate_rob_again)
            {
                j=-1;
                iterate_rob_again=true;
            }
        }

    }
    ///Case Multiplication and divison end
    //TODO a segmentation bug when reading the data memory remains cant run valgrind on M1
    ///case Floating Adders

    if(reservation_stations.entries[res_station_entry].pc==UNDEFINED && reservation_stations.entries[res_station_entry].type==ADD_RS && !IR_issued.already_issued && is_floating_point_operations_add_sub(IR_issued.opcode))
    {
        for (unsigned int j = Rob_head; j < rob.num_entries; j++) {
            //Check for empty rob entry
            if (rob.entries[j].pc == UNDEFINED) {
                reservation_stations.entries[res_station_entry].pc = PC_ADDER;
                IR_issued.already_issued = true;
                if(get_fp_register_tag(IR_issued.src1)==UNDEFINED)//No tags present no raw
                {
                    reservation_stations.entries[res_station_entry].value1=float2unsigned(get_fp_register(IR_issued.src1));//TODO expliciltly converting to unsigned not working
                    reservation_stations.entries[res_station_entry].tag1=UNDEFINED;
                }
                else {
                    if (rob.entries[get_fp_register_tag(IR_issued.src1)].value == UNDEFINED)//rob value is not present
                    {
                        reservation_stations.entries[res_station_entry].value1 = UNDEFINED;
                        reservation_stations.entries[res_station_entry].tag1 = get_fp_register_tag(IR_issued.src1);
                    }
                    else
                    {
                        reservation_stations.entries[res_station_entry].value1 = rob.entries[get_fp_register_tag(
                                IR_issued.src1)].value;
                    }
                }


                if(get_fp_register_tag(IR_issued.src2)==UNDEFINED)//No tags present no raw
                {
                    reservation_stations.entries[res_station_entry].value2=float2unsigned(get_fp_register(IR_issued.src2));//TODO expliciltly converting to unsigned not working
                    reservation_stations.entries[res_station_entry].tag2=UNDEFINED;
                }
                else {
                    if (rob.entries[get_fp_register_tag(IR_issued.src2)].value == UNDEFINED)//rob tag is not present
                    {
                        reservation_stations.entries[res_station_entry].value2 = UNDEFINED;
                        reservation_stations.entries[res_station_entry].tag2 = get_fp_register_tag(IR_issued.src2);
                    } else {
                        reservation_stations.entries[res_station_entry].value2 = rob.entries[get_fp_register_tag(
                                IR_issued.src2)].value;
                    }
                }
                reservation_stations.entries[res_station_entry].destination = j;
                reservation_stations.entries[res_station_entry].address = UNDEFINED;//no address other than Lw.lws sw and sws
                floating_point_tags[IR_issued.dest] = j;//update tags with rob entry
                rob.entries[j].pc = PC_ADDER;
                rob.entries[j].state = ISSUE;
                rob.entries[j].ready=false;
                rob.entries[j].destination = IR_issued.dest + NUM_GP_REGISTERS;
                rob.entries[j].value = UNDEFINED;
                pending_instructions.entries[j].pc = PC_ADDER;
                pending_instructions.entries[j].issue = clock_cycles;
                PC_ADDER=PC_ADDER+4;
                //print_reservation_stations();
                break;

            }
            if(Rob_head && j==rob.num_entries-1 &&!iterate_rob_again)
            {
                j=-1;
                iterate_rob_again=true;
            }
        }


    }

}
/*
 * This function just starts the excution units and as every execution units has different latencies they are excuted in a different way
 */
void sim_ooo::start_execution_operation(unsigned res_stat_entry, unsigned execution_stat_entry, instruction_t &Curren_in)
{
    //lw and lws
    //We are not calculating in the exceute stage because tracking the values isbecoming too confusing instead we will do it in writeback stage as by book
    if(exec_units[execution_stat_entry].type==MEMORY && exec_units[execution_stat_entry].busy==0 && exec_units[execution_stat_entry].pc==UNDEFINED && !Curren_in.already_executed  && just_loads(Curren_in.opcode))//found a free execution unit and are not Integer or floating point operations and are just loads or load
    {
        //calculate addresss the main job of load
        reservation_stations.entries[res_stat_entry].address += reservation_stations.entries[res_stat_entry].value1;
        exec_units[execution_stat_entry].busy=exec_units[execution_stat_entry].latency;//This is a better way to keep the unit busy instead of adding a latency and stalling it sourced from github links of tomasulo in java
        exec_units[execution_stat_entry].pc=reservation_stations.entries[res_stat_entry].pc;//copy pc of execution units to reservation stations
        //update rob entries
        for(int rob_index=0;rob_index<rob.num_entries;rob_index++)
        {
            if  (rob.entries[rob_index].pc==exec_units[execution_stat_entry].pc)//update for only that instruction_index in rob when the pc is matching
                rob.entries[rob_index].state=EXECUTE;

        }
        for(int instruction_index=0;instruction_index<pending_instructions.num_entries;instruction_index++)
        {
            if(pending_instructions.entries[instruction_index].pc==exec_units[execution_stat_entry].pc)
            {
                pending_instructions.entries[instruction_index].exe=clock_cycles;
            }

        }
        Curren_in.already_executed= true;
        return;

    }
    //Floating ADDS and SUBS
    if(exec_units[execution_stat_entry].type==ADDER && exec_units[execution_stat_entry].busy==0 && exec_units[execution_stat_entry].pc==UNDEFINED && !Curren_in.already_executed && is_floating_point_operations_add_sub(Curren_in.opcode))
    {
        //Wait for the latency
        exec_units[execution_stat_entry].busy=exec_units[execution_stat_entry].latency;//just because of this the code is so much longer TODO optimise this
        exec_units[execution_stat_entry].pc=reservation_stations.entries[res_stat_entry].pc;
        for(int rob_index=0;rob_index<rob.num_entries;rob_index++)
        {
            if(rob.entries[rob_index].pc==exec_units[execution_stat_entry].pc)
            {
                rob.entries[rob_index].state=EXECUTE;
            }
        }
        for(int instruction_index=0;instruction_index<pending_instructions.num_entries;instruction_index++)
        {
            if(pending_instructions.entries[instruction_index].pc==exec_units[execution_stat_entry].pc)
            {
                pending_instructions.entries[instruction_index].exe=clock_cycles;
            }
        }
        Curren_in.already_executed= true;
        return;
    }
    if(exec_units[execution_stat_entry].type==DIVIDER && exec_units[execution_stat_entry].busy==0 && exec_units[execution_stat_entry].pc==UNDEFINED && !Curren_in.already_executed && just_Division(Curren_in.opcode))
    {
        //Wait for the latency
        exec_units[execution_stat_entry].busy=exec_units[execution_stat_entry].latency;
        exec_units[execution_stat_entry].pc=reservation_stations.entries[res_stat_entry].pc;
        for(int rob_index=0;rob_index<rob.num_entries;rob_index++)
        {
            if(rob.entries[rob_index].pc==exec_units[execution_stat_entry].pc)
            {
                rob.entries[rob_index].state=EXECUTE;
            }
        }
        for(int instruction_index=0;instruction_index<pending_instructions.num_entries;instruction_index++)
        {
            if(pending_instructions.entries[instruction_index].pc==exec_units[execution_stat_entry].pc)
            {
                pending_instructions.entries[instruction_index].exe=clock_cycles;
            }
        }
        Curren_in.already_executed= true;
        return;
    }

    if(exec_units[execution_stat_entry].type==MULTIPLIER && exec_units[execution_stat_entry].busy==0 && exec_units[execution_stat_entry].pc==UNDEFINED && !Curren_in.already_executed && just_Mult(Curren_in.opcode))
    {
        //Wait for the latency
        exec_units[execution_stat_entry].busy=exec_units[execution_stat_entry].latency;
        exec_units[execution_stat_entry].pc=reservation_stations.entries[res_stat_entry].pc;
        for(int rob_index=0;rob_index<rob.num_entries;rob_index++)
        {
            if(rob.entries[rob_index].pc==exec_units[execution_stat_entry].pc)
            {
                rob.entries[rob_index].state=EXECUTE;
            }
        }
        for(int instruction_index=0;instruction_index<pending_instructions.num_entries;instruction_index++)
        {
            if(pending_instructions.entries[instruction_index].pc==exec_units[execution_stat_entry].pc)
            {
                pending_instructions.entries[instruction_index].exe=clock_cycles;
            }
        }
        Curren_in.already_executed= true;
        return;
    }

    if(exec_units[execution_stat_entry].type==INTEGER && exec_units[execution_stat_entry].busy==0 && exec_units[execution_stat_entry].pc==UNDEFINED && !Curren_in.already_executed && is_Integer_operation(Curren_in.opcode))
    {
        //Wait for the latency
        exec_units[execution_stat_entry].busy=exec_units[execution_stat_entry].latency;
        exec_units[execution_stat_entry].pc=reservation_stations.entries[res_stat_entry].pc;
        for(int rob_index=0;rob_index<rob.num_entries;rob_index++)
        {
            if(rob.entries[rob_index].pc==exec_units[execution_stat_entry].pc)
            {
                rob.entries[rob_index].state=EXECUTE;
            }
        }
        for(int instruction_index=0;instruction_index<pending_instructions.num_entries;instruction_index++)
        {
            if(pending_instructions.entries[instruction_index].pc==exec_units[execution_stat_entry].pc)
            {
                pending_instructions.entries[instruction_index].exe=clock_cycles;
            }
        }
        Curren_in.already_executed= true;
        return;
    }
    //Only store no latency in exec unit dont know why but matches testcase //TODO ask TA
    if(just_store(Curren_in.opcode)  && !Curren_in.already_executed)
    {
        reservation_stations.entries[res_stat_entry].address = reservation_stations.entries[res_stat_entry].address+float2unsigned( get_fp_register(Curren_in.src1));
        Curren_in.already_executed=true;
        return;

    }
}
/*
 * This function implements the flush protocl
 */
void sim_ooo::follow_flush_protocol(unsigned  rob_index)
{
    unsigned iterate_rob_again= false;
            //from stackoverflow answers whenever you are iterating from the Rob_head it may be set to 2 or 3 after commit stage and during
            //the issue stage there still maybe rob blocks empty above the new value of Rob_head and you need to iterate them again
    PC_ADDER=rob.entries[rob_index].value;
    for(unsigned int i=Rob_head;i<pending_instructions.num_entries;i++)
    {
        if(pending_instructions.entries[i].pc!=UNDEFINED)//instructions present
        {

            if(pending_instructions.entries[i].pc ==rob.entries[rob_index].pc)
            {
                //flush them
                pending_instructions.entries[i].commit = clock_cycles;

            }
            commit_to_log(pending_instructions.entries[i]);
            pending_instructions.entries[i].commit=UNDEFINED;
            pending_instructions.entries[i].issue=UNDEFINED;
            pending_instructions.entries[i].exe=UNDEFINED;
            pending_instructions.entries[i].pc=UNDEFINED;
            pending_instructions.entries[i].wr=UNDEFINED;
            //need to iterate again
        }
        if(i==pending_instructions.num_entries-1 && !iterate_rob_again)
        {
            i=-1;
            iterate_rob_again=true;
        }
    }
    for(unsigned int j=0;j<rob.num_entries;j++)
    {
//        rob_entry_t clear_entry=rob.entries[j];
        clean_rob(j);
    }
    for(unsigned int m=0;m<reservation_stations.num_entries;m++)
    {
//        res_station_entry_t clear_entry_res=reservation_stations.entries[m];
        clean_res_station(m);
    }
    //clear RMT/tags
    for(unsigned int i=0;i<NUM_GP_REGISTERS;i++)
    {
        int_tags[i]=UNDEFINED;
        floating_point_tags[i]=UNDEFINED;
    }
    for(unsigned int exec_entry=0;exec_entry<num_units;exec_entry++)//reset the excution entry of the flush instructions
    {
        exec_units[exec_entry].pc=UNDEFINED;
        exec_units[exec_entry].busy=0;
    }
    Rob_head=0;

}