#include "mvpipe2D.hpp"
#include <climits>

MVPipe::MVPipe(int depth, int* width, int lgn, int hash_shift) {
  mv_.depth = depth;
  mv_.width = width;
  mv_.lgn = lgn;
  //denote the row that using hash function to hash keys to the buckets
  mv_.hash_shift = hash_shift;
  mv_.sum = 0;

  mv_.counts = (SBucket****) calloc(depth, sizeof(SBucket***));
  for (int i = 0; i < depth; i++) {
    mv_.counts[i] = (SBucket***) calloc(depth, sizeof(SBucket**));
    for (int j = 0; j < depth; j++) {
      mv_.counts[i][j] = (SBucket**) calloc(width[i*depth+j], sizeof(SBucket*));
      for (int k = 0; k < width[i*depth+j]; k++) {
        mv_.counts[i][j][k] = (SBucket*)calloc(1, sizeof(SBucket));
        memset(mv_.counts[i][j][k], 0, sizeof(SBucket));
      }
    }
  }

  mv_.hash = new unsigned long[NUM_MASKS];
  mv_.scale = new unsigned long[NUM_MASKS];
  mv_.hardner = new unsigned long[NUM_MASKS];
  char name[] = "MVPipe";
  unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
  for (int i = 0; i < NUM_MASKS; i++) {
    mv_.hash[i] = GenHashSeed(seed++);
  }
  for (int i = 0; i < NUM_MASKS; i++) {
    mv_.scale[i] = GenHashSeed(seed++);
  }
  for (int i = 0; i < NUM_MASKS; i++) {
    mv_.hardner[i] = GenHashSeed(seed++);
  }
}

MVPipe::~MVPipe() {
  for (int i = 0; i < mv_.depth; i++) {
    for (int j = 0; j < mv_.depth; j++) {
      for (int k = 0; k < mv_.width[i*mv_.depth+j]; k++) {
        free(mv_.counts[i][j][k]);
      }
      free(mv_.counts[i][j]);
    }
    free(mv_.counts[i]);
  }
  free(mv_.counts);
  delete [] mv_.hash;
  delete [] mv_.scale;
  delete [] mv_.hardner;
}

int MVPipe::CalBucket(KEYTYPE hierar_key, int mask) {

  int bucket = 0;
  if (mv_.width[mask] >= mv_.hash_shift) {
    bucket = MurmurHash64A((unsigned char*)&hierar_key, LGN, mv_.hardner[mask]) % mv_.width[mask];
  } else {
    //TODO: test the correctness of bucket
    int sp = 0, dp = 0;
    unsigned src = masks[mask] >> 32;
    unsigned dst = masks[mask] & 0xffffffff;
    while (src != 0) {
      if ((src&0x00000001)==0) {
        src >>= 1;
        sp++;
      } else break;
    }

    dp = dst == 0 ? 32 : 0;
    while (dst != 0) {
      //if (dst%2==0) {
      if ((dst&0x00000001)==0) {
        dst >>= 1;
        dp++;
      } else break;
    }
    bucket = ((hierar_key >> (32+sp)) << (32-dp)) | ((hierar_key & 0xffffffff) >> dp);
  }
  return bucket;
}



void MVPipe::Update(int srow, int erow, int scol, KEYTYPE key, int val) {

  if (val == 0) return;

  KEYTYPE tempkey = key;
  int tval = val;
  KEYTYPE rowkey = key;
  int rowval = val;
  for (int i = srow; i < erow; i++) {
    tempkey = rowkey;
    tval = rowval;
    int base = i*mv_.depth;
    for (int j = scol; j < mv_.depth; j++) {
      int index = base + j;
      KEYTYPE hierar_key = tempkey & masks[index];
      unsigned long bucket = CalBucket(hierar_key, index);

      MVPipe::SBucket *sbucket = mv_.counts[i][j][bucket];
      sbucket->sum += tval;
      //it is the first flow
      if (sbucket->sum == tval) {
        sbucket->count += tval;
        sbucket->value += tval;
        sbucket->key = hierar_key;
        if (j == 0) {
            return;
        }
        scol = 0;
        break;
      } else if(hierar_key == sbucket->key) {
        sbucket->count += tval;
        sbucket->value += tval;
        if (j == 0) {
            return;
        }
        break;
      } else {
        sbucket->count -= tval;
        if (sbucket->count < 0) {
          //send the original candidate key to the next row
          tempkey = sbucket->key;
          long tempval = sbucket->value;
          sbucket->value = tval;
          sbucket->key = hierar_key;
          sbucket->count = -sbucket->count;
          tval = tempval;
          //backup the row key and value
          if (j == 0) {
            rowkey = tempkey;
            rowval = tempval;
          }
        }
      }
    }
    scol = 0;
  }
}



  int MVPipe::GetCount() {
    return mv_.sum;
  }


  void MVPipe::Reset() {
    mv_.sum = 0;
    for (int i = 0; i < mv_.depth; i++) {
        for (int j = 0;  j < mv_.depth; j++) {
            for (int k = 0; k < mv_.width[i*mv_.depth+j]; k++) {
                mv_.counts[i][j][k]->sum = 0;
                mv_.counts[i][j][k]->count = 0;
                mv_.counts[i][j][k]->value = 0;
                mv_.counts[i][j][k]->key = 0;
            }
        }
    }

  }

  MVPipe::SBucket**** MVPipe::GetTable() {
    return mv_.counts;
  }


  MVPipe::MV_type MVPipe::GetMV() {
    return mv_;
  }


  MVPipe::SBucket* MVPipe::GetBucket(int i, int j, int k) {
    return mv_.counts[i][j][j];
  }






