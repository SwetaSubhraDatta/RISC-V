//
// Created by Sweta Subhra Datta
//
#include "sim_cache.h"
#include "vector"
#include <iostream>
#include "algorithm"
#include "math.h"
#include <map>
using namespace std;

/*
 * This is the paramterised constructor of the class when only L1 or L1-VC
 */
cache_sim::cache_sim(unsigned long associativity, unsigned long l1_size, unsigned long block_size,unsigned VC_NUM_BLOCKS)
{
    unsigned rows_of_SETS = l1_size / (associativity * block_size);
    ///Allocating the size of Cache Table
    L1_Cachetable = vector<vector<L1_block>>(rows_of_SETS,vector<L1_block>(associativity));

    //only allocate victim cache when necessary
    if(VC_NUM_BLOCKS!=0) {
        victim_Cache = vector<L1_block>(VC_NUM_BLOCKS);
        victim_block_size = VC_NUM_BLOCKS;
        for(int i=0;i<victim_Cache.size();i++)
        {
            victim_Cache[i].LRU=numeric_limits<unsigned >::max();
            victim_Cache[i].address=UNDEFINED;
        }
    }
}

/*
 * This function helps to get the index of the address
 * @params::addres,cache_l1_size,assoxiativity,block_size
 * @return index
 */



unsigned long cache_sim::get_index(unsigned  address, unsigned long cache_l1_size, unsigned long associativity,unsigned long block_size)
{
    unsigned long int sets=cache_l1_size/(associativity*block_size);
    unsigned int index=log2(sets);
    unsigned int block_offset=log2(block_size);
    unsigned long index_bit=(address>>block_offset)&((1<<index)-1);
    return index_bit;
}
/*
 * This function helps to get the tag of the address
 * @params::addres,cache_l1_size,assoxiativity,block_size
 * @return index
 */

unsigned long   cache_sim::get_tag(unsigned address, unsigned long cache_l1_size, unsigned long associativity,
                                   unsigned long block_size)
{
    unsigned long int sets=cache_l1_size/(associativity*block_size);
    unsigned int index=log2(sets);
    unsigned int block_offset=log2(block_size);
    unsigned long long tag_bit=(address>>(index + block_offset));
    return tag_bit;
}

/*
 * This function helps to get the offset of the address
 * @params::addres,cache_l1_size,assoxiativity,block_size
 * @return index
 */

unsigned long cache_sim::get_offset(unsigned int address, unsigned long cache_l1_size, unsigned long associativity,unsigned long block_size)
{
    unsigned long int sets=cache_l1_size/(associativity*block_size);
    unsigned int index=log2(sets);
    unsigned int block_offset=log2(block_size);
    // unsigned int tag=32-(index+block_offset);
    unsigned long block_offset_bit=(address>>0 & (1<<block_offset)-1);
    return block_offset_bit;

}


