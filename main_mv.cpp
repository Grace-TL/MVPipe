#include <sys/timeb.h>
#include <stdio.h>
#include "src/mvpipe.hpp"
#include "lib/evaluation.hpp"
#include "lib/inputadaptor.hpp"
#include "lib/datatypes.hpp"
#include <unordered_map>
#include <time.h>
#include <iomanip>




#if NUM_MASKS==5
KEYTYPE masks[NUM_MASKS] = {
    0xFFFFFFFFu,
	0xFFFFFF00u,
	0xFFFF0000u,
	0xFF000000u,
	0x00000000u,
};

int width[NUM_MASKS] = {0};


#endif
#if NUM_MASKS==33
KEYTYPE masks[NUM_MASKS] = {
	0xFFFFFFFFu << 0,
	0xFFFFFFFFu << 1,
	0xFFFFFFFFu << 2,
	0xFFFFFFFFu << 3,
	0xFFFFFFFFu << 4,
	0xFFFFFFFFu << 5,
	0xFFFFFFFFu << 6,
	0xFFFFFFFFu << 7,
	0xFFFFFFFFu << 8,
	0xFFFFFFFFu << 9,
	0xFFFFFFFFu << 10,
	0xFFFFFFFFu << 11,
	0xFFFFFFFFu << 12,
	0xFFFFFFFFu << 13,
	0xFFFFFFFFu << 14,
	0xFFFFFFFFu << 15,
	0xFFFFFFFFu << 16,
	0xFFFFFFFFu << 17,
	0xFFFFFFFFu << 18,
	0xFFFFFFFFu << 19,
	0xFFFFFFFFu << 20,
	0xFFFFFFFFu << 21,
	0xFFFFFFFFu << 22,
	0xFFFFFFFFu << 23,
	0xFFFFFFFFu << 24,
	0xFFFFFFFFu << 25,
	0xFFFFFFFFu << 26,
	0xFFFFFFFFu << 27,
	0xFFFFFFFFu << 28,
	0xFFFFFFFFu << 29,
	0xFFFFFFFFu << 30,
	0xFFFFFFFFu << 31,
	0x00000000u
};

int width[NUM_MASKS] = {0};
#endif



int conditionedcount(std::unordered_map<unsigned, int> * m, const std::vector<Pair> &p, unsigned item, int mask) {
    std::vector<Pair> descendants; //the direct descendants of item,mask
    descendants.clear();
	for (std::vector<Pair>::const_iterator it = p.begin(); it != p.end(); ++it) {
		if ((masks[it->first] & masks[mask]) == masks[mask] && (it->second & masks[mask]) == item) {
			//it is a descendant, now eliminate non-direct descendants
            std::vector<Pair> newdescendants;
            newdescendants.clear();
			for (std::vector<Pair>::const_iterator iter = descendants.begin(); iter != descendants.end(); ++iter) {
				if ((masks[iter->first] & masks[it->first]) != masks[it->first] || (iter->second & masks[it->first]) != it->second) {
					newdescendants.push_back(*iter);
				}
			}
			descendants = newdescendants;
			descendants.push_back(*it);
		}
	}

	int s = m[mask][item];
	for (std::vector<Pair>::iterator iter = descendants.begin(); iter != descendants.end(); ++iter) {
		s -= m[iter->first][iter->second];
	}
	return s;
}



