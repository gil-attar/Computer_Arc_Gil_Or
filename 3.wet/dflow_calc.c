/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#define MAX_REGSISTER 32 
int num_of_insts;

typedef struct {
    int  src1; //array index [0, num_of_insts -1]. if = -1 it means Entry (undependent)
    int  src2; // -||-
    int  weight; // #clock cycles
    bool end_of_branch;
} Instruction;


ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    num_of_insts = numOfInsts;

    /*
        analyzeProg will create an array of Instruction structs.
        The instructions are organized in the array "chronologically" InstructionGraph[0] refers to the first
        instruction in the given trace.
    */

    Instruction* InstructionNodes = malloc(num_of_insts*sizeof(Instruction));
    if(!InstructionNodes){
        fprintf(stderr, "failed malloc for struct ");
        return PROG_CTX_NULL;
    }

    int register_dependency[MAX_REGSISTER]; /*each idx represents a different register. the val will be 
                                            the serial idx of the last instruction who wrote to the register
                                            (immidiate dependency) */
    for (int reg = 0; reg< 32; reg++){
        register_dependency[reg] = -1; 
        //-1 default (Entry) for undependent registers
    } 

    for(int i = 0; i<num_of_insts; i++){

        const InstInfo* current_inst_info = &progTrace[i];

        //====fill nodes====
        InstructionNodes[i].weight = (int)opsLatency[(int)current_inst_info->opcode]; //extract weight
        InstructionNodes[i].end_of_branch = true; /*default: end of branch = true.
                                                    if the current inst has a source, then we know that
                                                    the prev inst (that generated the source) should be
                                                    change: end_of_branch = false.
                                                    We handle it later. */


        /*  each instructions depends on (max) two registers.
            for each src ,we extract the last instruction who writes to the specific register.
            InstructionNodes[i].src1 for example will contain the serial idx of the prev relevant instruction
        */

        InstructionNodes[i].src1 = register_dependency[current_inst_info->src1Idx];
        InstructionNodes[i].src2 = register_dependency[current_inst_info->src2Idx];

        if(InstructionNodes[i].src1 != -1){
            /* It means that the current instruction has a dependency.
               Therefor, the prev instruction (most recent in the relevant register_dependency index) is
               not a end of branch.
            */
            int inst_to_change_idx = register_dependency[current_inst_info->src1Idx];
            InstructionNodes[inst_to_change_idx].end_of_branch = false;
        }
        if(InstructionNodes[i].src2 != -1){

            int inst_to_change_idx = register_dependency[current_inst_info->src2Idx];
            InstructionNodes[inst_to_change_idx].end_of_branch = false;
        }

        /*  Update for register_dependency.
            We check which register the current intrunction in writing to.
            Then, we change the value of the relevant register in register_dependency to
            the recent command serial idx.
        */
        register_dependency[current_inst_info->dstIdx]= i;
    }

    //print array Image:
    //for(int i=0; i<num_of_insts; i++){
    //    printf("src 1: %d  src2: %d  weight: %d   end_of_branch %d\n", InstructionNodes[i].src1,
    //                            InstructionNodes[i].src2, InstructionNodes[i].weight, InstructionNodes[i].end_of_branch);
    //}

    return (ProgCtx)InstructionNodes;
}

void freeProgCtx(ProgCtx ctx) {
    free(ctx);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    if (ctx == NULL || theInst >= num_of_insts || theInst < 0) {
        return -1; // Invalid instruction index or no ctx provided
    }
    Instruction* InstructionNodes = (Instruction*)ctx; //casting to real type

    int way1 = 0, way2 = 0;

    if (InstructionNodes[theInst].src1 != -1){
        int parent = InstructionNodes[theInst].src1;
        way1 = getInstDepth(ctx, (unsigned)parent) + InstructionNodes[parent].weight; //Or chenged
    }

    if (InstructionNodes[theInst].src2 != -1){
        int parent = InstructionNodes[theInst].src2;
        way2 = getInstDepth(ctx, (unsigned)parent) + InstructionNodes[parent].weight;
    }
    int sub_path = (way1 > way2 ? way1 : way2);
    return sub_path; //Or changed
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    if (ctx == NULL || theInst >= num_of_insts || theInst < 0) {
        return -1; // Invalid instruction index or no ctx provided
    }

    Instruction* InstructionNodes = (Instruction*)ctx; //casting to realy type

    *src1DepInst = InstructionNodes[theInst].src1;
    *src2DepInst = InstructionNodes[theInst].src2;
    return 0;
}

int getProgDepth(ProgCtx ctx) {
    if (ctx == NULL) {
        return -1; // Invalid context provided
    }
    Instruction* InstructionNodes = (Instruction*)ctx; //casting to realy type
    int max_depth = 0;
    int cur_depth = 0;
    for (int i = 0; i < num_of_insts; ++i) {
        if (InstructionNodes[i].end_of_branch) {
            cur_depth = getInstDepth(ctx, i) + InstructionNodes[i].weight; // Or changed (here we want first weight)
            if (cur_depth > max_depth) {
                max_depth = cur_depth;
            }
        }
    }
    return max_depth;
}