hit_miss_policy_t cache_sim::read_Cache(unsigned int address, unsigned long cache_l1_size, unsigned long int associativity,
                                        unsigned long int  block_size)
{
    unsigned long index = get_index(address,cache_l1_size,associativity,block_size);
    unsigned long long tag = get_tag(address,cache_l1_size,associativity,block_size);
    //unsigned long offset = get_block_offset(address);
    hit_miss_policy_t status;

    //if there is a read hit
    for (unsigned hit_loop = 0; hit_loop < associativity; hit_loop++)
    {
        if (L1_Cachetable[index][hit_loop].tag == tag && L1_Cachetable[index][hit_loop].valid_bit == 1)
        {
            L1_Cachetable[index][hit_loop].LRU = lru_counter;//Accessed
            status = HIT;
            return status;
        }
    }

    //if there is a read miss
    for(int miss_loop=0;miss_loop<associativity;miss_loop++)
    {
        status=MISS;
        ////////if conflict
        if(L1_Cachetable[index][miss_loop].tag!=UNDEFINED)
        {

            unsigned new_block = evict_blocK(index,associativity);
            if(L1_Cachetable[index][new_block].dirty_bit==1)
            {
                evict_count++;

            }

            L1_Cachetable[index][new_block].index = index;
            L1_Cachetable[index][new_block].tag = tag;
            L1_Cachetable[index][new_block].LRU= lru_counter;
            L1_Cachetable[index][new_block].valid_bit = 1;
            L1_Cachetable[index][new_block].dirty_bit=0;
            return MISS;
        }
            //////if empty
        else
        {
            if (L1_Cachetable[index][miss_loop].tag == UNDEFINED ) {
                L1_Cachetable[index][miss_loop].tag = tag;
                // cache_table[index].at(j).block_offset = offset;
                L1_Cachetable[index][miss_loop].dirty_bit = 0;
                L1_Cachetable[index][miss_loop].valid_bit = 1;
                L1_Cachetable[index][miss_loop].LRU = lru_counter;
                return MISS;
            }
        }


    }

    return status;
}
hit_miss_policy_t cache_sim::write_Cache(unsigned int address, unsigned long cache_l1_size,unsigned long associativity, unsigned long block_size)
{
    hit_miss_policy_t status;
    unsigned long index = get_index(address, cache_l1_size, associativity, block_size);
    unsigned long  tag = get_tag(address, cache_l1_size, associativity, block_size);
    for (unsigned hit_loop = 0; hit_loop < associativity; hit_loop++)

        //////////////////////FOR_HIT//////////////////////////////
    {
        if (L1_Cachetable[index][hit_loop].tag == tag && L1_Cachetable[index][hit_loop].valid_bit == 1)
        {

            L1_Cachetable[index][hit_loop].dirty_bit = 1;
            L1_Cachetable[index][hit_loop].LRU = lru_counter;

            status=HIT;
            return status;
        }

    }
    /////////////////////FOR_write_MISS/////////////////////
    for (unsigned miss_loop = 0; miss_loop < associativity; miss_loop++)
    {

        //Picture of L2 comes here
        if (L1_Cachetable[index][miss_loop].tag!=UNDEFINED)//when conflict
        {

            unsigned new_block = evict_blocK(index, associativity);
            if(L1_Cachetable[index][new_block].dirty_bit==1)
            {
                evict_count++;
            }
            L1_Cachetable[index][new_block].LRU = lru_counter;
            L1_Cachetable[index][new_block].tag = tag;
            L1_Cachetable[index][new_block].dirty_bit = 1;
            L1_Cachetable[index][new_block].valid_bit = 1;//Not needed
            L1_Cachetable[index][new_block].address = address;
            status= MISS;
            return status;
        }
        else
        {

            if (L1_Cachetable[index][miss_loop].tag == UNDEFINED )
            {
                L1_Cachetable[index][miss_loop].tag = tag;
                L1_Cachetable[index][miss_loop].index=index;
                L1_Cachetable[index][miss_loop].valid_bit = 1;
                L1_Cachetable[index][miss_loop].dirty_bit = 1;
                L1_Cachetable[index][miss_loop].address = address;
                L1_Cachetable[index][miss_loop].LRU = lru_counter;
                status=MISS;
                return status;
            }
        }

    }
    return status=MISS;
}

/*
 *
 */
