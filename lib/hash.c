#include <stdio.h>
#include "hash.h"
#include <string.h>
#define BIG_CONSTANT(x) (x##LLU)
#define FORCEINLINE _forceinline

#if defined(_MSC_VER)

#define FORCE_INLINE__forceinline

#include <stdlib.h>

#define ROTL64(x,y)_rotl64(x,y)

#define BIG_CONSTANT(x) (x)

// Other compilers

#else// defined(_MSC_VER)

#define FORCE_INLINE inline __attribute__((always_inline))

inline uint64_t rotl64 ( uint64_t x, int8_t r  )
{
    return (x << r) | (x >> (64 - r));
}

#define ROTL64(x,y)rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)


/* New AwareHash to hanle 4 bits each time
 */
uint64_t AwareHash(unsigned char* data, uint64_t n,
        uint64_t hash, uint64_t scale, uint64_t hardener) {

    n = 2*n;
    char tmp = 0;
	while (n) {
		hash *= scale;
        if(n%2 == 0)
        tmp = *data++;
        hash += (tmp >> (n%2)*4) % (1 << (n%2+1)*4);
        n--;
	}
	return hash ^ hardener;
}

uint64_t AwareHash_debug(unsigned char* data, uint64_t n,
        uint64_t hash, uint64_t scale, uint64_t hardener) {

	while (n) {
		hash *= scale;
		hash += *data++;
		n--;
	}
	return hash ^ hardener;
}

void mangle(const unsigned char* key, unsigned char* ret_key,
		int nbytes) {

	unsigned long long new_key = 0;
	int i;
    //fix nbytes = 4
	for (i=0; i<nbytes; ++i) {
        if(i == 0)
            new_key |= key[0];
        else if(i == 1)
            new_key |= key[3] << 8;
        else if(i == 2)
            new_key |= key[2] << 16;
        else
            new_key |= key[1] << 24;
	}
	new_key = (new_key * 2083697005) & (0xffffffff);
	for (i=0; i<nbytes; ++i) {
		ret_key[i] = (new_key >> (i * 8)) & 0xff;
    }
}

void unmangle(const unsigned char* key, unsigned char* ret_key,
        int nbytes) {

    /*
     * 10001^-1 mod 2^32 = 3472992753
     * 1001^-1 mod 2^32 = 3054961753
     * 101^-1 mod 2^32 = 2083697005

*/
    unsigned long long new_key = 0;
    int i;
    for (i=0; i<nbytes; ++i) {
        new_key |= key[i] << (i * 8);
    }
    new_key = (new_key * 101) & (0xffffffff);
    unsigned long long temp;
    for (i=0; i<nbytes; ++i) {
        if(i == 0)
            ret_key[0] = new_key & 0xff;
        else if(i == 1){
            temp = new_key;
            ret_key[1] = temp >> 24 & 0xff;
        }
        else if(i == 2){
            temp = new_key;
            ret_key[2] = temp >> 16 & 0xff;
        }
        else{
            temp = new_key;
            ret_key[3] = temp >> 8 & 0xff;
        }
    }

}


uint64_t seed = 0;
uint64_t GenHashSeed(uint64_t index) {
    /*
    if (index == 0) {
        srand(0);
    }
    */
    if (seed == 0) {
        seed = rand();
    }
    uint64_t x, y = seed + index;
    mangle((const unsigned char*)&y, (unsigned char*)&x, 8);
    return AwareHash((uint8_t*)&y, 8, 388650253, 388650319, 1176845762);
}

int is_prime(int num) {
    int i;
    for (i=2; i<num; i++) {
        if ((num % i) == 0) {
            break;
        }
    }
    if (i == num) {
        return 1;
    }
    return 0;
}

int calc_next_prime(int num) {
    while (!is_prime(num)) {
        num++;
    }
    return num;
}


//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby
// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
// and endian-ness issues if used across multiple platforms.
// 64-bit hash for 64-bit platforms
uint64_t MurmurHash64A ( const void * key, int len, uint64_t seed )
{
    const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len/8);

    while(data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch(len & 7)
    {
        case 7: h ^= uint64_t(data2[6]) << 48;
        case 6: h ^= uint64_t(data2[5]) << 40;
        case 5: h ^= uint64_t(data2[4]) << 32;
        case 4: h ^= uint64_t(data2[3]) << 24;
        case 3: h ^= uint64_t(data2[2]) << 16;
        case 2: h ^= uint64_t(data2[1]) << 8;
        case 1: h ^= uint64_t(data2[0]);
                h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}


uint32_t MurmurHash2 ( const void * key, int len, uint32_t seed  )
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value

    uint32_t h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch(len)
    {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

FORCE_INLINE uint64_t getblock64 ( const uint64_t * p, int i  )
{
      return p[i];

}

FORCE_INLINE uint64_t fmix64 ( uint64_t k  )
{
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;

    return k;

}



void MurmurHash3_x64_128 ( const void * key, const int len,
                                   const uint32_t seed, void * out )
{
      const uint8_t * data = (const uint8_t*)key;
        const int nblocks = len / 16;

        uint64_t h1 = seed;
        uint64_t h2 = seed;

        const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
        const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

        //----------
        //  // body
        //
        const uint64_t * blocks = (const uint64_t *)(data);

        for(int i = 0; i < nblocks; i++)
        {
            uint64_t k1 = getblock64(blocks,i*2+0);
            uint64_t k2 = getblock64(blocks,i*2+1);
            k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
            h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
            k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
            h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
        }

        const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

        uint64_t k1 = 0;
        uint64_t k2 = 0;

        switch(len & 15)
        {
            case 15: k2 ^= ((uint64_t)tail[14]) << 48;
            case 14: k2 ^= ((uint64_t)tail[13]) << 40;
            case 13: k2 ^= ((uint64_t)tail[12]) << 32;
            case 12: k2 ^= ((uint64_t)tail[11]) << 24;
            case 11: k2 ^= ((uint64_t)tail[10]) << 16;
            case 10: k2 ^= ((uint64_t)tail[ 9 ]) << 8;
            case  9: k2 ^= ((uint64_t)tail[ 8 ]) << 0;
                     k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

            case  8: k1 ^= ((uint64_t)tail[ 7 ]) << 56;
            case  7: k1 ^= ((uint64_t)tail[ 6 ]) << 48;
            case  6: k1 ^= ((uint64_t)tail[ 5 ]) << 40;
            case  5: k1 ^= ((uint64_t)tail[ 4 ]) << 32;
            case  4: k1 ^= ((uint64_t)tail[ 3 ]) << 24;
            case  3: k1 ^= ((uint64_t)tail[ 2 ]) << 16;
            case  2: k1 ^= ((uint64_t)tail[ 1 ]) << 8;
            case  1: k1 ^= ((uint64_t)tail[ 0 ]) << 0;
                     k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= len; h2 ^= len;

        h1 += h2;
        h2 += h1;

        h1 = fmix64(h1);
        h2 = fmix64(h2);

        h1 += h2;
        h2 += h1;

        ((uint64_t*)out)[0] = h1;
        ((uint64_t*)out)[1] = h2;
}

