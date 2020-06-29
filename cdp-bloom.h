#ifndef CDP_BLOOM_H
#define CDP_BLOOM_H

// https://en.wikipedia.org/wiki/Bloom_filter

#include "Arduino.h"
#include "mbedtls/md.h"
#include "cdpcfg.h"

typedef struct {
	uint32_t n; // number of records in the filter
	uint32_t k; // number of keys per record
	uint32_t m; // number of bits per key (== filter size is 2^m bits)
	uint32_t s; // salt
	byte *filt; // the actual filter
	} cdp_bf_t;

cdp_bf_t* cdp_bf_init(uint32_t k, uint32_t m, uint32_t s, cdp_bf_t *bf);
bool _cdp_bf_check_set(cdp_bf_t *bf, byte *data, uint32_t datasize, bool set);

#define cdp_bf_check_add(bf, data, datasize) _cdp_bf_check_set(bf,data,datasize,true)
#define cdp_bf_check(bf, data, datasize) _cdp_bf_check_set(bf,data,datasize,false)

#endif