hit_miss_policy_L1V
cache_sim::L1_Victim_read_cache(unsigned int address, unsigned long cache_l1_size, unsigned long associativity,
                                unsigned long block_size)
{
    unsigned long index = get_index(address,cache_l1_size,associativity,block_size);
    unsigned long long tag = get_tag(address,cache_l1_size,associativity,block_size);
    //unsigned long offset = get_block_offset(address);



    for (unsigned hit_loop = 0; hit_loop < associativity; hit_loop++)
    {
        //Case-0:if L1 hit do nothing return HIT
        if (L1_Cachetable[index][hit_loop].tag == tag && L1_Cachetable[index][hit_loop].valid_bit == 1)
        {
            L1_Cachetable[index][hit_loop].LRU = lru_counter;//Accessed
            L1_V_status = R_L1_HIT;
            return L1_V_status;
        }
    }


    ///////MISS L1
    for(unsigned miss_loop_L1=0;miss_loop_L1<associativity;miss_loop_L1++)
    {
        L1_V_status=L1_MISS_READ;
                            //Check in Victim
                              ///victim hit

                     for(unsigned victim_hit_loop=0;victim_hit_loop<victim_block_size;victim_hit_loop++)
                     {
                         if (victim_Cache[victim_hit_loop].tag == tag) {
                             L1_V_status=R_L1_MISS_BUT_VICTIM_HIT_SWAPPED;
                             swap_with_victim(index, associativity,victim_hit_loop);
                             swap_requests++;
                             return L1_V_status;



                         }
                     }

        if(L1_V_status==L1_MISS_READ)///Miss from Both L1 and Victim
        {
            //Case 1-if L1 is empty
                    //->push block to L1
            if (L1_Cachetable[index][miss_loop_L1].tag == UNDEFINED  && L1_Cachetable[index][miss_loop_L1].valid_bit==0 )//L1 is empty
            {
                L1_Cachetable[index][miss_loop_L1].tag = tag;
                L1_Cachetable[index][miss_loop_L1].index=index;
                L1_Cachetable[index][miss_loop_L1].valid_bit = 1;
                L1_Cachetable[index][miss_loop_L1].dirty_bit = 0;
                L1_Cachetable[index][miss_loop_L1].address = address;
                L1_Cachetable[index][miss_loop_L1].LRU = lru_counter;
                L1_V_status=R_L1_MISS_L1_EMPTY_REPLACED;
                return L1_V_status;

            }
            //Case 2;if L1 is not empty
                        //->evict block to Victim
                                //Case 2.1:if Victim is full
                                        //-> evict block from Victim based on LRU
                                            //if dirty bit of this block ==1 than mainmem++;
                                                //-> replace the newly evicted block from L1 in Victim
                                            //else
                                                //->if victim is not full just replace the block;
            if(L1_Cachetable[index][miss_loop_L1].tag!=UNDEFINED && L1_Cachetable[index][miss_loop_L1].valid_bit==1 && Cache_index_is_Full(index,associativity))
            {

                unsigned L1_evict_block_id = evict_blocK(index, associativity);
                for(int i=0;i<victim_block_size;i++) {
                    //Place available in Victim
                            if(victim_Cache[i].tag==UNDEFINED)
                            {
                                victim_Cache[i].tag = L1_Cachetable[index][L1_evict_block_id].tag;
                                victim_Cache[i].LRU =lru_counter;
                                victim_Cache[i].valid_bit = L1_Cachetable[index][L1_evict_block_id].valid_bit;
                                victim_Cache[i].dirty_bit = L1_Cachetable[index][L1_evict_block_id].dirty_bit;
                                victim_Cache[i].address = L1_Cachetable[index][L1_evict_block_id].address;
                               // sort_Victim();
                                //Dont forget to replace the new block in L1
                                L1_Cachetable[index][L1_evict_block_id].tag=tag;
                                L1_Cachetable[index][L1_evict_block_id].LRU=lru_counter;
                                L1_Cachetable[index][L1_evict_block_id].valid_bit=1;
                                L1_Cachetable[index][L1_evict_block_id].dirty_bit=0;
                                L1_Cachetable[index][L1_evict_block_id].address=address;
                                L1_V_status=R_L1_FULL_VICTIM_EMPTY_EVICTED2VICTIM_L1_REPLACED;
                                return L1_V_status;
                            }
                    //Victim full
                            if(victim_Cache[i].tag!=UNDEFINED && no_empty_blocks_in_victim())
                            {
                                unsigned victim_block_to_be_evicted=evict_block_from_victim();
                                if(victim_Cache[victim_block_to_be_evicted].dirty_bit==1)
                                {
                                    evict_count++;
                                }
                                victim_Cache[victim_block_to_be_evicted].tag = L1_Cachetable[index][L1_evict_block_id].tag;
                                victim_Cache[victim_block_to_be_evicted].LRU = lru_counter;
                                victim_Cache[victim_block_to_be_evicted].valid_bit = L1_Cachetable[index][L1_evict_block_id].valid_bit;
                                victim_Cache[victim_block_to_be_evicted].dirty_bit = L1_Cachetable[index][L1_evict_block_id].dirty_bit;
                                victim_Cache[victim_block_to_be_evicted].address = L1_Cachetable[index][L1_evict_block_id].address;
                               // sort_Victim();
                                swap_requests++;
                                //dont forget to replace the new block in L1
                                L1_Cachetable[index][L1_evict_block_id].tag=tag;
                                L1_Cachetable[index][L1_evict_block_id].LRU=lru_counter;
                                L1_Cachetable[index][L1_evict_block_id].valid_bit=1;
                                L1_Cachetable[index][L1_evict_block_id].dirty_bit=0;
                                L1_Cachetable[index][L1_evict_block_id].address=address;
                                L1_V_status=R_L1_FULL_VICTIM_FULL_EVICTEDFROMVICTIM_L1_REPLACED;
                                return L1_V_status;
                            }

                }

            }
        }

    }


    return L1_V_status;
}

