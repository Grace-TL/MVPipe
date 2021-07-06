#ifndef DATATYPE_H
#define DATATYPE_H
#include <utility>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <vector>

//0: size counting
//1: packet counting
#define COUNT_TYPE  1

#ifdef DIMENSION2
#define KEYTYPE uint64_t
#define LCLitem_t uint64_t
#define LGN 8
#ifdef BITLEVEL
#define NUM_MASKS 1089
#else
#define NUM_MASKS 25
#endif
#else
#define KEYTYPE uint32_t
#define LGN 4
#define LCLitem_t uint32_t
#ifdef BITLEVEL
#define NUM_MASKS  33
#else
#define NUM_MASKS 5
#endif
#endif



/**
 * IP flow key
 * */
typedef struct __attribute__ ((__packed__)) flow_key {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
} flow_key_t;

typedef struct __attribute__ ((__packed__)) Tuple {
    uint64_t key;
    int size;
} tuple_t;


struct pair_hash
{
    template <class T1, class T2>
        std::size_t operator() (const std::pair<T1, T2> &pair) const
        {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }

};


typedef std::pair<int, KEYTYPE>  Pair;

typedef std::unordered_map<unsigned long, int> Unmap;

typedef std::unordered_map<Pair, int, pair_hash> Bmap;

typedef std::vector<std::pair<Pair, int > > Bvec;



#endif
