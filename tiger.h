#ifndef TIGER_HASH_H__
#define TIGER_HASH_H__

#include <stdint.h>
#include <endian.h>
#include <string.h>
#include <sys/types.h>

class tiger_hash
{
	static uint64_t sandbox[4 * 256];

	uint64_t a, b, c;
	uint64_t temp[8];
	size_t temp_size, data_size;

	static inline void key_schedule(uint64_t *x) {
		x[0] -= x[7] ^ 0xA5A5A5A5A5A5A5A5LL;
		x[1] ^= x[0];
		x[2] += x[1];
		x[3] -= x[2] ^ ((~x[1])<<19);
		x[4] ^= x[3];
		x[5] += x[4];
		x[6] -= x[5] ^ ((~x[4])>>23);
		x[7] ^= x[6];
		x[0] += x[7];
		x[1] -= x[0] ^ ((~x[7])<<19);
		x[2] ^= x[1];
		x[3] += x[2];
		x[4] -= x[3] ^ ((~x[2])>>23);
		x[5] ^= x[4];
		x[6] += x[5];
		x[7] -= x[6] ^ 0x0123456789ABCDEFLL;
	}

	template<int MUL>
	static inline void round(uint64_t &a, uint64_t &b, uint64_t &c, uint64_t x) {
		c ^= x;
		a -=
		    sandbox[((c) >> (0*8)) & 0xFF] ^
		    sandbox[256 + (((c) >> (2*8)) & 0xFF)] ^
		    sandbox[512 + (((c) >> (4*8)) & 0xFF)] ^
		    sandbox[768 + (((c) >> (6*8)) & 0xFF)];

		b +=
		    sandbox[768 + (((c) >> (1*8)) & 0xFF)] ^
		    sandbox[512 + (((c) >> (3*8)) & 0xFF)] ^
		    sandbox[256 + (((c) >> (5*8)) & 0xFF)] ^
		    sandbox[((c) >> (7*8)) & 0xFF];

		b *= MUL;
	}

	template<int MUL>
	static inline void pass(uint64_t &a, uint64_t &b, uint64_t &c, uint64_t *x) {
		round<MUL>(a, b, c, *x++);
		round<MUL>(b, c, a, *x++);
		round<MUL>(c, a, b, *x++);
		round<MUL>(a, b, c, *x++);
		round<MUL>(b, c, a, *x++);
		round<MUL>(c, a, b, *x++);
		round<MUL>(a, b, c, *x++);
		round<MUL>(b, c, a, *x++);
	}

	void update(const uint64_t *data) {
		uint64_t x[8] = {0, 0, 0, 0, 0, 0, 0, 0 };
		for(uint8_t i = 0; i < 8; ++i) {
			x[i] = le64toh(data[i]);
		}
		uint64_t aa = a, bb = b, cc = c;

		pass<5>(a, b, c, x);
		key_schedule(x);
		pass<7>(c, a, b, x);
		key_schedule(x);
		pass<9>(b, c, a, x);
		a ^= aa;
		b -= bb;
		c += cc;
	}

public:
	inline tiger_hash():
		a(0x0123456789ABCDEFLL),
		b(0xFEDCBA9876543210LL),
		c(0xF096A5B4C3B2E187LL),
		temp_size(0), data_size(0) {}

	inline void reset() {
		a = 0x0123456789ABCDEFLL;
		b = 0xFEDCBA9876543210LL;
		c = 0xF096A5B4C3B2E187LL;
		temp_size = 0;
		data_size = 0;
	}

	void update(const void *_data, size_t size) {
		const uint8_t *data = (const uint8_t *)_data;
		if (temp_size != 0) {
			size_t need = 64 - temp_size;
			if (size >= need) {
				memcpy((uint8_t *)temp + temp_size, data, need);
				update(temp);
				data += need;
				data_size += need;
				size -= need;
				temp_size = 0;
			} else {
				memcpy((uint8_t *)temp + temp_size, data, size);
				temp_size += size;
				data_size += size;
				return;
			}
		}

		const uint64_t *src;
		for(src = (const uint64_t *)data; size >= 64; size -= 64, src += 8) {
			update(src);
			data_size += 64;
		}
		memcpy((uint8_t *)temp + temp_size, src, size);
		temp_size += size;
		data_size += size;
	}

	//buffer should be at least 24 bytes lenght
	void save(uint8_t *hash) {
		((uint8_t *)temp)[temp_size] = 1;
		memset(((uint8_t *)temp) + temp_size + 1, 0, 63 - temp_size);

		if (temp_size < 56) {
			temp[7] = htole64(data_size << 3);
		}

		update(temp);

		if (temp_size >= 56) {
			memset(temp, 0, sizeof(temp) - 8);
			temp[7] = htole64(data_size << 3);
			update(temp);
		}

		uint64_t * dst = (uint64_t *)hash;
		*dst++ = htole64(a);
		*dst++ = htole64(b);
		*dst++ = htole64(c);
	}
};

#endif