/*
 *
 */

hit_miss_policy_L1V
cache_sim::L1_Victim_write_cache(unsigned int address, unsigned long cache_l1_size, unsigned long associativity,
                                unsigned long block_size)
{
    unsigned long index = get_index(address,cache_l1_size,associativity,block_size);
    unsigned long long tag = get_tag(address,cache_l1_size,associativity,block_size);
    //unsigned long offset = get_block_offset(address);



    for (unsigned hit_loop = 0; hit_loop < associativity; hit_loop++)
    {
        //Case-0:if L1 hit do nothing return HIT
        if (L1_Cachetable[index][hit_loop].tag == tag && L1_Cachetable[index][hit_loop].valid_bit == 1)
        {
            L1_Cachetable[index][hit_loop].LRU = lru_counter;//Accessed

            L1_Cachetable[index][hit_loop].dirty_bit=1;

            L1_V_status = W_L1_HIT;
            return L1_V_status;
        }
    }


    ///////MISS L1
    for(unsigned miss_loop_L1=0;miss_loop_L1<associativity;miss_loop_L1++)
    {
        L1_V_status=L1_MISS_WRITE;
        //Check in Victim
        ///victim hit
        //sort_Victim();
        for(unsigned victim_hit_loop=0;victim_hit_loop<victim_block_size;victim_hit_loop++)
        {
            if (victim_Cache[victim_hit_loop].tag == tag ) {
                L1_V_status=W_L1_MISS_BUT_VICTIM_HIT_SWAPPED;
                swap_with_victim(index, associativity,victim_hit_loop);
                swap_requests++;
                //sort_Victim();
                return L1_V_status;



            }
        }

        if(L1_V_status==L1_MISS_WRITE)///Miss from Both L1 and Victim
        {
            //Case 1-if L1 is empty
            //->push block to L1
            if (L1_Cachetable[index][miss_loop_L1].tag == UNDEFINED  && L1_Cachetable[index][miss_loop_L1].valid_bit==0 )//L1 is empty
            {
                L1_Cachetable[index][miss_loop_L1].tag = tag;
                L1_Cachetable[index][miss_loop_L1].index=index;
                L1_Cachetable[index][miss_loop_L1].valid_bit = 1;
                L1_Cachetable[index][miss_loop_L1].dirty_bit = 1;
                L1_Cachetable[index][miss_loop_L1].address = address;
                L1_Cachetable[index][miss_loop_L1].LRU = lru_counter;
                L1_V_status=W_L1_MISS_L1_EMPTY_REPLACED;
                return L1_V_status;

            }
            //Case 2;if L1 is not empty
            //->evict block to Victim
            //Case 2.1:if Victim is full
            //-> evict block from Victim based on LRU
            //if dirty bit of this block ==1 than mainmem++;
            //-> replace the newly evicted block from L1 in Victim
            //else
            //->if victim is not full just replace the block;
            if(L1_Cachetable[index][miss_loop_L1].tag!=UNDEFINED && L1_Cachetable[index][miss_loop_L1].valid_bit==1 && Cache_index_is_Full(index,associativity))
            {
                //sort_Victim();

                unsigned L1_evict_block_id = evict_blocK(index, associativity);
                for(int i=0;i<victim_block_size;i++) {
                    //Place available in Victim
                    if(victim_Cache[i].tag==UNDEFINED)
                    {
                        victim_Cache[i].tag = L1_Cachetable[index][L1_evict_block_id].tag;
                        victim_Cache[i].LRU = lru_counter;
                        victim_Cache[i].valid_bit = L1_Cachetable[index][L1_evict_block_id].valid_bit;
                        victim_Cache[i].dirty_bit = L1_Cachetable[index][L1_evict_block_id].dirty_bit;
                        victim_Cache[i].address = L1_Cachetable[index][L1_evict_block_id].address;
                        swap_requests++;
                       // sort_Victim();
                        //Dont forget to replace the new block in L1
                        L1_Cachetable[index][L1_evict_block_id].tag=tag;
                        L1_Cachetable[index][L1_evict_block_id].LRU=lru_counter;
                        L1_Cachetable[index][L1_evict_block_id].valid_bit=1;
                        L1_Cachetable[index][L1_evict_block_id].dirty_bit=1;
                        L1_Cachetable[index][L1_evict_block_id].address=address;
                        L1_V_status=W_L1_FULL_VICTIM_EMPTY_EVICTED2VICTIM_L1_REPLACED;
                        return L1_V_status;
                    }
                    //Victim full
                    if(victim_Cache[i].tag!=UNDEFINED  && no_empty_blocks_in_victim())
                    {
                        unsigned evict_index_from_victim=evict_block_from_victim();
                        if(victim_Cache[evict_index_from_victim].dirty_bit==1)
                        {
                            evict_count++;
                        }
                        victim_Cache[evict_index_from_victim].tag = L1_Cachetable[index][L1_evict_block_id].tag;
                        victim_Cache[evict_index_from_victim].LRU =lru_counter;
                        victim_Cache[evict_index_from_victim].valid_bit = L1_Cachetable[index][L1_evict_block_id].valid_bit;
                        victim_Cache[evict_index_from_victim].dirty_bit = L1_Cachetable[index][L1_evict_block_id].dirty_bit;
                        victim_Cache[evict_index_from_victim].address = L1_Cachetable[index][L1_evict_block_id].address;
                        swap_requests++;
                        //sort_Victim();
                        //dont forget to replace the new block in L1
                        L1_Cachetable[index][L1_evict_block_id].tag=tag;
                        L1_Cachetable[index][L1_evict_block_id].LRU=lru_counter;
                        L1_Cachetable[index][L1_evict_block_id].valid_bit=1;
                        L1_Cachetable[index][L1_evict_block_id].dirty_bit=1;
                        L1_Cachetable[index][L1_evict_block_id].address=address;
                        L1_V_status=W_L1_FULL_VICTIM_FULL_EVICTEDFROMVICTIM_L1_REPLACED;
                        return L1_V_status;
                    }

                }

            }
        }

    }


    return L1_V_status;
}







