/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <math.h>

#define MAX_HISTORY_SIZE 256
//history, fsm
#define LL 0
#define LG 1
#define GL 2
#define GG 3
#define SNT 0
#define WNT 1
#define WT 2
#define ST 3

#define NONE 0
#define LSB 1
#define MID 2


//function declaretion
int get_index(uint32_t pc, unsigned btbSize);
unsigned int get_tag(uint32_t pc, unsigned btbSize);



typedef struct {
    unsigned prediction_arr[MAX_HISTORY_SIZE]; 
	//each index in the array matches a specific history
	//values: SNT WNT WT ST ( 0 1 2 3 )
} PredictionTable;

// BTB line (local history)
typedef struct {
    unsigned int line_pc;
	unsigned int tag;    // identifying the branch
	bool validation_bit;
	uint32_t target;
    char history_place; 
	PredictionTable* pred_t;
} BTB_line;

//global variable 
unsigned char hist_mask;
unsigned char global_history;
unsigned* global_fsm_table; //for GG/LG
BTB_line* BTB_table; //local histories
int BTB_size;
int flush_count;
int update_count;
int start_fsm_state

int status; //type of branch predictor (LL=0, LG=1, GL=2, GG=3)
int isShared; //0 = none, 1 = lsb, 2 = mid
int tagSize;


// get_index: takes pc adress and maps it to a btb line for the branch

//need to consider using share also - NEED FIX !!!
int get_index(uint32_t pc, unsigned btbSize){
	int bits_num = log2(btbSize);
	unsigned int main_mask = 0x0 - 1; //mask = 0xFF...
	
	main_mask = main_mask >> 32 - 2 - bits_num;
	int index = pc & main_mask;
	index = index >>2;
	return index;
}

