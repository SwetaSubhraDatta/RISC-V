#include <iostream>
#include <vector>
#include "math.h"
#include <fstream>

#define UNDEFINED 0xFFF
using namespace std;

typedef enum
{
    HIT,
    MISS,

}hit_miss_policy_t;

typedef enum
{
    R_L1_HIT,
    W_L1_HIT,
    L1_MISS_READ,
    L1_MISS_WRITE,
    R_L1_MISS_BUT_VICTIM_HIT_SWAPPED,
    R_L1_MISS_L1_EMPTY_REPLACED,
    R_L1_FULL_VICTIM_EMPTY_EVICTED2VICTIM_L1_REPLACED,
    R_L1_FULL_VICTIM_FULL_EVICTEDFROMVICTIM_L1_REPLACED,

    W_L1_MISS_BUT_VICTIM_HIT_SWAPPED,
    W_L1_MISS_L1_EMPTY_REPLACED,
    W_L1_FULL_VICTIM_EMPTY_EVICTED2VICTIM_L1_REPLACED,
    W_L1_FULL_VICTIM_FULL_EVICTEDFROMVICTIM_L1_REPLACED

}hit_miss_policy_L1V;


class cache_sim
        {
        public:
            ofstream outfile;
            unsigned memory_traffic=0;
            unsigned L1_reads=0;
            unsigned L1_writes=0;
            unsigned L1_read_misses=0;
            unsigned L1_writes_misses=0;
            unsigned swap_requests=0;
            unsigned swap_request_rate=0;
            unsigned swaps=0;
            float L1_VC_miss_rate=0;
            //unsigned writebacks_L1_VC=0;
            unsigned L2_reads=0;
            unsigned L2_read_misses=0;
            unsigned L2_writes=0;
            unsigned L2_writes_misses=0;
            unsigned L2_miss_rate=0;
            unsigned writebacks_L2=0;
            unsigned total_memory_traffic=0;


    unsigned evict_count=0;
    unsigned int lru_counter=0;
    unsigned victim_block_size=UNDEFINED;
    //Victim-Cache
    typedef struct L1_block //L1 properties
    {
        unsigned valid_bit=0;
        unsigned tag=UNDEFINED;
        unsigned dirty_bit=0;
        unsigned LRU=0;
        unsigned address=UNDEFINED;
        unsigned index=UNDEFINED;

    }L1;

    hit_miss_policy_L1V L1_V_status;
    vector<vector<L1_block>> L1_Cachetable;///Defining the 2D array
    vector<L1_block> victim_Cache;
    unsigned sets_l1;
    unsigned sets_l2;
    unsigned l1_assoc;
    unsigned l2_assoc;
    unsigned cache_size_l1;
    unsigned cache_size_l2;






    public:
    cache_sim(unsigned long int associativity,unsigned long int l1_size,unsigned long int block_size,unsigned VC_NUM_blocks);
    cache_sim(unsigned assoc_1,unsigned assoc_2, unsigned  l1_size,unsigned  L2_size,unsigned block_size);
    unsigned long get_tag(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity, unsigned long int block_size);
    unsigned long get_offset(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity, unsigned long int block_size);
    unsigned long get_index(unsigned int address,unsigned long int cache_l1_size,unsigned long int associativity,unsigned long int block_size);
    unsigned evict_blocK(unsigned index,unsigned long int associativity);
    bool Cache_index_is_Full(unsigned index,unsigned long int associativity);
    hit_miss_policy_L1V L1_Victim_read_cache(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity, unsigned long int block_size);
    hit_miss_policy_L1V L1_Victim_write_cache(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity, unsigned long int block_size);
    hit_miss_policy_t read_Cache(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity, unsigned long int block_size);
    //hit_miss_policy_t read_Cache(unsigned address,unsigned long cache_l1_size,int associativity,long block_size);
    void print_CacheTable(unsigned long int rows,unsigned long int columns);
    hit_miss_policy_t write_Cache(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity, unsigned long int block_size);
    void sort_Victim();
    bool no_empty_blocks_in_victim();

    //void print_t_CacheTable(unsigned  long associativity,unsigned long sets );

    void swap_with_victim(unsigned index,unsigned assoc,unsigned victim_block);
    unsigned evict_block_from_victim();



};


 class cache_params
         {
//global variables
public:
    unsigned long int block_size;
    unsigned long int l1_size;
    unsigned long int l1_assoc;
    unsigned long int vc_num_blocks;
    unsigned long int l2_size;
    unsigned long int l2_assoc;
};

class L1_L2_Cache:public cache_sim
 {
 public:

     hit_miss_policy_t read_Cache_L1_l2(unsigned address,unsigned long cache_l1_size,int associativity,long block_size);
     hit_miss_policy_t write_Cache_L1_L2(unsigned address,unsigned long cacahe_l1_size,int assocaitiviy,long blocK_size );
     unsigned get_tag_L2(unsigned int address, unsigned long cache_l2_size, unsigned long int associativity, unsigned long int block_size);
     unsigned get_index_L2(unsigned int address,unsigned long int cache_l2_size,unsigned long int associativity,unsigned long int block_size);



 };
// Put additional data structures here as per your requirement