/////////////////////////////////UTILITY//////////////////////////
unsigned int cache_sim::evict_block_from_victim()
{
    int block=0;//taking block as 0
    unsigned  min_blk_timestamp= numeric_limits<unsigned >::max();
    for(int i=0;i<victim_block_size;i++)
    {

        if(victim_Cache[i].LRU<min_blk_timestamp)
        {
            min_blk_timestamp=victim_Cache[i].LRU;
            block=i;
        }
    }
    return block;
}

unsigned int cache_sim::evict_blocK(unsigned int index, unsigned long associativity)
{
    int block=0;//taking block as 0
  unsigned  min_blk_timestamp= numeric_limits<unsigned >::max();
    for(int i=0;i<associativity;i++)
    {

        if(L1_Cachetable[index][i].LRU<min_blk_timestamp)
        {
            min_blk_timestamp=L1_Cachetable[index][i].LRU;
            block=i;
        }
    }
    return block;
}

void cache_sim::swap_with_victim(unsigned int index, unsigned int assoc,unsigned victimblock)
{
    vector<vector<L1_block>> hold_Cache;
    hold_Cache=L1_Cachetable;


    int lru_block=0;//taking block as 0
    unsigned  min_blk_timestamp= numeric_limits<unsigned >::max();
    for(int i=0;i<assoc;i++)
    {

        if(L1_Cachetable[index][i].LRU<min_blk_timestamp)
        {
            min_blk_timestamp=L1_Cachetable[index][i].LRU;
            lru_block=i;
        }
    }
    L1_Cachetable[index][lru_block].tag = victim_Cache[victimblock].tag;
    L1_Cachetable[index][lru_block].LRU = lru_counter;
    L1_Cachetable[index][lru_block].valid_bit = victim_Cache[victimblock].valid_bit;
    L1_Cachetable[index][lru_block].dirty_bit = victim_Cache[victimblock].dirty_bit;
    L1_Cachetable[index][lru_block].address = victim_Cache[victimblock].address;
    L1_Cachetable[index][lru_block].index = victim_Cache[victimblock].index;
    ///////////////////////////
    unsigned lru_victim_block=evict_block_from_victim();
    victim_Cache[lru_victim_block].index = hold_Cache[index][lru_block].index;
    victim_Cache[lru_victim_block].tag = hold_Cache[index][lru_block].tag;
    victim_Cache[lru_victim_block].valid_bit = hold_Cache[index][lru_block].valid_bit;
    victim_Cache[lru_victim_block].dirty_bit = hold_Cache[index][lru_block].dirty_bit;
    victim_Cache[lru_victim_block].address = hold_Cache[index][lru_block].address;
    victim_Cache[lru_victim_block].LRU = lru_counter;

        ////////////////

}