//get_tag: take pc and returns tag
unsigned int get_tag(uint32_t pc, unsigned btbSize){
	int num_zeros_left = 2 + log2(btbSize) //number of bits to remove from the left
	unsigned int left_mask = (0x0-1) << num_zeros_left;
	
	int num_zeros_right = 32 - tagSize - num_zeros_left;
	uint32_t right_mask = ((uint32_t)0xFFFFFFFF) >> num_zeros_right;

	unsigned int tag = pc&left_mask&right_mask;
	tag = tag >> num_zeros_left;
	return tag;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	/* ===TO DO===
		> consider tag size
		> consider L/Gshare
		>check GL if correct
		>validation bit false
	*/

	//----config global variable----			
	BTB_size = btbSize; 
	isShared = Shared;
	tagSize =tagSize;
	start_fsm_state = fsmState;

	if(isGlobalHist && isGlobalTable){
		status = GG;
	}else if(isGlobalHist== 1 &&isGlobalTable==0){
		status = GL;
	}else if(isGlobalHist == 0 && isGlobalTable ==1){
		status = LG;
	}else{
		status = LL;
	}

	//history mask: will help considering the right amount of history bits
	hist_mask = 0b11111111 >> historySize; //will put zeros from the left
	//perhaps need to change to: hist_mask = (1 << historySize) - 1;

	//-----------------------------
	

	//creating structs
	switch(status){
		case LL:
			//creating BTB table
			BTB_table = malloc(btbSize*sizeof(BTB_line));
			if(!BTB_table) return 1; //mallic failed

			//creating fsm: local fsm table (one table for each branch)
			for(int i = 0; i<btbSize; i++){
				
				BTB_table[i].pred_t = malloc(MAX_HISTORY_SIZE*sizeof(unsigned));
				if(!BTB_table[i].pred_t) return 1;
				
				// gil added, need to check with or:
				BTB_table[i].validation_bit = false;
				BTB_table[i].tag = 0;
				BTB_table[i].target = 0;
				//

				//filling the table with initial state
				for(int j=0; j<MAX_HISTORY_SIZE; j++){
					BTB_table[i].pred_t[j] = fsmState;
				}
			}

			break;
		case LG:
			//creating BTB table
			BTB_table = malloc(btbSize*sizeof(BTB_line));
			if(!BTB_table) return -1; //mallic failed
			
			//creating fsm: global table
			global_fsm_table = malloc(MAX_HISTORY_SIZE*sizeof(unsigned));
			if(!global_fsm_table) return -1;

			// gil added, need to check with or: also we might need a for loop
			BTB_table[i].validation_bit = false;
			BTB_table[i].tag = 0;
			BTB_table[i].target = 0;
			// perhaps need without [0]
			//why the BTB table isnt big?

			for(int j=0; j<MAX_HISTORY_SIZE; j++){
				global_fsm_table[j] = fsmState;
			}

			break;
		
		case GG:
			//global history initiation
			global_history = 0b00000000;
			
			//global fsm initiation
			global_fsm_table = malloc(MAX_HISTORY_SIZE*sizeof(unsigned));
			if(!global_fsm_table) return -1;
			
			// gil added, need to check with or: also we might need a for loop
			BTB_table[i].validation_bit = false;
			BTB_table[i].tag = 0;
			BTB_table[i].target = 0;
			// perhaps need without [0]
			// perhaps also make the history to poinf for the global fsm?
			//why there is no BTB table?


			for(int j=0; j<MAX_HISTORY_SIZE; j++){
				global_fsm_table[j] = fsmState;
			}

			break;
	
		case GL: 
				//global history initiation
				global_history = 0b00000000;

				//creating BTB table in order to map each branch to it's BTB
				//we won't consider local histories
				BTB_table = malloc(btbSize*sizeof(BTB_line));
				if(!BTB_table) return -1; //malloc failed
				
				//local fsm initiation
				for(int i = 0; i<btbSize; i++){
					BTB_table[i].pred_t = malloc(MAX_HISTORY_SIZE*sizeof(unsigned));
					if(!BTB_table[i].pred_t) return 1;
					
					// gil added, need to check with or:
					BTB_table[i].validation_bit = false;
					BTB_table[i].tag = 0;
					BTB_table[i].target = 0;
					//perhaps also make the history to poinf for the global fsm?


					//filling the table with initial state
					for(int j=0; j<MAX_HISTORY_SIZE; j++){
						BTB_table[i].pred_t[j] = fsmState;
					}
				}

			break;
	}

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	bool is_in=false;
	int index = get_index(pc);
	int curr_tag = get_tag(pc);
	int prediction;
	//check if there is valid prediction
	if (BTB_table[index].tag != NULL){
		if (BTB_table[index].tag == curr_tag && BTB_table[index].validation_bit == true){
			is_in = true;
		}
	}
	if (is_in == false){
		*dst = pc + 4;
		return false;	
	} 	
	//we known its in BTB. now need to check if to jump or not. different check for every status	
	//hist_mask - consider it who which place to go in the table
	//this syntax is 99% bad... theoretical: btb -> history -> correct line in history
	if (status == LL){
		prediction = BTB_table[index].pred_t[BTB_table[index].history_place & hist_mask];
	}
	if (status == LG){
		prediction = global_fsm_table[BTB_table[index].history_place & hist_mask];
	}
	if (status == GL){
		prediction = BTB_table[index].pred_t[global_history & hist_mask];
	}
	if (status == GG){
		prediction = global_fsm_table[global_history & hist_mask];
	}

	if (prediction == WT || prediction == ST){
		*dst = BTB_table[index].target;
		return true
	}
	else{
		*dst = pc + 4;
		return false;
	}
}


	//first ver, might come in handy
	/*
	if (status == LL &&
			(BTB_table[index].pred_t[BTB_table[index].(history_place&hist_mask)] == ST || BTB_table[index].pred_t[BTB_table[index].(history_place&hist_mask)] == WT)){  //perharps syntax isnt good
			*dst = BTB_table[index].target;
			return true;
		}
		//local hist global table
		else if (status == LG &&
			(global_fsm_table[BTB_table[index].(history_place&hist_mask)] == ST || global_fsm_table[BTB_table[index].(history_place&hist_mask)] == WT)){ 
			*dst = BTB_table[index].target;
			return true;
		}
		// global hist local table
		else if (status == GL &&
			(BTB_table[index].pred_t[global_history&hist_mask] == ST || BTB_table[index].pred_t[global_history&hist_mask] == WT)){	//perharps syntax isnt good
			*dst = BTB_table[index].target;
			return true;
		}
		else if (status == GG &&
			(global_fsm_table[global_history&hist_mask] == ST || global_fsm_table[global_history&hist_mask] == WT)){
			*dst = BTB_table[index].target;
			return true;
		}
	}
	*/

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
	update_count++;

	int index = get_index(pc);
	unsigned int curr_tag = get_tag(pc);
	bool is_in = false;
	if (curr_tag == BTB_table[index].tag && BTB_table[index].validation_bit == true)
		is_in =true;
	
	int history_index;
	PredictionTable* pred_table = BTB_table[index].pred_t;
	switch (status) {
		case GG:
			history_index = global_history & hist_mask;
			if (is_in == true){
				if (taken && global_fsm_table[history_index] < ST)
					global_fsm_table[history_index]++;
				else if (!taken && global_fsm_table[history_index] > SNT)
					global_fsm_table[history_index]--;
			}
			else{
				global_fsm_table[history_index] = start_fsm_state;
			}
			break;

		case GL:
			history_index = global_history & hist_mask;
			if (is_in == true){
				if (taken && pred_table[history_index] < ST)
					pred_table[history_index]++;
				else if (!taken && pred_table[history_index] > SNT)
					pred_table[history_index]--;
			}
			else{
				pred_table[history_index] = start_fsm_state;
			}
			break;

		case LG:
			history_index = BTB_table[index].history_place & hist_mask;
			if (is_in == true){
				if (taken && global_fsm_table[history_index] < ST)
					global_fsm_table[history_index]++;
				else if (!taken && global_fsm_table[history_index] > SNT)
					global_fsm_table[history_index]--;
			}
			else{
				global_fsm_table[history_index] = start_fsm_state;
			}
			break;

		case LL:
			history_index = BTB_table[index].history_place & hist_mask;
			if (is_in == true){
				if (taken && pred_table[history_index] < ST)
					pred_table[history_index]++;
				else if (!taken && pred_table[history_index] > SNT)
					pred_table[history_index]--;
			}
			else{
				pred_table[history_index] = start_fsm_state;
			}
			break;
	}
	if (is_in == false || (taken == true && pred_dst = pc+4) || (taken == false && pred_dst = targetPc) )
		flush_count++;

	//update BTB line
	BTB_table[index].tag = curr_tag;
	BTB_table[index].target = targetPc;
	BTB_table[index].validation_bit = true;
	BTB_table[index].line_pc = pc;
	
	// === Update History === not exactly sure how to do it, need to see what is going on
	if (status == GG || status == GL) {
		global_history = ((global_history << 1) | (taken ? 1 : 0)) & ((1 << MAX_HISTORY_SIZE) - 1);
	} else {
		BTB_table[index].history_place = ((BTB_table[index].history_place << 1) | (taken ? 1 : 0)) & ((1 << MAX_HISTORY_SIZE) - 1);
	}
}

	
/*	unsigned int target_tag = get_tag(pc)
	int target_index = get_index(pc)

	BTB_table[target_index].validation_bit=true;
	BTB_table[target_index].target = targetPc;
	
	if (status == GG){
		if (taken && pred_dst == targetPc){ //was right
			if(global_fsm_table[global_history&hist_mask] != ST){
				global_fsm_table[global_history&hist_mask]++;
				global_history << 1
				global_history += 1;
			}
		}
		else if(!taken && pred_dst == targetPc){ //was wrong
		
		} else if(taken && pred_dst != targetPc){ //was wrong

		} else if(!taken && pred_dst != targetPc){ //was right
		
		} else if{
			
		}
		BTB_table[index].



	
	return;
}
*/


void BP_GetStats(SIM_stats *curStats){

	
	return;
}