void MVPipe::Query(int thresh, Bvec &results) {

  //1. push all keys in the low level to high level if its counts do not
  //exceed the threshold
  for (int i = 0; i < mv_.depth; i++) {
    for (int j = 0; j < mv_.depth; j++) {
      int index = i*mv_.depth+j;
      for (int k = 0; k < mv_.width[index]; k++) {
        KEYTYPE key = mv_.counts[i][j][k]->key;
        int resval = 0;
        int flag = 0;
        resval = (mv_.counts[i][j][k]->sum + mv_.counts[i][j][k]->count)/2;

        //check the false positives
        int bucket = 0;
        int addval = mv_.counts[i][j][k]->value;
        for (int r = 0; r < 4; r++) {
            if (j+1+r >= mv_.depth) break;
            KEYTYPE rkey = key & masks[index+1+r];
            bucket = CalBucket(rkey, index+1+r);
            int est = 0;
            if (rkey == mv_.counts[i][j+1+r][bucket]->key) {
                est = (mv_.counts[i][j+1+r][bucket]->sum + mv_.counts[i][j+1+r][bucket]->count)/2 + addval;
                addval += mv_.counts[i][j+1+r][bucket]->value;
            } else {
                est = (mv_.counts[i][j+1+r][bucket]->sum - mv_.counts[i][j+1+r][bucket]->count)/2 + addval;
            }
            resval = resval > est ? est : resval;
        }


        int orival = resval;

        if (resval > thresh) {
          //calculate the conditioned count of the candidate hhh
          std::vector<std::pair<Pair, int> > descendants; //the direct descendants of item,mask
          descendants.clear();
          for (auto it = results.begin(); it != results.end(); ++it) {
            if ((masks[it->first.first] & masks[index]) == masks[index] && (it->first.second & masks[index]) == key) {
              //it is a descendant, now eliminate non-direct descendants
              std::vector<std::pair<Pair, int> > newdescendants;
              newdescendants.clear();
              for (auto iter = descendants.begin(); iter != descendants.end(); ++iter) {
                if ((masks[iter->first.first] & masks[it->first.first]) != masks[it->first.first]
                    || (iter->first.second & masks[it->first.first]) != it->first.second) {
                  newdescendants.push_back(*iter);
                }
              }
              descendants = newdescendants;
              descendants.push_back(*it);


              //add back removed val
             if (it->first.first%mv_.depth == 0 || it->first.first/mv_.depth == i) {
                 unsigned long bucket = CalBucket(it->first.second, it->first.first);
                 orival += mv_.counts[it->first.first/mv_.depth][it->first.first%mv_.depth][bucket]->value;
             }
            }
          }

          int s = resval;
          for (auto iter = descendants.begin(); iter != descendants.end(); ++iter) {
            //ignore backbone nodes and same level nodes, they are already be sibtracted in the update
            if (iter->first.first%mv_.depth == 0 || iter->first.first/mv_.depth == index/mv_.depth) {
                continue;
            }
            //get the estimate of the descendant node
            int bucket = CalBucket(iter->first.second, iter->first.first);
            int est = 0;
            int row = (iter->first.first)/mv_.depth;
            int col = (iter->first.first)%mv_.depth;
            if (iter->first.second == mv_.counts[row][col][bucket]->key) {
              //upper
              //est = (mv_.counts[row][col][bucket]->sum + mv_.counts[row][col][bucket]->count)/2;
              //lower
              est = mv_.counts[row][col][bucket]->value;
            } else {
              //upper
              //est = (mv_.counts[row][col][bucket]->sum - mv_.counts[row][col][bucket]->count)/2;
              //lower
              est = 0;
            }
            s -= est;

            //get the estimate of the same level descendant node of the descendant node
            for (auto tit = results.begin(); tit != results.end(); ++tit) {
              //tit is a child of iter
              if ((masks[tit->first.first] & masks[iter->first.first]) == masks[iter->first.first]
                  && (tit->first.second & masks[iter->first.first]) == iter->first.second) {
                //tit is at the same level with iter and it does not equal iter
                if (tit->first.first/mv_.depth == iter->first.first/mv_.depth && (tit->first.first != iter->first.first
                            || tit->first.second != iter->first.second)) {
                  //check if tit has other higher level parents in the descendant set
                  int hasparent = 0;
                  for (auto lit = descendants.begin(); lit != descendants.end(); lit++) {
                    //lit is the parent of tit
                    if (((masks[lit->first.first] | masks[tit->first.first]) == masks[tit->first.first])
                        && (lit->first.second == (tit->first.second & masks[lit->first.first]))) {
                      //lit is at higher level than tit
                      if (lit->first.first/mv_.depth > tit->first.first/mv_.depth) {
                        hasparent = 1;
                        break;
                      }
                    }
                  }
                  //lit is the pure child of iter
                  if (hasparent == 0) {
                    //get the estimate of the descendant of descendant
                    int bucket = CalBucket(tit->first.second, tit->first.first);
                    int estt = 0;
                    row = (tit->first.first)/mv_.depth;
                    col = (tit->first.first)%mv_.depth;
                    if (tit->first.second == mv_.counts[row][col][bucket]->key) {
                      //upper
                      //estt = (mv_.counts[row][col][bucket]->sum + mv_.counts[row][col][bucket]->count)/2;
                      //lower
                      est = mv_.counts[row][col][bucket]->value;
                    } else {
                      //upper
                      //estt = (mv_.counts[row][col][bucket]->sum - mv_.counts[row][col][bucket]->count)/2;
                      //lower
                      est = 0;
                    }
                    s -= estt;
                  }
                }
              }
            }
          }

          for (auto iter = descendants.begin(); iter != descendants.end(); ++iter) {
            for (auto it = iter + 1; it != descendants.end(); ++it) {
              //compute glb of iter and it
              if ( (iter->first.second & masks[it->first.first]) != (it->first.second & masks[iter->first.first]) ) continue;
              int glbmask = 0; while (masks[glbmask] != (masks[it->first.first] | masks[iter->first.first])) glbmask++;
              //glb has been computed now check that there is no third parent
              int numparents = 0;
              for (auto it3 = descendants.begin(); it3 != descendants.end(); ++it3) {
                if (((masks[it3->first.first] | masks[glbmask]) == masks[glbmask])
                        && (it3->first.second == ((it->first.second | iter->first.second) & masks[it3->first.first]))){
                  numparents++;
                }
              }
              if (numparents < 2)
                std::cout << "Error in conditionedcount!"<< std::endl;

              if (numparents == 2) {
                //get the estimate for the glb node
                KEYTYPE glbkey = it->first.second | iter->first.second;
                if (glbmask%mv_.depth != 0) {
                  int bucket = CalBucket(glbkey, glbmask);
                  int est = 0;
                  int row = glbmask/mv_.depth;
                  int col = glbmask%mv_.depth;
                  if (glbkey == mv_.counts[row][col][bucket]->key) {
                    //upper
                    est = (mv_.counts[row][col][bucket]->sum + mv_.counts[row][col][bucket]->count)/2;
                    //lower
                    //est = mv_.counts[row][col][bucket]->value;
                  } else {
                    //upper
                    est = (mv_.counts[row][col][bucket]->sum - mv_.counts[row][col][bucket]->count)/2;
                    //lower
                    //est = 0;
                  }
                  s+= est;
                }
              }
              }
            }
            if (s > thresh) {
              Pair node;
              node.first = index;
              node.second = key;
              std::pair<Pair, int > rnode;
              rnode.first = node;
              //rnode.second = s;
              rnode.second = orival;
              results.push_back(rnode);
              flag = 1;
            }
          }
          if (flag == 0) {
            if (j == 0)
              Update(i, mv_.depth, j+1, mv_.counts[i][j][k]->key, mv_.counts[i][j][k]->value);
            else if (j != mv_.depth - 1)
              Update(i, i+1, j+1, mv_.counts[i][j][k]->key, mv_.counts[i][j][k]->value);
          }
        }
      }
    }

  }











