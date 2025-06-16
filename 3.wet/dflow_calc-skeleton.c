/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
int num_of_insts;

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    num_of_insts = numOfInsts;

    //FEW NOTES FOR OR:
    // look at the names in the struct: src1, src2, is_independent, wight, etc.....
    // i made num_of_insts global, i already put it in the code above
    // the ctx is an array of structs. here is a short example from GPT on how to make it (so make sure you dont do a pointers array):
    
    /*
    typedef struct {
    int src1;
    int src2;
    int weight;
    // ... maybe other fields
    } Instruction;

     typedef Instruction* ProgCtx;  // ctx points to the first element in an array of structs
    */




    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
}

//done
int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    if (ctx == NULL || theInst >= num_of_insts || theInst < 0) {
        return -1; // Invalid instruction index or no ctx provided
    }
    int way1 = 0;
    int way2 = 0;    
    if (ctx[theInst].src1 != -1){
        way1 = getInstDepth(ctx, ctx[theInst].src1);
    }
    if (ctx[theInst].src2 != -1){
        way2 = getInstDepth(ctx, ctx[theInst].src2);
    }
    return ctx[theInst].weight + max(way1, way2);
}

//done
int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    if (ctx == NULL || theInst >= num_of_insts || theInst < 0) {
        return -1; // Invalid instruction index or no ctx provided
    }
    *src1DepInst = ctx[theInst].src1;
    *src2DepInst = ctx[theInst].src2;
    return 0;
}

// done
int getProgDepth(ProgCtx ctx) {
    int max_depth = 0;
    int cur_depth = 0;
    for (int i = 0; i < num_of_insts; ++i) {
        if (ctx[i].is_independent) {
            cur_depth = getInstDepth(ctx, i);
            if (cur_depth > max_depth) {
                max_depth = cur_depth;
            }
        }
    }
    return max_depth;
}


