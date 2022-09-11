Data Structures Used

@Rob Buffer_entries(vectors)-It took me two days to figure the easiest implementation of Rob buffer. After much discussions on git issues page of some git implementation and going through stack overflow.
Previously I was trying to use pointers and having a circular queue(not even properly implemented) for head and tail.
Now I use the concept of advancing the head and reiterate the rob every time.
Rnd on every issue I will reiterate the rob_buffer once so that the empty cells above Rob Heads(1 &2 where 2 is the so called “tail”) are also filled.
Advance the rob head when the instruction is committed

@reservation stations_entries(vectors) - All the reservations station entries are now vectors instead of pointer to arrays of structs for easier debugging

@pending instructions_entries(vectors)-These are also vectors and the concept of reiteration is also used here as in Rob in commit stage

@excution_unit_entries(arrays)-Already implemented from before

@instruction_window_t(structs)-has the following properties
unsigned src2; //second source register in the assembly instruction
unsigned dest; //destination register
unsigned immediate; //immediate field
string label;parsing
bool already_issued
bool already_executed
bool already_written//not used
bool already_commited .//not used

@executiion_units(structs):
exe_unit_t type;  // execution unit type
unsigned latency; // execution unit latency
unsigned busy;  
unsigned pc;
@instr_window_entry_t(structs);
unsigned pc
unsigned issue
unsigned exe
unsigned wr
unsigned commit
@rob_entry_t(struct)
bool ready=false
unsigned pc=UNDEFINED;  	
stage_t state;//Issue by default
unsigned destination
unsigned value

@res_station_type(struct):
res_station_t type; // reservation station type
unsigned name;	   
unsigned pc
unsigned value1
unsigned value2
unsigned tag1
unsigned tag2
unsigned destination
unsigned address



Functions Used:

@issue()->This function helps to issue the instruction in the clock cycle
@update_reservation_station_and_rob(res_stationEntry,rob entry,bool already issues):This functions fills the res_station and rob by using PC as map tool for instrcutions 	 
following the steps are followed:
Find empty res_station-check if the instruction is not already issued and no other instruction is issued in same cc
Find empty rob //see comments on how to fill it
Fill rob
If not filled from head reiterate just to be sure no empty spots above rob head
If instruction. Is not issued prevent PC from progressing

@execute->This function helps to excite the instruction by sending them through different execution station only if the instruction is not already excused
@start_excution_Operations(res_station_entry,execunuts,bool already_executed)
Uses the PC as mapping variable to map the instruction from res-station To Rob entry
Fills exec stations only if its not busy		
@write_back-Writes the instruction to rob
Flush_protocol-Follows flush protocol of emptying the rob and res_station if branch is mispredicted.

@commit-commits from rob to register files
@run-runs the core of the simulator-it will complete all the instructions if cycle==0 using the runcycle concept as in project1

Run the program
@grendel makefile
cd project2_code
cd c++
make
./bin/testcase1
./bin/testcase2
:
:
:
./bin/testcase10