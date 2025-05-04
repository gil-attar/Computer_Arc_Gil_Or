/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

// get_index: takes pc adress and maps it to a btb line for the branch
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

//need to consider using share also
bool BP_predict(uint32_t pc, uint32_t *dst){
	bool is_in=false;
	int index = -1;
	for (int i=0; i<BTB_size; i++){
		//perhaps the if condition need to consider share mid/lsb/notshare. easy fix if so - split to 3 ifs and manuver the pc value to go for the correct tag and thats how we get the index
		if (BTB_table[i] != NULL && get_tag(pc) == BTB_table[i].tag && BTB_table[i].validation_bit == true){ //perhaps the null check in a line befor
			index = i;
			is_in = true;
		}
	}
	//hist_mask - consider it who which place to go in the table
	if (is_in){ //we known its branch. now need to check if to jump or not. different check for every status
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
	*dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
	update_count++;
	unsigned int target_tag = get_tag(pc)
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

void BP_GetStats(SIM_stats *curStats){

	
	return;
}

