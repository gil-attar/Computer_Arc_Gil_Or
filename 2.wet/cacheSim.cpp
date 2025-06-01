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

//declare functions
//creates Cache struct and returns a pointer to it
Cache* Cache_init(unsigned block_size, unsigned L1_size, unsigned L2_size,
					unsigned L1_assoc, unsigned L2_assoc, unsigned L1_clocks,
					unsigned L2_clocks, unsigned write_alloc);
void update_lru(unsigned int** lru, int set_index, int accessed_way, int ways_num);
int lru_way(unsigned int** lru, int set_index, int ways_num);
void create_masks(); 




typedef struct{
	unsigned int adress; //adress of first item in the block
	unsigned int tag;
	bool dirty_bit;
	bool valid; //valid bit - true if the block is valid, false if it is not

	//gil documented
	//unsigned int set;
} Block;


typedef struct{
//cache level (L1/L2) struct holds a pointer to an array of blocks.
// set 0: blocks[0,...,#W-1] | set1: blocks[#W,..,2#W-1]
	Block* blocks;
	unsigned ways_num; //assoc_level
	unsigned int set_mask; 
	unsigned int tag_mask;
	int set_bits;
	int tag_bits;
	//gil documented
	//unsigned write_allocate; // false: No Write Allocate | true: Write Allocate
	//double miss_rate; //to print use: %.03f
	//int access_time; //access time in clock cycles units
	//LRU* lru; //each cache level has it's own LRU
	int blocks_num;
	unsigned int** lru;
} Cache_Level;

typedef struct{
	Cache_Level* L1;
	Cache_Level* L2;
} Cache;

void update_lru(Cache_Level* cache, int set_index, int accessed_way) {
    int* lru_set = cache->lru[set_index];
    int base = set_index * cache->ways_num;
    int old_value = lru_set[accessed_way];

    for (int i = 0; i < cache->ways_num; i++) {
        if (i == accessed_way) continue;
        if (!cache->blocks[base + i].valid) continue; // only valid blocks participate

        if (lru_set[i] > old_value) {
            lru_set[i]--;
        }
    }

    lru_set[accessed_way] = cache->ways_num - 1;
}

int lru_way(Cache_Level* cache, int set_index) {
    int base = set_index * cache->ways_num;

    // look for invalid block (empty spot)
    for (int i = 0; i < cache->ways_num; i++) {
        if (!cache->blocks[base + i].valid) {
            return i;
        }
    }

    // return block with LRU = 0
    for (int i = 0; i < cache->ways_num; i++) {
        if (cache->lru[set_index][i] == 0) {
            return i;
        }
    }

    return -1; // should not happen
}


//------------------------------------------------------------------------------------------------------
//NEED
int search_way(Cache_Level* level, unsigned int tag, unsigned int set_index) {
	// Search for the tag in the specified set
	for (int i = 0; i < level->ways_num; i++) {
		if (level->blocks[set_index * level->ways_num + i].tag == tag) {
			// Update LRU
			update_lru(level->lru, set_index, i, level->ways_num);
			return i; // Found in this way
		}
	}
	return -1; // Not found
}

