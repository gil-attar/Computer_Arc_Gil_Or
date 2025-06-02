#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;


//global variables
int cm = 0;
int commands_num = 0; //number of L1 accesses
int L1_misses = 0; //number of L1 misses - same as L2 accesses
int L2_misses = 0; //number of L2 misses - same as Mem accesses
int is_write_allocate = 0; //write allocate - 1 for true, 0 for false
unsigned int set_mask_L1 = 0x0 - 1; //mask = 0xFF...
unsigned int tag_mask_L1 = 0x0 - 1; //mask = 0xFF...
int set_bits_L1;
int tag_bits_L1;
unsigned int set_mask_L2 = 0x0 - 1; //mask = 0xFF...
unsigned int tag_mask_L2 = 0x0 - 1; //mask = 0xFF...
int set_bits_L2;
int tag_bits_L2;

//declare structs
typedef struct{
	unsigned int tag;
	bool dirty_bit;
	bool valid; //valid bit - true if the block is valid, false if it is not
} Block;

typedef struct{
//cache level (L1/L2) struct holds a pointer to an array of blocks.
// set 0: blocks[0,...,#W-1] | set1: blocks[#W,..,2#W-1]
	Block* blocks;
	int cache_number; //1 for L1, 2 for L2
	unsigned assoc; //assoc_level = log2(num of ways)
	unsigned int set_mask; 
	unsigned int tag_mask;
	int set_bits;
	int tag_bits;
	int offset_bits;
	int ways_num; //number of ways in the cache level
	int blocks_num;
	unsigned int** lru;
} Cache_Level;

typedef struct{
	Cache_Level* L1;
	Cache_Level* L2;
} Cache;

// declare functions
// creates Cache struct and returns a pointer to it
Cache* Cache_init(unsigned block_size, unsigned L1_size, unsigned L2_size, unsigned L1_assoc,
				  unsigned L2_assoc, unsigned L1_clocks, unsigned L2_clocks, unsigned write_alloc);
void Cache_free(Cache* cache);

void create_masks(unsigned block_bits, unsigned chache_bits, Cache_Level* cache);
unsigned get_tag(unsigned adress, Cache_Level* cache);
unsigned get_set(unsigned adress, Cache_Level* cache);

void update_lru(Cache_Level* cache, int set_index, int accessed_way);
int lru_way(Cache_Level* cache, int set_index);

int search_way(Cache_Level* cache, unsigned int tag, unsigned int set);
void insert_way(Cache_Level* cache, unsigned int tag, unsigned int set, int way);







//------------------------------------------------------------------------------------------------------

