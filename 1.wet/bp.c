/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


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
int get_index(uint32_t pc);
unsigned int get_tag(uint32_t pc);
int get_shared_idx_fsm(uint32_t pc, unsigned char history);
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,bool isGlobalHist, bool isGlobalTable, int Shared);
bool BP_predict(uint32_t pc, uint32_t *dst);
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void BP_GetStats(SIM_stats *curStats);

typedef struct {
    int prediction_arr[MAX_HISTORY_SIZE]; 
	//each index in the array matches a specific history
	//values: SNT WNT WT ST ( 0 1 2 3 )
} PredictionTable;

// BTB line (local history)
typedef struct {
    unsigned int line_pc;
	unsigned int tag;    // identifying the branch
	bool validation_bit;
	uint32_t target;
    unsigned char history_place; 
	PredictionTable* pred_t;
} BTB_line;

//global variable 
unsigned hist_mask;
unsigned global_history;
unsigned* global_fsm_table; //for GG/LG
BTB_line* BTB_table; //local histories
int BTB_size;
int flush_count;
int update_count;
int start_fsm_state;
int history_size;
int status; //type of branch predictor (LL=0, LG=1, GL=2, GG=3)
int tagsize;
int share_mod; //0 = none, 1 = lsb, 2 = mid

// get_index: takes pc adress and maps it to a btb line for the branch

//need to consider using share also - NEED FIX !!!
int get_index(uint32_t pc){
	int bits_num = log2(BTB_size);
	unsigned int main_mask = 0x0 - 1; //mask = 0xFF...
	main_mask = main_mask >> (32 - 2 - bits_num);
	int index = pc & main_mask;
	index = index >>2;
	return index;
}

//function to get index for fsm (depends on sharing mode)
int get_shared_idx_fsm(uint32_t pc, unsigned char history){
	int fsm_idx;
	int bits_shift;
	if(share_mod ==0){
	    bits_shift =0;
		fsm_idx = history & hist_mask; 
	}
	if(share_mod ==1){
	    bits_shift = 2;
		fsm_idx = (history^(pc>>bits_shift)) & hist_mask; 
	}
	if(share_mod ==2){
	    bits_shift = 16;
		fsm_idx = (history^(pc>>bits_shift)) & hist_mask; 
	}
	//DELETE
	//printf("real index for fsm: %d\n", fsm_idx);
	
	return fsm_idx;
}