int main(int argc, char* argv[]) {
		double time;
        int memory = 0;
        int lgn = sizeof(KEYTYPE)*8;
        int threshold = 1000;
        if (argc < 3) {
            printf("Usage: %s [threshold] [Memory(KB)]\n", argv[0]);
            return 0;
        }
        threshold = atoi(argv[1]);


        //0. Load data
        char dir[256] = "traces/";
        char filelist[256] = "iptraces.txt";
        unsigned long  buf_size = 5000000000;
        std::cout << argv[0] << "\t Threshold=" << threshold << std::endl;


        std::ifstream tracefiles(filelist);
        if (!tracefiles.is_open()) {
            std::cout << "Error opening file" << std::endl;
            return -1;
        }


        double precision, recall, error;
        double avg_pre = 0, max_pre = 0, min_pre = 1;
        double avg_rec = 0, max_rec = 0, min_rec = 1;
        double avg_err = 0, max_err = 0, min_err = 100;
        double avg_time = 0, max_time = 0, min_time = 10000000000;
        double avg_thr = 0, max_thr = 0, min_thr = 10000000000;
        int total_len = 0;
        int len = 0;
        Evaluation *eva = new Evaluation();
        for (std::string file; getline(tracefiles, file);) {
            len++;
            InputAdaptor* adaptor =  new InputAdaptor(dir, file, buf_size);
            std::cout << "[Dataset]: " << dir << file << std::endl;
            std::cout << "[Message] Finish read data." << std::endl;

            Unmap detect;
            Unmap ground;
            Unmap origround;
            origround.clear();
            ground.clear();
            detect.clear();
                origround.clear();
                ground.clear();
                detect.clear();

                if (adaptor->GetDataSize() == 0) {
                    break;
                }
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
                    for (int j = 0; j < NUM_MASKS; j++) {
                        m[j][item & masks[j]] += size;
                    }
                }

                std::vector<std::pair<int, unsigned> > exact;
                std::vector<int> exactcount;
                std::vector<int> oricount;
                exact.clear();
                exactcount.clear();
                oricount.clear();
                for (int i = 0; i < NUM_MASKS; i++) {
                    for (auto it = m[i].begin(); it != m[i].end(); ++it) {
                        int ccount = conditionedcount(m, exact, it->first, i);
                        if (it->second >= threshold &&  ccount >= threshold) {
                            exact.push_back(std::pair<int, KEYTYPE>(i, it->first));
                            exactcount.push_back(ccount);
                            oricount.push_back(it->second);
                        }
                    }
                }
                std::vector< std::pair<int, KEYTYPE> >::const_iterator it1 = exact.begin();
                std::vector< int >::const_iterator it2 = exactcount.begin();
                std::vector< int >::const_iterator it3 = oricount.begin();
                while (it1 != exact.end())  {
                    //prefix;
                    unsigned long key = it1->second;
                    key <<= 32;
                    key |= it1->first;
                    ground[key] = *it2;
                    origround[key] = *it3;
                    ++it1;
                    ++it2;
                    ++it3;
                }
                std::cout << "Total true HHHs: " << ground.size() << std::endl;
                /*************************************************************************/
                //2. do measurement

                //Another method to set memory
                memory = atoi(argv[2]);
                int total_bucket = memory*8*1024/128;
                int row_bucket = total_bucket/NUM_MASKS;
                int flag = 0;
                for (int i  = NUM_MASKS; i > 0; i--) {
                    long total = (long)1<<((NUM_MASKS-i)*33/(NUM_MASKS-1));
                    if (total < (long)row_bucket) {
                        flag = NUM_MASKS-i;
                        width[i-1] = (int)total;
                        total_bucket -= (int)total;
                        row_bucket = total_bucket/(i-1);
                    } else {
                        width[i-1] = row_bucket;
                    }
                }

                MVPipe *mv = new MVPipe(NUM_MASKS, width, lgn, NUM_MASKS-flag-1);
                adaptor->Reset();
                double start = Evaluation::now_us();
                while(adaptor->GetNext(&t) == 1) {
                    if (COUNT_TYPE == 0) {
                        mv->Update(0, t.key, t.size);
                    } else if (COUNT_TYPE == 1) {
                        mv->Update(0, t.key, 1);
                    }
                }
                double end = Evaluation::now_us();

                time = (end - start)/1000000;
                double throughput = num_item / time;
                std::vector<std::pair<unsigned long, int> > res;
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

            total_len += len;
        }
        delete eva;
        tracefiles.close();
        std::cout << "--------------------------------------- statistical ------------------------------" << std::endl;
        std::cout << std::setfill(' ');
        std::cout << std::setw(20) << std::left << "Algorithm"
            << std::setw(20) << std::left << "Type"
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
            << std::setw(20) << std::left << NUM_MASKS
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