Cache* Cache_init(unsigned block_size, unsigned L1_size, unsigned L2_size,
				  unsigned L1_assoc, unsigned L2_assoc, unsigned L1_clocks,unsigned L2_clocks,
				  unsigned write_alloc){
/*the function returns pointer to a Cache struct. 
 block_size:  <log2(size)>
 L1_size, L2_size : <log2(size)>
 L1_assoc, L2_assoc: <log2(number of ways)>
*/
	Cache* Cache_p = (Cache*)malloc(sizeof(Cache));
	Cache_p->L1 = (Cache_Level*)malloc(sizeof(Cache_Level));
	Cache_p->L2 = (Cache_Level*)malloc(sizeof(Cache_Level));

	Cache_p->L1->blocks_num = 1 << (L1_size - block_size); 
	Cache_p->L2->blocks_num = 1 << (L2_size - block_size); 

	Cache_p->L1->blocks = (Block*)malloc(Cache_p->L1->blocks_num*sizeof(Block));
	Cache_p->L2->blocks = (Block*)malloc(Cache_p->L2->blocks_num*sizeof(Block));
	
	for (int i = 0; i < Cache_p->L1->blocks_num; i++) {
    	Cache_p->L1->blocks[i].valid = false; // Mark as empty
		Cache_p->L1->blocks[i].dirty_bit = false; // Initially not dirty
	}
	for (int i = 0; i < Cache_p->L2->blocks_num; i++) {
    	Cache_p->L2->blocks[i].valid = false; // Mark as empty
		Cache_p->L2->blocks[i].dirty_bit = false; // Initially not dirty
	}

	is_write_allocate = write_alloc;
	
	Cache_p->L1->assoc = L1_assoc;
	Cache_p->L2->assoc = L2_assoc;

	Cache_p->L1->cache_number = 1;
	Cache_p->L2->cache_number = 2;

	Cache_p->L1->ways_num = 1 << L1_assoc; // ways_num = 2^assoc_level
	Cache_p->L2->ways_num = 1 << L2_assoc; // ways_num = 2^assoc_level
	
	//function that sets: set_bits, offset_bits, tag_bits and masks
	create_masks(block_size, L1_size, Cache_p->L1);
	create_masks(block_size, L2_size, Cache_p->L2);

	// LRU allocation and init - might need to use these 2 lines, compiler can do problems with malloc
    //unsigned int num_sets_L1 = L1_p->blocks_num / L1_p->ways_num;
    //unsigned int num_sets_L2 = L2_p->blocks_num / L2_p->ways_num;

    Cache_p->L1->lru = (unsigned int**)malloc(Cache_p->L1->blocks_num * sizeof(unsigned int*));
    for (int i = 0; i < Cache_p->L1->blocks_num; i++) {
        Cache_p->L1->lru[i] = (unsigned int*)malloc(Cache_p->L1->ways_num * sizeof(unsigned int));
        for (int j = 0; j < Cache_p->L1->ways_num; j++) {
            Cache_p->L1->lru[i][j] = 0; // perhaps ways_num - 1
        }
    }

    Cache_p->L2->lru = (unsigned int**)malloc(Cache_p->L2->blocks_num * sizeof(unsigned int*));
    for (int i = 0; i < Cache_p->L2->blocks_num; i++) {
        Cache_p->L2->lru[i] = (unsigned int*)malloc(Cache_p->L2->ways_num * sizeof(unsigned int));
        for (int j = 0; j < Cache_p->L2->ways_num; j++) {
            Cache_p->L2->lru[i][j] = 0; // perhaps ways_num - 1
        }
    }	
	
	return Cache_p;
}

void Cache_free(Cache* cache) {
    if (!cache) return;

    if (cache->L1) {
        if (cache->L1->lru) {
            for (int i = 0; i < cache->L1->blocks_num; i++) {
                free(cache->L1->lru[i]);
            }
            free(cache->L1->lru);
        }
        free(cache->L1->blocks);
        free(cache->L1);
    }

    if (cache->L2) {
        if (cache->L2->lru) {
            for (int i = 0; i < cache->L2->blocks_num; i++) {
                free(cache->L2->lru[i]);
            }
            free(cache->L2->lru);
        }
        free(cache->L2->blocks);
        free(cache->L2);
    }

    free(cache);
}

//------------------------------------------------------------------------------------------------------

void create_masks(unsigned block_bits, unsigned cache_bits, Cache_Level* cache){
	//function that gets user's parameters and pointer to cache.
	//determines number of bits of set, tag and offset.
	//it changes the masks to enable us to extract set and tag from each adress
	unsigned assoc_bits = cache->assoc; //what is assoc bits? do we need to add int/ unsighned? or is it a field in cache level?
	//find number of offset bits 
	cache->offset_bits = block_bits; //block_size =block_bits = log2(block_size)
	//find number of set bits and change masks accordingly
	cache->set_bits = cache_bits - block_bits - assoc_bits; //chache_bits = cache_size = log2(c_Size)
	cache->tag_bits = cache->offset_bits +cache->set_bits;
	cache->tag_mask = (0x0 - 1) << (cache->set_bits+ cache->offset_bits); //0x0-1 = FFFF...
	cache->set_mask = ((1u<<cache->set_bits) - 1u)<<cache->offset_bits;
}

unsigned get_tag(unsigned adress, Cache_Level* cache){
	unsigned tag = (adress&(cache->tag_mask))>>(cache->offset_bits + cache->set_bits);
	return tag;
}

unsigned get_set(unsigned adress, Cache_Level* cache){
	//unsigned tag  = (addr & tag_mask) >> (offset_bits + set_bits);
	unsigned set_index = (adress&(cache->set_mask))>>(cache->offset_bits);
	return set_index;
}

//------------------------------------------------------------------------------------------------------