//NEEDS TO CALL UPDATE LRU + SET VALID AND DIRTY BITS
int insert_way(Cache_Level* level, unsigned int tag, unsigned int set_index, unsigned int address) { 
	// Insert a new way with the specified tag in the specified set
	for (int i = 0; i < level->ways_num; i++) {
		if (level->blocks[set_index * level->ways_num + i].tag == 0) {
			// Found an empty way, insert here
			level->blocks[set_index * level->ways_num + i].tag = tag;
			level->blocks[set_index * level->ways_num + i].adress = address;
			level->blocks[set_index * level->ways_num + i].dirty_bit = false; // Initially not dirty
			update_lru(level->lru, set_index, i, level->ways_num);
			return i; // Inserted successfully
		}
	}
	// If no empty way found, we need to evict a way
	int way_to_evict = lru_way(level->lru, set_index, level->ways_num);
	if (way_to_evict != -1) {
		// Evict the way
		level->blocks[set_index * level->ways_num + way_to_evict].tag = tag;
		level->blocks[set_index * level->ways_num + way_to_evict].adress = address;
		level->blocks[set_index * level->ways_num + way_to_evict].dirty_bit = false; // Initially not dirty
		update_lru(level->lru, set_index, way_to_evict, level->ways_num);
		return way_to_evict; // Evicted and inserted successfully
	}
	return -1; // Error, no way could be inserted
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

	while (getline(file, line)) {
		commands_num++;
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		//------------------------------------------------------------------------------------------------------

		//
		//
		// FOR OR:
		// line is processed. it is in stringstream ss. need to parse it and put info in vars (can make functions to handle)
		// unsigned int input_block_address = 0; // address of the block
		// int read_write; // 0 for read, 1 for write
		// unsigned int input_tag;
		// unsigned int input_set; // will be the index in the blocks array
		// call init and make a cache struct
		//
		//
		
		//------------------------------------------------------------------------------------------------------


		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;


		bool in_L1 = true; // is the block in L1?
		bool in_L2 = true; // is the block in L2?
		
		//call gils functions:
		int way_L1 = search_way(cashe->L1); //fix inside ()
		int way_L2 = search_way(cashe->L2);	//fix inside ()
		if (way_L1 == -1){
			in_L1 = false; // not in L1
			L1_misses++;
			if (way_L2 == -1) {
				in_L2 = false; // not in L2
				L2_misses++;
			}
		}

		if (read_write == 0){ //read
			// just need to update lru - this is a simulation [inside of a simulation! (inside of another simulation!)].
			// oh i forgot, i also need to bring the block to the cache if it is not there...
			if (in_L1) {
				update_lru(cashe->L1->lru, input_set, way_L1, cashe->L1->ways_num);
				cashe->L1->blocks[input_set * cashe->L1->ways_num + way_L1].valid = true; // Mark the block as valid
			}
			else if (in_L2) {
				int way_to_delete_L1 = lru_way(cashe->L1->lru, input_set, cashe->L1->ways_num);
				insert_way(cashe->L1, input_tag, input_set, way_to_delete_L1); //will call update lru on L1
				update_lru(cashe->L2->lru, input_set, way_L2, cashe->L2->ways_num); //update L2
				cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1].valid = true; // Mark the block as valid in L1, insert might do it too
				cashe->L2->blocks[input_set * cashe->L2->ways_num + way_L2].valid = true; // Mark the block as valid
				

			}
			else {
   				int way_to_delete_L2 = lru_way(cashe->L2->lru, input_set, cashe->L2->ways_num);
    			Block* victim_L2 = &cashe->L2->blocks[input_set * cashe->L2->ways_num + way_to_delete_L2];
				// if need to delete from L2, then also delete from L1 (if it exists)
				if (victim_L2->valid) {
					for (int i = 0; i < cashe->L1->ways_num; i++) {
						Block* blk = &cashe->L1->blocks[input_set * cashe->L1->ways_num + i];
						if (blk->valid && blk->tag == victim_L2->tag) {
							blk->valid = false;
							break;
						}
					}
				}
				
				// insert block to L2
				insert_way(cashe->L2, input_tag, input_set, way_to_delete_L2); // will call update lru on L2
				cashe->L2->blocks[input_set * cashe->L2->ways_num + way_to_delete_L2].valid = true; // insert might do it too

				// insert block to L1
				int way_to_delete_L1 = lru_way(cashe->L1->lru, input_set, cashe->L1->ways_num);
				insert_way(cashe->L1, input_tag, input_set, way_to_delete_L1); // will call update lru on L1
				cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1].valid = true; // insert might do it too
			}

		}

		else { //write
			// write allocate
			if (is_write_allocate) {
				if (in_L1) {
					update_lru(cashe->L1->lru, input_set, way_L1, cashe->L1->ways_num);
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_L1].valid = true; // Mark the block as valid
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_L1].dirty_bit = true; // Update dirty bit in L1
				}
				else if (in_L2) {
					int way_to_delete_L1 = lru_way(cashe->L1->lru, input_set, cashe->L1->ways_num);
					Block* victim_L1 = &cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1];

					if (victim_L1->valid && victim_L1->dirty_bit) {
						// search for matching block in L2 (same set)
						for (int i = 0; i < cashe->L2->ways_num; i++) {
							Block* blk_L2 = &cashe->L2->blocks[input_set * cashe->L2->ways_num + i];
							if (blk_L2->valid && blk_L2->tag == victim_L1->tag) {
								blk_L2->dirty_bit = true;  // write back to L2 the deleted block from L1
								break;
							}
						}
					}
					insert_way(cashe->L1, input_tag, input_set, way_to_delete_L1); //will call update lru on L1
					update_lru(cashe->L2->lru, input_set, way_L2, cashe->L2->ways_num); //update lru on L2
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1].valid = true; // Mark the block as valid in L1, insert might do it too
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1].dirty_bit = true; // Update dirty bit in L1
					cashe->L2->blocks[input_set * cashe->L2->ways_num + way_L2].valid = true; // Mark the block as valid
				}
				else { //this is gonna be long....

					// insert into L2
					int way_to_delete_L2 = lru_way(cashe->L2->lru, input_set, cashe->L2->ways_num);
					Block* victim_L2 = &cashe->L2->blocks[input_set * cashe->L2->ways_num + way_to_delete_L2];

					if (victim_L2->valid) {
						// invalidate matching L1 block (cuz inclusive)
						for (int i = 0; i < cashe->L1->ways_num; i++) {
							Block* blk_L1 = &cashe->L1->blocks[input_set * cashe->L1->ways_num + i];
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

					insert_way(cashe->L2, input_tag, input_set, way_to_delete_L2); // insert to L2
					cashe->L2->blocks[input_set * cashe->L2->ways_num + way_to_delete_L2].valid = true;
					cashe->L2->blocks[input_set * cashe->L2->ways_num + way_to_delete_L2].dirty_bit = false; // since it's a write

					// insert into L1
					int way_to_delete_L1 = lru_way(cashe->L1->lru, input_set, cashe->L1->ways_num);
					Block* victim_L1 = &cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1];

					if (victim_L1->valid && victim_L1->dirty_bit) {
						// Writeback to L2
						for (int i = 0; i < cashe->L2->ways_num; i++) {
							Block* blk_L2 = &cashe->L2->blocks[input_set * cashe->L2->ways_num + i];
							if (blk_L2->valid && blk_L2->tag == victim_L1->tag) {
								blk_L2->dirty_bit = true;
								break;
							}
						}
					}
					insert_way(cashe->L1, input_tag, input_set, way_to_delete_L1); // insert to L1
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1].valid = true;
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_to_delete_L1].dirty_bit = true; // since it's a write
				}
			}
			
			// write no allocate
			else {
				if (in_L1){
					cashe->L1->blocks[input_set * cashe->L1->ways_num + way_L1].dirty_bit = true; // Update dirty bit in L1
				}
				else if (in_L2) { // else cuz writeback
					cashe->L2->blocks[input_set * cashe->L2->ways_num + way_L2].dirty_bit = true; // Update dirty bit in L2
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

	return 0;
}


Cache* Cache_init(unsigned block_size, unsigned L1_size, unsigned L2_size,
				  unsigned L1_assoc, unsigned L2_assoc, unsigned L1_clocks,unsigned L2_clocks,
				  unsigned write_alloc){
/*the function returns pointer to a Cache struct. 
 block_size:  <log2(size)>
 L1_size, L2_size : <log2(size)>
 L1_assoc, L2_assic: <number of ways>
*/

		/* TO DO:
		  - check if blocks_num is true
		  - add LRU to cache levels
		  -add error handle
		*/

	Cache_Level* L1_p = malloc(sizeof(Cache_Level));
	Cache_Level* L2_p = malloc(sizeof(Cache_Level));

	L1_p->blocks_num = (L1_size-block_size); //CHECK THIS!!! perhaps use: 1 << L1_size, might not need to devide by 8
	L2_p->_blocks_num = (L2_size -block_size); //CHECK THIS!!!; perhaps use: 1 << L2_size, might not need to devide by 8

	L1_p->blocks = malloc(L1_p->blocks_num*sizeof(Block));
	L2_p->blocks = malloc(L2_p->blocks_num*sizeof(Block));
	
	//gil added
	for (int i = 0; i < L1_p->blocks_num; i++) {
    	L1_p->blocks[i].valid = false; // Mark as empty
		L1_p->blocks[i].dirty_bit = false; // Initially not dirty
	}
	for (int i = 0; i < L2_p->blocks_num; i++) {
    	L2_p->blocks[i].valid = false; // Mark as empty
		L2_p->blocks[i].dirty_bit = false; // Initially not dirty
	}


	//gil added
	is_write_allocate = write_alloc;
	
	//gil documented
	//L1_p->write_allocate = write_alloc;
	//L2_p->write_allocate = write_alloc;
	
	//gil documented
	//L1_p->miss_rate = 0;
	//L2_p->miss_rate = 0;

	L1_p->ways_num = L1_assoc;
	L2_p->ways_num = L2_assoc;
	
	//gil documented
	//L1_p->access_time = L1_clocks;
	//L2_p->access_time = L2_clocks;

	//do we need to set everything to null??? - gil added


	// LRU allocation and init - might need to use these 2 lines, compiler can do problems with malloc
    //unsigned int num_sets_L1 = L1_p->blocks_num / L1_p->ways_num;
    //unsigned int num_sets_L2 = L2_p->blocks_num / L2_p->ways_num;

    L1_p->lru = malloc(L1_p->blocks_num * sizeof(unsigned int*));
    for (int i = 0; i < L1_p->blocks_num; i++) {
        L1_p->lru[i] = malloc(L1_p->ways_num * sizeof(unsigned int));
        for (int j = 0; j < L1_p->ways_num; j++) {
            L1_p->lru[i][j] = j;
        }
    }

    L2_p->lru = malloc(L2_p->blocks_num * sizeof(unsigned int*));
    for (int i = 0; i < L2_p->blocks_num; i++) {
        L2_p->lru[i] = malloc(L2_p->ways_num * sizeof(unsigned int));
        for (int j = 0; j < L2_p->ways_num; j++) {
            L2_p->lru[i][j] = j;
        }
    }	
	
	Cache* Cache_p = malloc(sizeof(Cache));
	Cache_p->L1 = L1_p;
	Cache_p->L2 = L2_p;
	
	return Cache_p;
}

void create_masks(unsigned block_bits, unsigned chache_bits, unsigned assoc, Cache_Level* cache){
	//function that gets user's parameters and pointer to cache.
	//determines number of bits of set, tag and offset.
	//it changes the masks to enable us to extract set and tag from each adress

	//find number of offset bits 
	cache->offset_bits = block_bits; //block_size =block_bits = log2(block_size)

	
	//find number of set bits and change masks accordingly
	cache->set_bits = cache_bits - block_bits - assoc_bits; //chache_bits = cache_size = log2(c_Size)
	cache->tag_bits = cache->offset_bits +cache->set_bits;
	
	cache->tag_mask_p = (0x0 - 1) << (cache->set_bits+ cache->offset_bits); //0x0-1 = FFFF...
	cache->set_mask = ((1u<<cache->set_bits) - 1u)<<cache->offset_bits;


}


function get_tag()

function get_set(){
	///unsigned set_index = (addr & set_mask) >> offset_bits;
	//unsigned tag  = (addr & tag_mask) >> (offset_bits + set_bits);
}