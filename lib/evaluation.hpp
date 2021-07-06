#ifndef EVALUATION
#define EVALUATION
#include "string.h"
#include "datatypes.hpp"
#include <iostream>
#include <time.h>
#include <sys/time.h>

class Evaluation {

    public:
        Evaluation();
        ~Evaluation();
        static inline double now_us ()
        {
            struct timespec tv;
            clock_gettime(CLOCK_MONOTONIC, &tv);
            return (tv.tv_sec * (uint64_t) 1000000 + (double)tv.tv_nsec/1000);
        }

        static inline std::string to_ip(KEYTYPE key) {
            std::string ip = std::to_string((key>>24)&255) + "." + std::to_string((key>>16)&255) + "." + std::to_string((key>>8)&255) + "." + std::to_string(key&255);
            return ip;
        }


        void get_accuracy(Unmap &ground, Unmap &detect, double *precision, double *recall, double *error);
        void get_accuracy(Bmap &ground, Bmap &detect, double *precision, double *recall, double *error);

        void get_accuracy(Unmap &ground, Unmap &origround, Unmap &detect, double *precision, double *recall, double *error);
        void get_accuracy(Bmap &ground, Bmap &origround, Bmap &detect, double *precision, double *recall, double *error);


};


#endif


Evaluation::Evaluation() {};

Evaluation::~Evaluation() {};



void Evaluation::get_accuracy(Unmap &ground, Unmap &detect, double *precision, double *recall, double *error) {
    int tp=0, fp=0, fn=0;
    *error = 0;
    for (auto it = detect.begin(); it != detect.end(); it++) {
            if (ground.find(it->first) != ground.end() ) {
                tp++;
                *error += 1.0*abs(it->second - ground[it->first])/ground[it->first];
            }
    }

    for (auto it = ground.begin(); it != ground.end(); it++) {
        if (detect.find(it->first) != detect.end() ) {
        }
    }

    fn = ground.size() - tp; //remove root
    fp = detect.size() - tp;
    *error = *error/tp;
    *precision = tp*1.0/(tp+fp);
    *recall = tp*1.0/(tp+fn);
}


void Evaluation::get_accuracy(Bmap &ground, Bmap &detect, double *precision, double *recall, double *error) {
    int tp=0, fp=0, fn=0;
    *error = 0;
    for (auto it = detect.begin(); it != detect.end(); it++) {
            if (ground.find(it->first) != ground.end() ) {
                tp++;
                *error += 1.0*abs(it->second - ground[it->first])/ground[it->first];
            }
    }

    fn = ground.size() - tp; //remove root
    fp = detect.size() - tp;
    *error = *error/tp;
    *precision = tp*1.0/(tp+fp);
    *recall = tp*1.0/(tp+fn);
}



//compared with original count in the stream
void Evaluation::get_accuracy(Unmap &ground, Unmap &origround, Unmap &detect, double *precision, double *recall, double *error) {
    int tp=0, fp=0, fn=0;
    *error = 0;
    for (auto it = detect.begin(); it != detect.end(); it++) {
            if (ground.find(it->first) != ground.end() ) {
                tp++;
                *error += 1.0*abs(it->second - origround[it->first])/origround[it->first];
            }
    }

    fn = ground.size() - tp; //remove root
    fp = detect.size() - tp;
    *error = *error/tp;
    *precision = tp*1.0/(tp+fp);
    *recall = tp*1.0/(tp+fn);
}


void Evaluation::get_accuracy(Bmap &ground, Bmap &origround, Bmap &detect, double *precision, double *recall, double *error) {
    int tp=0, fp=0, fn=0;
    *error = 0;
    for (auto it = detect.begin(); it != detect.end(); it++) {
            if (ground.find(it->first) != ground.end() ) {
                tp++;
                *error += 1.0*abs(it->second - origround[it->first])/origround[it->first];
            }
    }

    fn = ground.size() - tp; //remove root
    fp = detect.size() - tp;
    *error = *error/tp;
    *precision = tp*1.0/(tp+fp);
    *recall = tp*1.0/(tp+fn);
}


