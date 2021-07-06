#include "mvpipe.hpp"
#include <climits>
#ifdef HASH31
#include "../hhh/lib/prng.h"
#endif

MVPipe::MVPipe(int depth, int* width, int lgn, int hash_shift) {

    mv_.depth = depth;
    mv_.width = width;
    mv_.lgn = LGN;
    //denote the row that using hash function to hash keys to the buckets
    mv_.hash_shift = hash_shift;
    mv_.sum = 0;
    mv_.counts = new SBucket **[depth];
    for (int i = 0; i < depth; i++) {
        mv_.counts[i] = (SBucket**)calloc(width[i], sizeof(SBucket*));
        for (int j = 0; j < width[i]; j++) {
            mv_.counts[i][j] = (SBucket*)calloc(1, sizeof(SBucket));
            memset(mv_.counts[i][j], 0, sizeof(SBucket));
        }
    }

    mv_.hash = new unsigned long[depth];
    mv_.scale = new unsigned long[depth];
    mv_.hardner = new unsigned long[depth];
    char name[] = "MVPipe";
    unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
    for (int i = 0; i < depth; i++) {
        mv_.hash[i] = GenHashSeed(seed++);
    }
    for (int i = 0; i < depth; i++) {
        mv_.scale[i] = GenHashSeed(seed++);
    }
    for (int i = 0; i < depth; i++) {
        mv_.hardner[i] = GenHashSeed(seed++);
    }
}

MVPipe::~MVPipe() {
    for (int i = 0; i < mv_.depth; i++) {
        for (int j = 0; j < mv_.width[i]; j++) {
            free(mv_.counts[i][j]);
        }
        free(mv_.counts[i]);
    }
    delete [] mv_.hash;
    delete [] mv_.scale;
    delete [] mv_.hardner;
    delete [] mv_.counts;
}

//original update
//row: means insert start from the row
void MVPipe::Update(int row, KEYTYPE key, int val) {
    unsigned long bucket = 0;
    int keylen = LGN;
    // if key is not in the i-th row, then insert it in the i+1-th row
    // depth equals to the hierarchy level
    KEYTYPE tempkey = key;
    int i = 0;
    for (i = row; i < mv_.depth; i++) {
        if (val == 0) break;
        KEYTYPE hierar_key = tempkey & masks[i];
        if (i < mv_.hash_shift)
            bucket = MurmurHash2((unsigned char*)&hierar_key, keylen, mv_.hardner[i]) % mv_.width[i];
        else {
            bucket = hierar_key >> (i*33/(NUM_MASKS-1));
        }
        MVPipe::SBucket *sbucket = mv_.counts[i][bucket];
        sbucket->sum += val;
        if(hierar_key == sbucket->key) {
            sbucket->count += val;
            sbucket->value += val;
            break;
        } else {
            sbucket->count -= val;
            if (sbucket->count < 0) {
                //send the original candidate key to the next row
                tempkey = sbucket->key;
                long tempval = sbucket->value;
                sbucket->value = val;
                val = tempval;
                sbucket->key = hierar_key;
                sbucket->count = -sbucket->count;
            }
        }
    }
}

void MVPipe::Query(int thresh, std::vector<std::pair<unsigned long, int> >&results) {

    //1. push all keys in the low level to high level if its counts do not
    //exceed the threshold

    for (int i = 0; i < mv_.depth; i++) {
        for (int j = 0; j < mv_.width[i]; j++) {
            KEYTYPE key = mv_.counts[i][j]->key;
            int resval = 0;
            resval = (mv_.counts[i][j]->sum + mv_.counts[i][j]->count)/2;
            //check false postive
            int bucket = 0;
            int addval = mv_.counts[i][j]->value;
            for (int r = 0; r < 4; r++) {
                if (i+1+r >= mv_.depth) break;
                KEYTYPE rkey = key & masks[i+1+r];
                if (i+1+r < mv_.hash_shift)
                    bucket = MurmurHash2((unsigned char*)&rkey, LGN, mv_.hardner[i+1+r]) % mv_.width[i+1+r];
                else {
                    bucket = rkey >> ((i+1+r)*33/(NUM_MASKS-1));
                }
                int est = 0;
                if (rkey == mv_.counts[i+1+r][bucket]->key) {
                    est = (mv_.counts[i+1+r][bucket]->sum + mv_.counts[i+1+r][bucket]->count)/2 + addval;
                    addval += mv_.counts[i+1+r][bucket]->value;
                } else {
                    est = (mv_.counts[i+1+r][bucket]->sum - mv_.counts[i+1+r][bucket]->count)/2 + addval;
                }
                resval = resval > est ? est : resval;
            }


            if (resval > thresh) {
                //add back descendant
                for (auto it = results.begin(); it != results.end(); it++) {
                    //if it is descendants
                    KEYTYPE dkey = it->first >> 32;
                    int p = it->first & 0xffffffff;
                    int bucket = 0;
                    if (((dkey&masks[i]) == key) && ((masks[i]&masks[p]) == masks[i])) {
                        if (p < mv_.hash_shift)
                            bucket = MurmurHash2((unsigned char*)&dkey, LGN, mv_.hardner[p]) % mv_.width[p];
                        else {
                            bucket = dkey >> (p*33/(NUM_MASKS-1));
                        }
                        resval += mv_.counts[p][bucket]->value;
                    }
                }
                std::pair<unsigned long, int> node;
                node.first = key;
                node.first <<= 32;
                node.first |= i;
                node.second = resval;
                results.push_back(node);
            } else { // it is not a heavy hitter, then aggregate its value to the next level
                Update(i+1, mv_.counts[i][j]->key, mv_.counts[i][j]->value);
            }
        }

    }
    std::cout << "results.size = " << results.size() << std::endl;
}







int MVPipe::GetCount() {
    return mv_.sum;
}


void MVPipe::Reset() {
    mv_.sum = 0;
    for (int i = 0; i < mv_.depth; i++){
        for (int j = 0; j < mv_.width[i]; j++) {
            mv_.counts[i][j]->sum = 0;
            mv_.counts[i][j]->count = 0;
            mv_.counts[i][j]->key = 0;
            mv_.counts[i][j]->value = 0;
        }
    }
}

MVPipe::SBucket*** MVPipe::GetTable() {
    return mv_.counts;
}

void iprint(KEYTYPE key) {
    std::cout << "key=" << ((key>>24)&255) << "." << ((key>>16)&255) << "." << ((key>>8)&255) << "." << (key&255) << std::endl;
}

MVPipe::MV_type MVPipe::GetMV() {
    return mv_;
}

MVPipe::SBucket* MVPipe::GetBucket(int i, int j) {
    return mv_.counts[i][j];
}