void cache_sim::sort_Victim()

{
    if(!victim_Cache.empty()) {

        std::sort(victim_Cache.begin(), victim_Cache.end(),
                  [](const L1 &left, const L1 &right) { return (left.LRU < right.LRU); });
    }
}

bool cache_sim::Cache_index_is_Full(unsigned int index, unsigned long associativity)
{
    for(unsigned i=0;i<associativity;i++)
    {
        if(L1_Cachetable[index][i].tag==UNDEFINED && L1_Cachetable[index][i].valid_bit==0)
        {
            return false;

        }
    }
    return true;
}

bool cache_sim::no_empty_blocks_in_victim()
{
    for(int i=0;i<victim_block_size;i++)
    {
        if(victim_Cache[i].tag==UNDEFINED)
        {
            return false;
        }
    }
    return true;
}

void cache_sim::print_CacheTable(unsigned long rows, unsigned long columns)
{
    for(int i=0;i<L1_Cachetable.size();i++) {
        std::sort(L1_Cachetable[i].begin(), L1_Cachetable[i].end(),
                  [](const L1 &left, const L1 &right) { return (left.LRU > right.LRU); });
    }

    //vector<vector<L1_block>>sorted_table(L1_Cachetable.size());
    cout<<"===== L1 contents ====="<<endl;
//    std::sort(L1_Cachetable.begin(), L1_Cachetable.end(),
//              [](const L1 &left, const L1 &right) { return (left.LRU < right.LRU); });
    for(int i=0;i<rows;i++)
    {
        cout<<"set  "<<dec<<i<<": ";
        for (int j=0;j<columns;j++)
        {
            cout << " " << hex<<L1_Cachetable[i][j].tag ;
            if(L1_Cachetable[i][j].dirty_bit==1)
            {
                cout<<" D ";
            }



        }
        cout<<"\n";
    }

    if(!victim_Cache.empty()) {
        cout<<"\n===== VC contents ======"<<endl;
       sort_Victim();
        for (int i = victim_Cache.size()-1;i>=0; i--) {
            cout << hex <<victim_Cache[i].address<< " ";
        }
    }
    cout<<"\n"<<endl;
    cout<<"===== Simulation results ====="<<endl;
    cout<<"a. number of L1 reads: "<<dec<<L1_reads<<endl;
    cout<<"b. number of L1 read misses: "<<dec<<L1_read_misses<<endl;
    cout<<"c. number of L1 writes: "<<dec<<L1_writes<<endl;
    cout<<"d. number of L1 write_misses: "<<dec<<L1_writes_misses<<endl;
    cout<<"e. number of swap requests: "<<dec<<swap_requests<<endl;
   // cout<<"f. swap request rate: "<<left<<setfill('0')<<setw(4)<<(float)swap_request_rate<<endl;
    printf("f. swap request rate: ");
    printf("%.4f",(double)swap_request_rate);
    cout<<"\n";
    cout<<"g. number of swaps: "<<dec<<swaps<<endl;
    //cout<<"h. combined L1+VC miss rate: "<<dec<<L1_VC_miss_rate<<endl;
    printf("h. combined L1+VC miss rate: ");
    printf("%.4f",L1_VC_miss_rate);
    cout<<"\n";
    cout<<"i. number writebacks from L1/VC: "<<dec<<evict_count<<endl;
    cout<<"j. number of L2 reads: "<<dec<<L2_reads<<endl;
    cout<<"k. number of L2 read misses: "<<dec<<L2_read_misses<<endl;
    cout<<"l. number of L2 writes: "<<dec<<L2_writes<<endl;
    cout<<"m. number of L2 write misses: "<<dec<<L2_writes_misses<<endl;
    //cout<<"n. L2 miss rate: "<<dec<<L2_miss_rate<<endl;
    printf("f. L2 miss rate: ");
    printf("%.4f",(double)L2_miss_rate);
    cout<<"\n";
    cout<<"o. number of writebacks from L2: "<<dec<<writebacks_L2<<endl;
    cout<<"p. total memory traffic: "<<dec<<total_memory_traffic<<endl;
}











