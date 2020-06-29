
// https://en.wikipedia.org/wiki/Bloom_filter

#include "cdp-bloom.h"

cdp_bf_t* cdp_bf_init(uint32_t k, uint32_t m, uint32_t s, cdp_bf_t *bf) {
	if (bf == NULL) {
		bf = (cdp_bf_t *)calloc(sizeof(cdp_bf_t), 1);
	}
	bf->k = k;
	bf->m = m;
	bf->n = 0;
	if (s == 0) {
		bf->s = esp_random();
	} else {
		bf->s = s;
	}
	if (bf->filt == NULL) {
		bf->filt = (byte *)calloc(1<<(m-3),1);
	} else {
		memset(bf->filt, 0, 1<<(m-3));
	}
	return bf;
}

bool _cdp_bf_check_set(cdp_bf_t *bf, byte *data, uint32_t datasize, bool set) {
	uint32_t kset = 0;
	uint32_t need = bf->m >> 3;
	if (bf->m & 7) {
		need++;
	}
	byte hash[CDPCFG_BLOOM_HASH_SIZE];
	byte offs = CDPCFG_BLOOM_HASH_SIZE;
	uint32_t seq = bf->s;
	for (uint32_t i=0 ; i < bf->k ; i++) {
		if ((offs+need) > CDPCFG_BLOOM_HASH_SIZE) {
			mbedtls_md_context_t ctx;
			mbedtls_md_type_t md_type = CDPCFG_BLOOM_HASH_TYPE;
			mbedtls_md_init(&ctx);
			mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
			mbedtls_md_starts(&ctx);
			mbedtls_md_update(&ctx, (const unsigned char *) &seq, 4);
			mbedtls_md_update(&ctx, (const unsigned char *) data, datasize);
			mbedtls_md_update(&ctx, (const unsigned char *) &seq, 4);
			mbedtls_md_finish(&ctx, hash);
			mbedtls_md_free(&ctx);
			seq++;
			offs = 0;
		}

		uint32_t val = 0;
		uint32_t want = bf->m;
		while (want > 0) {
			byte b = hash[offs++];
			if (want >= 8) {
				val = (val << 8) + b;
				want -= 8;
			} else {
				val = (val << want) + (b >> (8-want));
				want = 0;
			}
		}

		uint32_t fo = val >> 3;
		byte fm = 1 << (val & 7);

		byte ov = bf->filt[fo];
		if ((ov & fm) == 0) {
			if (set) {
				bf->filt[fo] = ov | fm;
				kset++;
			} else {
				return false;
			}
		}
	}

	if (set && (kset > 0)) {
		bf->n++;
		return false;
	}
	return true;
}

