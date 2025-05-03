/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"


//creating global BTB for access from every function


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	//----config global variable----			
	BTB_size = btbSize; 

	if(isGlobalHist && isGlobalTable){
		status = GG;
	}else if(isGlobalHist== 1 &&isGlobalTable==0){
		status = GL;
	}else if(isGlobalHist == 0 && isGlobalTable ==1){
		status = LG;
	}else{
		status = LL;
	}
	//--------
	

	//-----creating structs-----
	if(status == LL | status== LG){
		//creating BTB table: each branch has it's own history
		BTB_line* BTB_table = malloc(btbSize*sizeof(BTB_line));

		if(!BTB_table){
			return 1; //malloc fail
		}
	}

	//creatin fsm table/tables
	if(status == LL | status = GL){
		//creating fsm: local history tables
		for(int i = 0; i<btbSize; i++){
			BTB_table->i->pred_t = malloc(MAX_HISTORY_SIZE*sizeof(int));
			if(!BTB_table->i->pred_t){
				return 1;
			}
		}
	}

	//-------------------

			
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	bool is_in=false;
	int index = -1;
	for (int i=0; i<BTB_size; i++){
		if (pc == BTB_table[i].line_pc && BTB_table[i].validation_bit == true){
			index = i;
			is_in = true;
		}
	}
	if (is_in){ //known branch. now need to check if to jump or not. different check for every status
		if (status == LL){

		}
		else if (status == LG){

		}
		else if (status == GL){
			
		}
		else if (status == GG){
			
		}
	}
	*dst = pc + 4;
	return false;
}
	
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){



	
	return;
}

void BP_GetStats(SIM_stats *curStats){

	
	return;
}