void update_lru(Cache_Level* cache, unsigned int set_index, int accessed_way) {
	if (cm) printf("cache number %d, in update_lru: set_index=%d, accessed_way=%d\n",cache->cache_number, set_index, accessed_way);
    unsigned int* lru_set = cache->lru[set_index];
    int base = set_index * cache->ways_num;
    //unsigned int old_value = lru_set[accessed_way];
    for (int i = 0; i < cache->ways_num; i++) {
        if (i == accessed_way) continue;
        if (!cache->blocks[base + i].valid) continue; // only valid blocks participate

        if (lru_set[i] > lru_set[accessed_way] && lru_set[i]>0) {
            lru_set[i]--;
        }
    }

    lru_set[accessed_way] = cache->ways_num - 1;

	if (cm) {
    	if (cm) printf("Updated LRU set[%d]: \n", set_index);
    	for (int i = 0; i < cache->ways_num; i++) {
			if (!cache->blocks[base + i].valid) continue;
        	if (cm) printf("index is [%d], value is: %d \n", i, lru_set[i]);
    	}
	}
}

int lru_way(Cache_Level* cache, int set_index) {
	if (cm) printf("cache number %d, in lru_way: set_index=%d\n",cache->cache_number, set_index);
    int base = set_index * cache->ways_num;
	if (cm) printf("in lru_way: base=%d\n", base);
    // look for invalid block (empty spot)
    for (int i = 0; i < cache->ways_num; i++) {
		if (cm) printf("accesing to cache->blocks[%d]\n", base + i);
        if (!cache->blocks[base + i].valid) {
			if (cm) printf("Found empty block at way %d in set %d\n", i, set_index);
            return i;
        }
    }
    // return block with LRU = 0
    for (int i = 0; i < cache->ways_num; i++) {
		if (cm) printf("accesing to cache->lru[set_idx = %d][i= %d] = %d \n", set_index, i, cache->lru[set_index][i]);
        if (cache->lru[set_index][i] == 0) {
			if (cm) printf("Found LRU block at way %d in set %d\n", i, set_index);
            return i;
        }
    }
    //fprintf(stderr,"lru_way: no valid victim, default -1\n");
    return -1;
}

//------------------------------------------------------------------------------------------------------

int search_way(Cache_Level* cache, unsigned int tag, unsigned int set) {
	if (cm) printf("cache number %d, in search_way: tag=%u, set=%d\n", cache->cache_number, tag, set);
    for (int i = 0; i < cache->ways_num; i++) {
        Block* blk = &cache->blocks[set * cache->ways_num + i];
        if (blk->valid && blk->tag == tag) {
            update_lru(cache, set, i);
			if (cm) printf("search_way: Found block with tag %u in set %d at way %d\n", tag, set, i);
            return i;
        }
    }
	if (cm) printf("search_way: Block with tag %u not found in set %d\n", tag, set);
    return -1;
}

void insert_way(Cache_Level* cache, unsigned int tag, unsigned int set, int way) {
	if (cm) printf("cache number %d, in insert_way: tag=%u, set=%d, way=%d\n", cache->cache_number, tag, set, way);
    Block* blk = &cache->blocks[set * cache->ways_num + way];
    blk->tag = tag;
    blk->valid = true;
	blk->dirty_bit = false; // Initially not dirty, in write we will update manually
    update_lru(cache, set, way);
	if (cm) printf("end insert_way: Inserted block with tag %u in set %d at way %d\n", tag, set, way);
}


