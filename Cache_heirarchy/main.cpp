#include <iostream>
#include "sim_cache.h"


int main(int argc,char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    cache_params params;
    // look at sim_cache.h header file for the the definition of struct cache_params
    char rw;                // variable holds read/write type read from input file. The array size is 2 because it holds 'r' or 'w' and '\0'. Make sure to adapt in future projects
    unsigned  addr; // Variable holds the address read from input file



    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    params.block_size       = strtoul(argv[1], NULL, 10);
    params.l1_size          = strtoul(argv[2], NULL, 10);
    params.l1_assoc         = strtoul(argv[3], NULL, 10);
    params.vc_num_blocks    = strtoul(argv[4], NULL, 10);
    params.l2_size          = strtoul(argv[5], NULL, 10);
    params.l2_assoc         = strtoul(argv[6], NULL, 10);
    trace_file              = argv[7];
    int sets=params.l1_size/(params.l1_assoc*params.block_size);
    bool L1_VC;
    if(params.vc_num_blocks==0)
                    L1_VC=false;
    else
                   L1_VC= true;
    // Open trace_file in read mode
    cache_sim L1_Cache(params.l1_assoc,params.l1_size,params.block_size,params.vc_num_blocks);
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    // Print params
    printf("===== Simulator configuration =====\n"
           "L1_BLOCKSIZE:                     %lu\n"
           "L1_SIZE:                          %lu\n"
           "L1_ASSOC:                         %lu\n"
           "VC_NUM_BLOCKS:                    %lu\n"
           "L2_SIZE:                          %lu\n"
           "L2_ASSOC:                         %lu\n"
           "trace_file:                       %s\n"
           "===================================\n\n", params.block_size, params.l1_size, params.l1_assoc, params.vc_num_blocks, params.l2_size, params.l2_assoc, trace_file);

    char str[2];
    int i;
    while(fscanf(FP, "%s %lx", str, &addr) != EOF)
    {

        rw = str[0];
        if (rw == 'r')
        {
            if(!L1_VC)
            {
                //printf("%s %lx\n", "read", addr);
                hit_miss_policy_t status = L1_Cache.read_Cache(addr, params.l1_size, params.l1_assoc,
                                                               params.block_size);
                if (status == MISS) {
                    L1_Cache.L1_read_misses++;
                }

            }
            else
            {
                hit_miss_policy_L1V status_v=L1_Cache.L1_Victim_read_cache(addr,params.l1_size,params.l1_assoc,params.block_size);
                if(status_v!=R_L1_HIT)
                {
                    L1_Cache.L1_read_misses++;
                }
            }

            L1_Cache.L1_reads++;


        }// Print and test if file is read correctly
        else if (rw == 'w')
        {
            if(!L1_VC)
            {
                //printf("%s %lx\n", "read", addr);
                hit_miss_policy_t status = L1_Cache.write_Cache(addr, params.l1_size, params.l1_assoc,
                                                               params.block_size);
                if (status == MISS)
                {
                    L1_Cache.L1_writes_misses++;
                }

            }
            else
            {
                hit_miss_policy_L1V status_v=L1_Cache.L1_Victim_write_cache(addr,params.l1_size,params.l1_assoc,params.block_size);
                if(status_v!=W_L1_HIT)
                {
                    L1_Cache.L1_writes_misses++;
                }
            }
            L1_Cache.L1_writes++;
           // printf("%s %lx\n", "write", addr);
        }
        L1_Cache.lru_counter++;
    }
    L1_Cache.L1_VC_miss_rate=(float)(L1_Cache.L1_read_misses+L1_Cache.L1_writes_misses-L1_Cache.swaps)/(float)(L1_Cache.L1_reads+L1_Cache.L1_writes);

    L1_Cache.total_memory_traffic=L1_Cache.L1_read_misses+L1_Cache.L1_writes_misses-L1_Cache.swaps+L1_Cache.evict_count;

    L1_Cache.print_CacheTable(sets,params.l1_assoc);



    return 0;
}
