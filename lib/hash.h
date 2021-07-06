#ifndef __HASH_H__
#define __HASH_H__

#include <stdint.h>
#include <stdlib.h>

#if defined (__cplusplus)
extern "C" {
#endif

uint64_t AwareHash(unsigned char* data, uint64_t n,
        uint64_t hash, uint64_t scale, uint64_t hardener);

uint64_t AwareHash_debug(unsigned char* data, uint64_t n,
        uint64_t hash, uint64_t scale, uint64_t hardener);

uint64_t GenHashSeed(uint64_t index);

int is_prime(int num);

int calc_next_prime(int num);

void mangle(const unsigned char* key, unsigned char* ret_key,
		int nbytes);

void unmangle(const unsigned char* key, unsigned char* ret_key,
		int nbytes);

uint64_t MurmurHash64A( const void * key, int len, uint64_t seed );
uint32_t MurmurHash2( const void *key, int len, uint32_t seed );
void MurmurHash3_x64_128 ( const void * key, const int len,
                                   const uint32_t seed, void * out );
#if defined (__cplusplus)
}
#endif


#endif
