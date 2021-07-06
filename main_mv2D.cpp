#include <sys/timeb.h>
#include <stdio.h>
#include "src/mvpipe2D.hpp"
#include "lib/evaluation.hpp"
#include "lib/inputadaptor.hpp"
#include "lib/datatypes.hpp"
#include <unordered_map>
#include <time.h>
#include <iomanip>




#if NUM_MASKS==25
uint64_t masks[NUM_MASKS] = {

    0xFFFFFFFFFFFFFFFFull, 0xFFFFFF00FFFFFFFFull, 0xFFFF0000FFFFFFFFull, 0xFF000000FFFFFFFFull, 0x00000000FFFFFFFFull,
    0xFFFFFFFFFFFFFF00ull, 0xFFFFFF00FFFFFF00ull, 0xFFFF0000FFFFFF00ull, 0xFF000000FFFFFF00ull, 0x00000000FFFFFF00ull,
    0xFFFFFFFFFFFF0000ull, 0xFFFFFF00FFFF0000ull, 0xFFFF0000FFFF0000ull, 0xFF000000FFFF0000ull, 0x00000000FFFF0000ull,
    0xFFFFFFFFFF000000ull, 0xFFFFFF00FF000000ull, 0xFFFF0000FF000000ull, 0xFF000000FF000000ull, 0x00000000FF000000ull,
    0xFFFFFFFF00000000ull, 0xFFFFFF0000000000ull, 0xFFFF000000000000ull, 0xFF00000000000000ull, 0x0000000000000000ull
};


int width[NUM_MASKS] = {
   1024
};


#endif
#if NUM_MASKS==1089
uint64_t masks[NUM_MASKS] = {
    1024
};

int width[NUM_MASKS] = {
   1024
};
#endif

int conditionedcount(const std::unordered_map<KEYTYPE, int> * m, const std::vector<Pair> &p, KEYTYPE item, int mask) {


    std::vector<Pair> descendants; //the direct descendants of item,mask
    descendants.clear();
	//int countcandidates=0;
	for (std::vector<Pair>::const_iterator it = p.begin(); it != p.end(); ++it) {
        if ((masks[it->first] & masks[mask]) == masks[mask] && (it->second & masks[mask]) == item) {
			//it is a descendant, now eliminate non-direct descendants
            std::vector<Pair> newdescendants;
            newdescendants.clear();
			//countcandidates++;
			for (std::vector<Pair>::const_iterator iter = descendants.begin(); iter != descendants.end(); ++iter) {
				if ((masks[iter->first] & masks[it->first]) != masks[it->first] || (iter->second & masks[it->first]) != it->second) {
					newdescendants.push_back(*iter);
				}
			}
			descendants = newdescendants;
			descendants.push_back(*it);
		}
	}

    std::unordered_map<KEYTYPE, int>::const_iterator ci = m[mask].find(item);
	int s = (ci != m[mask].end() ? ci->second : 0);


    for (std::vector<Pair>::const_iterator iter = descendants.begin(); iter != descendants.end(); ++iter) {

        std::unordered_map<KEYTYPE, int>::const_iterator ci = m[iter->first].find(iter->second);
        if (ci != m[iter->first].end()) s -= ci->second;
	}
	for (std::vector<Pair>::const_iterator iter = descendants.begin(); iter != descendants.end(); ++iter) {
		for (std::vector<Pair>::const_iterator it = iter + 1; it != descendants.end(); ++it) {
			//compute glb of iter and it
			if ( (iter->second & masks[it->first]) != (it->second & masks[iter->first]) ) continue;
			int glbmask = 0; while (masks[glbmask] != (masks[it->first] | masks[iter->first])) glbmask++;
			//glb has been computed now check that there is no third parent
			int numparents = 0;
			for (std::vector<Pair>::const_iterator it3 = descendants.begin(); it3 != descendants.end(); ++it3) {
				if (((masks[it3->first] | masks[glbmask]) == masks[glbmask])
                        && (it3->second == ((it->second | iter->second) & masks[it3->first]))) {
                    numparents++;
                }
			}

			if (numparents < 2) {
			    std::cout << "Error in conditionedcount!"<< std::endl;
			}
			if (numparents == 2) {

                std::unordered_map<KEYTYPE, int>::const_iterator ci = m[glbmask].find(it->second | iter->second);
				if (ci != m[glbmask].end()) s+= ci->second;
			}
		}
	}
	return s;
}