//get_tag: take pc and returns tag
unsigned int get_tag(uint32_t pc){
	int num_zeros_left = 2 + log2(BTB_size); //number of bits to remove from the left
	unsigned int left_mask = (0x0-1) << num_zeros_left;
	
	int num_zeros_right = 32 - tagsize - num_zeros_left;
	uint32_t right_mask = ((uint32_t)0xFFFFFFFF) >> num_zeros_right;

	unsigned int tag = pc & left_mask & right_mask;
	tag = tag >> num_zeros_left;
	return tag;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	/* ===TO DO===
		> consider tag size
	*/

	//----config global variable----			
	BTB_size = btbSize; 
	tagsize =tagSize;
	start_fsm_state = fsmState;
	share_mod = Shared;
	history_size = historySize;
	update_count = 0;
	flush_count = 0;

	if(isGlobalHist && isGlobalTable){
		status = GG;
	}else if(isGlobalHist== 1 && isGlobalTable==0){
		status = GL;
	}else if(isGlobalHist == 0 && isGlobalTable ==1){
		status = LG;
	}else{
		status = LL;
	}

	//history mask: will help considering the right amount of history bits
	hist_mask = (1 << historySize) - 1; //will put zeros from the left

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
					BTB_table[i].pred_t->prediction_arr[j] = start_fsm_state;
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
			for(int i=0; i< BTB_size; i++){
				BTB_table[i].validation_bit = false;
				BTB_table[i].tag = 0;
				BTB_table[i].target = 0;
			}
			//why the BTB table isnt big?

			for(int j=0; j<MAX_HISTORY_SIZE; j++){
				global_fsm_table[j] = start_fsm_state;
			}

			break;
		
		case GG:
			//global history initiation
			global_history = 0b00000000;
			
			BTB_table = malloc(btbSize*sizeof(BTB_line));
			if(!BTB_table) return -1; //malloc failed

			//global fsm initiation
			global_fsm_table = malloc(MAX_HISTORY_SIZE*sizeof(unsigned));
			if(!global_fsm_table) return -1;
			
			// gil added, need to check with or: also we might need a for loop
			for(int i=0; i<BTB_size; i++){
				BTB_table[i].validation_bit = false;
				BTB_table[i].tag = 0;
				BTB_table[i].target = 0;
			}
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
					if(!BTB_table[i].pred_t)
						return 1;
					
					// gil added, need to check with or:
					BTB_table[i].validation_bit = false;
					BTB_table[i].tag = 0;
					BTB_table[i].target = 0;
					//perhaps also make the history to poinf for the global fsm?


					//filling the table with initial state
					for(int j=0; j<MAX_HISTORY_SIZE; j++){
						BTB_table[i].pred_t->prediction_arr[j] = start_fsm_state;
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
	printf("\n \n pred: val bit is = %d\n", BTB_table[index].validation_bit);

	//check if there is valid prediction
	if (BTB_table[index].tag == curr_tag && BTB_table[index].validation_bit == true){
		is_in = true;
	}
	
	if (is_in == false){
		*dst = pc + 4;
		return false;	
	} 	
	//we known its in BTB. now need to check if to jump or not. different check for every status	
	int shared_index;
	if (status == LL){
		shared_index = get_shared_idx_fsm (pc,BTB_table[index].history_place);
		prediction = BTB_table[index].pred_t->prediction_arr[shared_index];
		//prediction = BTB_table[index].pred_t[BTB_table[index].history_place & hist_mask];
	}
	else if (status == LG){
		shared_index = get_shared_idx_fsm (pc,BTB_table[index].history_place);
		prediction = global_fsm_table[shared_index];

		//prediction = global_fsm_table[BTB_table[index].history_place & hist_mask];
	}
	else if (status == GL){
		shared_index = get_shared_idx_fsm (pc,global_history);
		prediction = BTB_table[index].pred_t->prediction_arr[shared_index];
		//DELET
		printf("pred: LG global history is = %d\n", global_history);

		//prediction = BTB_table[index].pred_t[global_history & hist_mask];
	}
	else { //(status == GG)
		shared_index = get_shared_idx_fsm (pc,global_history);
		prediction = global_fsm_table[shared_index];
		//DELET
		//printf("pred: GG global history is = %d\n", global_history);

		//prediction = global_fsm_table[global_history & hist_mask];
	}
	//DELET
	printf("pred: shared index is = %d\n", shared_index);
	
	if (prediction == WT || prediction == ST){
		*dst = BTB_table[index].target;
		return true;
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
	PredictionTable* pred_table = BTB_table[index].pred_t; // perhaps pred_t->prediction_arr
	switch (status) {
		case GG:
			history_index = get_shared_idx_fsm (pc,global_history);
			//DELETE
			//printf("update: GG fsm befor is = %d\n", global_fsm_table[history_index]);
				if (taken && global_fsm_table[history_index] < ST)
					global_fsm_table[history_index]++;
				else if (!taken && global_fsm_table[history_index] > SNT)
					global_fsm_table[history_index]--;

			//DELETE
			//printf("update: GG fsm after is = %d\n", global_fsm_table[history_index]);

			// else{
			// 	global_fsm_table[history_index] = start_fsm_state;
			// }
			break;

		case GL:
			history_index = get_shared_idx_fsm (pc,global_history);
			//DELETE
			printf("update: GL fsm befor is = %d\n", pred_table->prediction_arr[history_index]);
			if (is_in == true){
				if (taken && pred_table->prediction_arr[history_index] < ST)
					pred_table->prediction_arr[history_index]++;
				else if (!taken && pred_table->prediction_arr[history_index] > SNT)
					pred_table->prediction_arr[history_index]--;
			}
			else{
				pred_table->prediction_arr[history_index] = start_fsm_state;
			}
			//DELETE
			printf("update: GL fsm after is = %d\n", pred_table->prediction_arr[history_index]);
			break;

		case LG:
			history_index = get_shared_idx_fsm (pc,BTB_table[index].history_place);
			//DELETE
			//printf("update: LG fsm befor is = %d\n", global_fsm_table[history_index]);
			//if (is_in == true){
				if (taken && global_fsm_table[history_index] < ST)
					global_fsm_table[history_index]++;
				else if (!taken && global_fsm_table[history_index] > SNT)
					global_fsm_table[history_index]--;
			//}

			//DELETE
			//printf("update: LG fsm after is = %d\n", global_fsm_table[history_index]);


			// else{
			// 	global_fsm_table[history_index] = start_fsm_state;
			// }
			break;

		case LL:
			history_index = get_shared_idx_fsm (pc,BTB_table[index].history_place);
			//DELETE
			//printf("update: LG fsm befor is = %d\n", pred_table->prediction_arr[history_index]);
			if (is_in == true){
				if (taken && pred_table->prediction_arr[history_index] < ST)
					pred_table->prediction_arr[history_index]++;
				else if (!taken && pred_table->prediction_arr[history_index] > SNT)
					pred_table->prediction_arr[history_index]--;
			}
			else{
				pred_table->prediction_arr[history_index] = start_fsm_state;
			}
			//DELETE
			//printf("update: LG fsm after is = %d\n", pred_table->prediction_arr[history_index]);
			break;
	}
	if ((is_in == false && taken == true) || (taken == true && pred_dst == pc+4) || (taken == false && pred_dst == targetPc) )
		flush_count++;

	//update BTB line
	BTB_table[index].tag = curr_tag;
	BTB_table[index].target = targetPc;
	BTB_table[index].validation_bit = true;
	BTB_table[index].line_pc = pc;
	
	// === Update History === not exactly sure how to do it, need to see what is going on
	
	
	
	if (status == GG || status == GL) {
		//DELETE
	printf("update: global history index befor is = %d\n", global_history);
		global_history = ((global_history << 1) | (taken ? 1 : 0)) & (hist_mask);
		//DELETE
	printf("update: history index after is = %d\n", global_history);
	} else {
		//DELETE
	printf("update: local history index befor is = %d\n", BTB_table[index].history_place);
		BTB_table[index].history_place = ((BTB_table[index].history_place << 1) | (taken ? 1 : 0)) & (hist_mask);
		//DELETE
		printf("update: history index after is = %d\n", BTB_table[index].history_place);
	}
	

}

	
void BP_GetStats(SIM_stats *curStats){

	curStats->br_num = update_count;
	curStats->flush_num = flush_count;

	//unsigned btb_size = 0;
	//unsigned int predic_tables_size = 0;
	//int byte_size = 8; //defiend because sizeof returns size in bytes

	//calculate size of structs (BTB + Prediction Tables). pc+val-2bits00: 32 + 1 - 2 = 31
	switch(status){
		case LL:
			//local history and fsm
			curStats->size = BTB_size*(tagsize + 31 + history_size + 2*(1 << history_size));
			//btb_size = byte_size*sizeof(BTB_line)*BTB_size; //ADD PADDING???
			//predic_tables_size = byte_size*MAX_HISTORY_SIZE*sizeof(unsigned)*BTB_size; //each branch has table

			//free willy
			for(int i=0; i<BTB_size; i++){
				//free fsm of each branch
				free(BTB_table[i].pred_t);
			}
			free(BTB_table);
			
			break;
		
		case LG:
			//local history, global fsm
			curStats->size = BTB_size*(tagsize + 31 + history_size) + 2*(1 << history_size);
			//btb_size = byte_size*sizeof(BTB_line)*BTB_size;
			//predic_tables_size = byte_size*MAX_HISTORY_SIZE*sizeof(unsigned); //one table


			//And I'm freeeeeee, free fallin'
			free(global_fsm_table);
			free(BTB_table);
			break;

		case GL:
			curStats->size = history_size + BTB_size*(tagsize + 31 + 2*(1 << history_size));
			//btb_size = byte_size*sizeof(BTB_line)*BTB_size;
			//predic_tables_size = byte_size*MAX_HISTORY_SIZE*sizeof(unsigned)*BTB_size;

			//I want to break freeee
			for(int i=0; i<BTB_size; i++){
				//free fsm of each branch
				free(BTB_table[i].pred_t);
			}
			free(BTB_table);
			break;
		
		case GG:
		curStats->size = history_size + 2*(1 << history_size) + BTB_size*(tagsize + 31);
		//btb_size = byte_size*sizeof(global_history);
		//predic_tables_size = byte_size*MAX_HISTORY_SIZE*sizeof(unsigned); //one table

		//FREEda Kahlo
		free(global_fsm_table);
		free(BTB_table);

		break;
	}
	
	//curStats->size = btb_size + predic_tables_size;
	return;
}
