//
// Created by Rob Datta on 9/24/21.
//

#include "sim_cache.h"

cache_sim::cache_sim(unsigned  assoc_1, unsigned  assoc_2, unsigned  l1_size, unsigned L2_size,
                     unsigned  block_size)

                     {
                         unsigned L1_SETS = l1_size / (assoc_1 * block_size);
                         unsigned L2_SETS=L2_size/(assoc_2*block_size);
                         L1_Cachetable = vector<vector<L1_block>>(L1_SETS,vector<L1_block>(assoc_1));
                         L2_Cachetable= vector<vector<L1_block>>(L2_SETS, vector<L1_block>(assoc_2));
                         sets_l1=L1_SETS;
                         sets_l2=L2_SETS;
                         l1_assoc=assoc_1;
                         l2_assoc=assoc_2;
                         cache_size_l1=l1_size;

                     }

unsigned int L1_L2_Cache::get_index_L2(unsigned int address, unsigned long cache_l2_size, unsigned long associativity,unsigned long block_size)
                              {
    unsigned long int sets=cache_l2_size/(associativity*block_size);
    unsigned int index=log2(sets);
    unsigned int block_offset=log2(block_size);
    unsigned long index_bit=(address>>block_offset)&((1<<index)-1);
    return index_bit;
                              }

unsigned int L1_L2_Cache::get_tag_L2(unsigned int address, unsigned long cache_l2_size, unsigned long associativity,
                                     unsigned long block_size)
                                     {
                                         unsigned long int sets=cache_l2_size/(associativity*block_size);
                                         unsigned int index=log2(sets);
                                         unsigned int block_offset=log2(block_size);
                                         unsigned long long tag_bit=(address>>(index + block_offset));
                                         return tag_bit;
                                     }






hit_miss_policy_t
L1_L2_Cache::read_Cache_L1_l2(unsigned int address, unsigned long cache_l1_size, int associativity, long block_size)
{
    unsigned long index = get_index(address,cache_l1_size,associativity,block_size);
    unsigned long long tag = get_tag(address,cache_l1_size,associativity,block_size);
    hit_miss_policy_t status;

    for(unsigned hit_loop=0;hit_loop<associativity;hit_loop++)
    {
        ///L1 hit
        if(L1_Cachetable[index][hit_loop].tag==tag ||L1_Cachetable[index][hit_loop].valid_bit==1)
        {

            L1_Cachetable[index][hit_loop].LRU=lru_counter;
            status=HIT;
            return status;

        }
    }
    ///L1 miss
    for(unsigned miss_loop=0;miss_loop<associativity;miss_loop++)
    {
        status=MISS;

        //Step 1:
                //Case 1 :empty block available in l1
                                //seearch in L2 ka bakchodi  ///read request to L2 +
                                //insert block
                                //return miss
                //Case2: No empty block in L1
                            //Case 2.1:if dirty_bit==0 than same as case 1 //just overwrite the data
                            //Case 2.2::If dirty_bit==1
                                                    //convert address to L2
                                                    //than  write_request to_L2
                                                                    //if HIT tha L2_BLOCK_DIRTY_BIT=1;
                                                                    //else MISS
                                                                            //Case 1::Empty block available || dirty_bit==0
                                                                                        //replace block and dirty bit =1;
                                                                             //Case 2::No empty block in L2 + dirty  bit =1
                                                                                        //evict block to memory and evict++
                                                                                        //replace  block in L2

              ///get replaced block and convert to L1 address
              /// ///replace block L1 address









    }

return HIT;
}

hit_miss_policy_t
L1_L2_Cache::write_Cache_L1_L2(unsigned int address, unsigned long cacahe_l1_size, int assocaitiviy, long blocK_size)
{
return MISS;
}