int main(int argc, char* argv[]) {

        int lgn = sizeof(KEYTYPE)*8;
        int threshold = 1000;
        if (argc < 3) {
            printf("Usage: %s [threshold] [Memory(KB)]\n", argv[0]);
            return 0;
        }
        threshold = atoi(argv[1]);



        //Set memory, unit KB
        int memory = atoi(argv[2]);
        int total_bucket = memory*8*1024/128;
        int row_bucket = total_bucket/NUM_MASKS;
        int dim = sqrt(NUM_MASKS);
#if NUM_MASKS==1089
        //init masks for 2D-bit
        for (int i = 0; i < dim; i++) {
            unsigned long long rowmask = 0xffffffffffffffffull & (0xffffffff00000000ull << i);
            rowmask = rowmask | 0x00000000ffffffffull;
            std::cout << "----------------------------- line " << i << " ---------------------------" << std::endl;
            for (int j = 0; j < dim; j++) {
                masks[i*dim+j] = rowmask & (0xffffffffffffffffull << j);
                //std::cout << std::hex << masks[i*dim+j] << "\t" << std::endl;
            }
        }
#endif


        for (int i  = 0; i < NUM_MASKS; i++) {
            //1. get the width of each row
            unsigned src_mask = masks[i] >> 32;
            unsigned dst_mask = masks[i] & 0xffffffff;
            int sp=0, dp=0;
            while (src_mask%2==0 && src_mask!=0) {
                src_mask >>= 1;
                sp++;
            }

            while (dst_mask%2==0 && dst_mask!=0) {
                dst_mask >>= 1;
                dp++;
            }

            src_mask = src_mask << (32-dp) | dst_mask;
            if (src_mask < (unsigned)row_bucket && i/dim >= dim - i%dim) {
                width[i] = src_mask+1;
                width[(dim-i%dim-1)*dim+(dim-1-i/dim)] = 2*row_bucket-src_mask-1;
            } else {
                width[i] = row_bucket;
            }
        }
        //for (int i = 0; i < NUM_MASKS; i++) {
        //    std::cout << "width[" << i << "] = " << width[i] << std::endl;
        //    if (i%dim == 0) width[i] += 1000;
        //}


        //0. Load data
        char dir[256] = "traces/";
        char filelist[256] = "iptraces.txt";
        long buf_size = 5000000000;
        std::cout << argv[0] << "\t threshold=" << threshold << "\t memory=" << memory << std::endl;



        std::ifstream tracefiles(filelist);
        if (!tracefiles.is_open()) {
            std::cout << "Error opening file" << std::endl;
            return -1;
        }

        double time = 0;
        double precision, recall, error;
        double avg_pre = 0, max_pre = 0, min_pre = 1;
        double avg_rec = 0, max_rec = 0, min_rec = 1;
        double avg_err = 0, max_err = 0, min_err = 100;
        double avg_time = 0, max_time = 0, min_time = 10000000000;
        double avg_thr = 0, max_thr = 0, min_thr = 10000000000;
        int total_len = 0;
        Evaluation *eva = new Evaluation();
        for (std::string file; getline(tracefiles, file);) {
            total_len++;
            Bmap detect;
            Bmap ground;
            Bmap origround;
            origround.clear();
            ground.clear();
            detect.clear();
            InputAdaptor* adaptor =  new InputAdaptor(dir, file, buf_size);
            std::cout << "[Dataset]: " << dir << file << std::endl;
            std::cout << "[Message] Finish read data." << std::endl;
            std::cout << "[Message] Now calculate the ground truth." << std::endl; 
            if (adaptor->GetDataSize() == 0) {
                break;
            }
            ground.clear();
            detect.clear();
            origround.clear();
            //Get ground
            adaptor->Reset();
            std::unordered_map<KEYTYPE, int> m[NUM_MASKS];
            for (int i = 0; i < NUM_MASKS; i++) {
                m[i].clear();
            }
            unsigned long sum = 0;
            tuple_t t;
            unsigned long num_item = 0;
            while (adaptor->GetNext(&t) == 1) {
                int size = 0;
                if (COUNT_TYPE == 0) {
                    size = t.size;
                } else {
                    size = 1;
                }
                sum += size;
                num_item++;
                KEYTYPE item = t.key;

                for (int i = 0; i < NUM_MASKS; i++) {
                    m[i][item & masks[i]] += size;
                }
            }

            std::vector<Pair> exact;
            std::vector<int> exactcount;
            std::vector<int> oricount;
            exact.clear();
            exactcount.clear();
            oricount.clear();
            for (int i = 0; i < NUM_MASKS; i++) {
                for (auto it = m[i].begin(); it != m[i].end(); ++it) {
                    if (it->second >= threshold) {
                        int condcount = conditionedcount(m, exact, it->first, i);
                        if (condcount >= threshold) {
                            exact.push_back(Pair(i, it->first));
                            exactcount.push_back(condcount);
                            oricount.push_back(it->second);
                        }
                    }
                }
            }

            std::vector<Pair>::const_iterator it1 = exact.begin();
            std::vector<int>::const_iterator it2 = exactcount.begin();
            std::vector<int>::const_iterator it3 = oricount.begin();


            while (it1 != exact.end())  {
                ground[*it1] = *it2;
                origround[*it1] = *it3;
                ++it1;
                ++it2;
                ++it3;
            }
            std::cout << "Total true HHHs: " << ground.size() << std::endl;
            std::cout << "[Message] Start detection" << std::endl;
            /*************************************************************************/
            //2. do measurement
            int mvdepth = (int)sqrt(NUM_MASKS);
            MVPipe *mv = new MVPipe(mvdepth, width, lgn, row_bucket);
            adaptor->Reset();
            double start = Evaluation::now_us();
            //debug
            while(adaptor->GetNext(&t) == 1) {
                if (COUNT_TYPE == 0) {
                    mv->Update(0, mvdepth, 0, t.key, t.size);
                } else if (COUNT_TYPE == 1) {
                    mv->Update(0, mvdepth, 0, t.key, 1);
                }
            }

            double end = Evaluation::now_us();
            time = (end - start)/1000000;
            double throughput = adaptor->GetDataSize() / time;
            Bvec res;
            mv->Query(threshold, res);
            for (auto it = res.begin(); it != res.end(); it++) {
                detect[it->first] = it->second;
            }
            //3. evaluate
            eva->get_accuracy(ground, origround, detect, &precision, &recall, &error);
            avg_pre += precision;
            avg_rec += recall;
            avg_err += error;
            avg_time += time;
            avg_thr += throughput;
            min_pre = min_pre > precision ? precision : min_pre;
            max_pre = max_pre < precision ? precision : max_pre;
            min_rec = min_rec > recall ? recall : min_rec;
            max_rec = max_rec < recall ? recall : max_rec;
            min_err = min_err > error ? error : min_err;
            max_err = max_err < error ? error : max_err;
            min_time = min_time > time ? time : min_time;
            max_time = max_time < time ? time : max_time;
            min_thr = min_thr > throughput ? throughput : min_thr;
            max_thr = max_thr < throughput ? throughput : max_thr;
            std::cout << "Memory = " << memory << std::endl;
            printf("Total packets: %lu \n", num_item);
            printf("Total HHHs: %lu \n", res.size());
            std::cout << std::setw(20) << std::left << "Algorithm"
                << std::setw(20) << std::left << "Memory"
                << std::setw(20) << std::left << "Threshold"
                << std::setw(20) << std::left << "Precision"
                << std::setw(20) << std::left << "Recall" << std::setw(20)
                << std::left << "Error" << std::setw(20) << std::left << "Time"
                << std::setw(20)  << std::left << "Throughput" << std::endl;
            std::cout << std::setw(20) << std::left <<  argv[0]
                << std::setw(20) << std::left << memory
                << std::setw(20) << std::left << threshold
                << std::setw(20) << std::left << precision
                << std::setw(20) << std::left << recall << std::setw(20)
                << std::left << error << std::setw(20) << std::left << time
                << std::setw(20)  << std::left << throughput << std::endl;
            delete mv;
        }

        delete eva;
        tracefiles.close();
        std::cout << "--------------------------------------- statistical ------------------------------" << std::endl;
        std::cout << std::setfill(' ');
        std::cout << std::setw(20) << std::left << "Algorithm"
            << std::setw(20) << std::left << "Memory"
            << std::setw(20) << std::left << "Threshold"
            << std::setw(20) << std::left << "Precision"
            << std::setw(20) << std::left << "Recall"
            << std::setw(20) << std::left << "Error"
            << std::setw(20) << std::left << "Time"
            << std::setw(20) << std::left << "Throughput"
            << std::setw(20)  << std::left << "MinPre"
            << std::setw(20)  << std::left << "MinRec"
            << std::setw(20)  << std::left << "MinErr"
            << std::setw(20)  << std::left << "MinTime"
            << std::setw(20) << std::left << "MinThr"
            << std::setw(20)  << std::left << "MaxPre"
            << std::setw(20)  << std::left << "MaxRec"
            << std::setw(20)  << std::left << "MaxErr"
            << std::setw(20)  << std::left << "MaxTime"
            << std::setw(20) << std::left << "MaxThr"
            << std::endl;

        std::cout << std::setw(20) << std::left <<  argv[0]
            << std::setw(20) << std::left << memory
            << std::setw(20) << std::left << threshold
            << std::setw(20) << std::left << avg_pre/total_len
            << std::setw(20) << std::left << avg_rec/total_len
            << std::setw(20) << std::left << avg_err/total_len
            << std::setw(20) << std::left << avg_time/total_len
            << std::setw(20) << std::left << avg_thr/total_len
            << std::setw(20)  << std::left << min_pre
            << std::setw(20)  << std::left << min_rec
            << std::setw(20)  << std::left << min_err
            << std::setw(20)  << std::left << min_time
            << std::setw(20)  << std::left << min_thr
            << std::setw(20)  << std::left << max_pre
            << std::setw(20)  << std::left << max_rec
            << std::setw(20)  << std::left << max_err
            << std::setw(20)  << std::left << max_time
            << std::setw(20)  << std::left << max_thr
            << std::endl;
        return 0;
}

