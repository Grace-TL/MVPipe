#ifndef MVKETCH_H
#define MVKETCH_H

#include <vector>
#include <unordered_set>
#include <utility>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "datatypes.hpp"
extern "C"
{
#include "hash.h"
}


extern uint64_t masks[NUM_MASKS];


class MVPipe {
    public: typedef struct SBUCKET_type { //Total sum V(i, j) int sum;
                
                int sum;

                long count;

                long value;

                KEYTYPE key;

            } SBucket;

            struct MV_type {

                //Counter to count total degree
                int sum;

                //Counter table of the diagnose line
                SBucket ****counts;

                //Outer sketch depth and width
                int depth;
                int* width;

                //# key word bits
                int lgn;

                int hash_shift;
                unsigned long *hash, *scale, *hardner;
            };




            MVPipe(int depth, int* width, int lgn, int hash_shift);

            ~MVPipe();

            void Update(int srow, int erow, int scol, KEYTYPE key, int value);

            int PointQuery(KEYTYPE key);

            void Query(int thresh, std::vector<std::pair<Pair, int > >&results);

            int GetCount();

            void Reset();

            MVPipe::SBucket**** GetTable();

            MVPipe::MV_type GetMV();
    
            MVPipe::SBucket* GetBucket(int i, int j, int k); 
    
    private:

            int CalBucket(KEYTYPE hierar_key, int mask); 

            void SetBucket(int row, int column, int sum, long count, KEYTYPE key);
            
            MV_type mv_;
};

#endif
