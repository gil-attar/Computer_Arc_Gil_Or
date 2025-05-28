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

//creates Cache struct and returns a pointer to it


Cache* Cache_init(unsigned block_size, unsigned L1_size, unsigned L2_size,
					unsigned L1_assoc, unsigned L2_assoc, unsigned L1_clocks
					unsigned L2_clocks, unsigned write_alloc);


typedef struct{
	unsigned int adress; //adress of first item in the block
	unsigned int tag;
	unsigned int set;
	bool dirty_bit;
} Block;


typedef struct{
//cache level (L1/L2) struct holds a pointer to an array of blocks.
// set 0: blocks[0,...,#W-1] | set1: blocks[#W,..,2#W-1]
	Block* blocks;
	unsigned write_allocate; // false: No Write Allocate | true: Write Allocate
	double miss_rate; //to print use: %.03f
	unsigned ways_num; //assoc_level
	int access_time; //access time in clock cycles units
	LRU* lru; //each cache level has it's own LRU
} Cache_Level;

typedef struct{
	Cache_Level* L1;
	Cache_Level* L2;
} Cache;

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

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

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

		int L1_blocks_num = (2^L1_size)/(8*2^block_size); //CHECK THIS!!!
		int L2_blocks_num = (2^L2_size)/(8*2^block_size); //CHECK THIS!!!;

		L1_p->blocks = malloc(L1_blocks_num*sizeof(Block));
		L2_p->blocks = malloc(L2_blocks_num*sizeof(Block));
	
		L1_p->write_allocate = write_alloc;
		L2_p->write_allocate = write_alloc;
		
		L1_p->miss_rate = 0;
		L2_p->miss_rate = 0;

		L1_p->ways_num = L1_assoc;
		L2_p->ways_num = L2_assoc;
		
		L1_p->access_time = L1_clocks;
		L2_p->access_time = L2_clocks;

		//====NEED TO DEFINE LRU====
		//L1_p->lru = ;
		//L2_p->lru = ;
	
		Cache* Cache_p = malloc(sizeof(Cache));
		Cache_p->L1 = L1_p;
		Cache_p->L2 = L2_p;

		return Cache_p;

}