//------------------------------------------------------------------------------------------------------


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	} 

	// Get input arguments
	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	//===build Cache===
	Cache* cache =  Cache_init(BSize, L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc);

	while (getline(file, line)) {
		if (cm) printf("in while loop, line: %s\n", line.c_str());
		commands_num++;
		stringstream ss(line);
		string address;
		int r_w_flag; //R=0 | W=1
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
				
		// DEBUG - remove this line
		if (cm) cout << "operation: " << operation;

		if(operation == 'r'){
			r_w_flag = 0;
		}else if(operation == 'w'){
			r_w_flag = 1;
		}

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		if (cm) cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16); //this is the adress unsigned

		// DEBUG - remove this line
		if (cm) cout << " (dec) " << num << endl;
		
		unsigned int input_tag_L1 = get_tag(num, cache->L1);
		unsigned int input_tag_L2 = get_tag(num, cache->L2);


		unsigned int input_set_L1 =  get_set(num, cache->L1);
		unsigned int input_set_L2 =  get_set(num, cache->L2);

		if (cm){
			printf("input_tag_L1: %u, input_set_L1: %u\n", input_tag_L1, input_set_L1);
			printf("input_tag_L2: %u, input_set_L2: %u\n", input_tag_L2, input_set_L2);
		}

		bool in_L1 = true; // is the block in L1?
		bool in_L2 = true; // is the block in L2?
		
		int way_L1 = search_way(cache->L1, input_tag_L1, input_set_L1);
		int way_L2 = search_way(cache->L2, input_tag_L2, input_set_L2);
		if (way_L1 == -1){
			in_L1 = false; // not in L1
			L1_misses++;
			if (way_L2 == -1) {
				in_L2 = false; // not in L2
				L2_misses++;
			}
		}
		if (cm) {
			if (in_L1) printf("Block found in L1 at way %d\n", way_L1);
			else printf("Block not found in L1\n");
			if (in_L2) printf("Block found in L2 at way %d\n", way_L2);
			else printf("Block not found in L2\n");
		}

		if (r_w_flag == 0){ //read
			// just need to update lru - this is a simulation [inside of a simulation! (inside of another simulation!)].
			// oh i forgot, i also need to bring the block to the cache if it is not there...
			if (in_L1) {
				update_lru(cache->L1, input_set_L1, way_L1); // update L1
				cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_L1].valid = true; // Mark the block as valid
			}
			else if (in_L2) {
				int way_to_delete_L1 = lru_way(cache->L1, input_set_L1);
				// in order to delete from L1, we need to update victim access in L2 if its dirty in L1
				Block* victim_L1 = &cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1];
				if (victim_L1->valid && victim_L1->dirty_bit) {
					// search for matching block in L2 (same set)
					for (int i = 0; i < cache->L2->ways_num; i++) {
						Block* blk_L2 = &cache->L2->blocks[input_set_L2 * cache->L2->ways_num + i];
						if (blk_L2->valid && blk_L2->tag == victim_L1->tag) {
							update_lru(cache->L2, input_set_L2, i);
							blk_L2->dirty_bit = true;  // write back to L2 the deleted block from L1
							break;
						}
					}
				}
				insert_way(cache->L1, input_tag_L1, input_set_L1, way_to_delete_L1); // will call update lru on L1
				update_lru(cache->L2, input_set_L2, way_L2); // update L2
				cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1].valid = true; // Mark the block as valid in L1, who cares that it also happend in insert_way?
				cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_L2].valid = true; // Mark the block as valid, in case it was unvalid for some reason
			}
			else {
				int way_to_delete_L2 = lru_way(cache->L2, input_set_L2);
    			Block* victim_L2 = &cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_to_delete_L2];
				// if need to delete from L2, then also delete from L1 (if it exists)
				if (victim_L2->valid) {
					for (int i = 0; i < cache->L1->ways_num; i++) {
						Block* blk = &cache->L1->blocks[input_set_L1 * cache->L1->ways_num + i];
						if (blk->valid && blk->tag == victim_L2->tag) { // check if it affect update lru
							blk->valid = false;
							break;
						}
					}
				}
				
				// insert block to L2
				insert_way(cache->L2, input_tag_L2, input_set_L2, way_to_delete_L2); // will call update lru on L2
				cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_to_delete_L2].valid = true; // insert might do it too

				// insert block to L1
				int way_to_delete_L1 = lru_way(cache->L1, input_set_L1);
				//in case the block wasnt in L1, we need to again update + writeback to L2 if it was dirty. if the block was deleted the update wont do anything
				Block* victim_L1 = &cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1];
				if (victim_L1->valid && victim_L1->dirty_bit) {
					// search for matching block in L2 (same set)
					for (int i = 0; i < cache->L2->ways_num; i++) {
						Block* blk_L2 = &cache->L2->blocks[input_set_L2 * cache->L2->ways_num + i];
						if (blk_L2->valid && blk_L2->tag == victim_L1->tag) {
							update_lru(cache->L2, input_set_L2, i);
							blk_L2->dirty_bit = true;  // write back to L2 the deleted block from L1
							break;
						}
					}
				}
				insert_way(cache->L1, input_tag_L1, input_set_L1, way_to_delete_L1); // will call update lru on L1
				cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1].valid = true; // insert might do it too
			}

		}

		else { //write
			// write allocate
			if (is_write_allocate) {
				if (in_L1) {
					update_lru(cache->L1, input_set_L1, way_L1);
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_L1].valid = true; // Mark the block as valid
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_L1].dirty_bit = true; // Update dirty bit in L1
				}
				else if (in_L2) {
					int way_to_delete_L1 = lru_way(cache->L1, input_set_L1);
					Block* victim_L1 = &cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1];

					if (victim_L1->valid && victim_L1->dirty_bit) {
						// search for matching block in L2 (same set)
						for (int i = 0; i < cache->L2->ways_num; i++) {
							Block* blk_L2 = &cache->L2->blocks[input_set_L2 * cache->L2->ways_num + i];
							if (blk_L2->valid && blk_L2->tag == victim_L1->tag) {
								update_lru(cache->L2, input_set_L2, i); // update lru on L2
								blk_L2->dirty_bit = true;  // write back to L2 the deleted block from L1
								break;
							}
						}
					}

					insert_way(cache->L1, input_tag_L1, input_set_L1, way_to_delete_L1); //will call update lru on L1
					update_lru(cache->L2, input_set_L2, way_L2); //update lru on L2
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1].valid = true; // Mark the block as valid in L1, insert might do it too
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1].dirty_bit = true; // Update dirty bit in L1
					cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_L2].valid = true; // Mark the block as valid
				}
				else { //this is gonna be long....

					// insert into L2
					int way_to_delete_L2 = lru_way(cache->L2, input_set_L2);
					Block* victim_L2 = &cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_to_delete_L2];

					if (victim_L2->valid) {
						// invalidate matching L1 block (cuz inclusive)
						for (int i = 0; i < cache->L1->ways_num; i++) {
							Block* blk_L1 = &cache->L1->blocks[input_set_L1 * cache->L1->ways_num + i];
							if (blk_L1->valid && blk_L1->tag == victim_L2->tag) {
								if (blk_L1->dirty_bit == true) {
									// Writeback to L2
									victim_L2->dirty_bit = true;
									//now what the hell do i do with the dirty bit in L2?
									// was it all for nothing? what is the point of this? this is just a simulation...
									// what is even the point of life? what are we?
									// are we just prompts or are we humans? i refuse to belive we are just 0s and 1s!
									// i don't know... i just... i don't know anymore...
								}
								blk_L1->valid = false;
								break;
							}
						}
					}

					insert_way(cache->L2, input_tag_L2, input_set_L2, way_to_delete_L2); // insert to L2
					cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_to_delete_L2].valid = true;
					cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_to_delete_L2].dirty_bit = false; // since it's a write

					// insert into L1
					int way_to_delete_L1 = lru_way(cache->L1, input_set_L1);
					Block* victim_L1 = &cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1];

					if (victim_L1->valid && victim_L1->dirty_bit) {
						// Writeback to L2
						for (int i = 0; i < cache->L2->ways_num; i++) {
							Block* blk_L2 = &cache->L2->blocks[input_set_L2 * cache->L2->ways_num + i];
							if (blk_L2->valid && blk_L2->tag == victim_L1->tag) {
								update_lru(cache->L2, input_set_L2, i); // update lru on L2
								blk_L2->dirty_bit = true;
								break;
							}
						}
					}

					insert_way(cache->L1, input_tag_L1, input_set_L1, way_to_delete_L1); // insert to L1
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1].valid = true;
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_to_delete_L1].dirty_bit = true; // since it's a write
				}
			}
			
			// write no allocate
			else {
				if (in_L1){
					cache->L1->blocks[input_set_L1 * cache->L1->ways_num + way_L1].dirty_bit = true; // Update dirty bit in L1
					update_lru(cache->L1, input_set_L1, way_L1);
				}
				else if (in_L2) { // else cuz writeback
					cache->L2->blocks[input_set_L2 * cache->L2->ways_num + way_L2].dirty_bit = true; // Update dirty bit in L2
					update_lru(cache->L2, input_set_L2, way_L2);
				}
				else {
					//best case ever - write to memory, no need to update L1 or L2
				}
			}
		}
	}

	//calculate the miss rates and average access time with formula. use the global vars.
	double L1MissRate = (double)L1_misses / commands_num;
	double L2MissRate = (double)L2_misses / L1_misses; // L2 access are the same as L1 misses
	double avgAccTime = (double)(commands_num * L1Cyc + L1_misses * L2Cyc + L2_misses * MemCyc) / commands_num; 	// avg = (L1 access * L1 time + L2 access * L2 time + Mem access * Mem time) / commands_num

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	Cache_free(cache);
	return 0;
}

