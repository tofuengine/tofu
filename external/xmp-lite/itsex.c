#ifndef LIBXMP_CORE_DISABLE_IT

/* Public domain IT sample decompressor by Olivier Lapicque */

#include "itsex.h"

static inline uint32_t read_bits(HIO_HANDLE *ibuf, uint32_t *bitbuf, int *bitnum, int n, int *err)
{
	uint32_t retval = 0;
	int i = n;
	int bnum = *bitnum;
	uint32_t bbuf = *bitbuf;

	if (n > 0 && n <= 32) {
		do {
			if (bnum == 0) {
				if (hio_eof(ibuf)) {
					*err = EOF;
					return 0;
				}
				bbuf = hio_read8(ibuf);
				bnum = 8;
			}
			retval >>= 1;
			retval |= bbuf << 31;
			bbuf >>= 1;
			bnum--;
			i--;
		} while (i != 0);

		i = n;

		*bitnum = bnum;
		*bitbuf = bbuf;
	} else {
		/* Invalid shift value. */
		*err = -2;
		return 0;
	}

	return (retval >> (32 - i));
}


int itsex_decompress8(HIO_HANDLE *src, uint8_t *dst, uint32_t len, int it215)
{
	/* uint32_t size = 0; */
	uint32_t block_count = 0;
	uint32_t bitbuf = 0;
	int bitnum = 0;
	uint8_t left = 0, temp = 0, temp2 = 0;
	uint32_t d, pos;
	int err = 0;

	while (len) {
		if (!block_count) {
			block_count = 0x8000;
			/*size =*/ hio_read16l(src);
			left = 9;
			temp = temp2 = 0;
			bitbuf = bitnum = 0;
		}

		d = block_count;
		if (d > len)
			d = len;

		/* Unpacking */
		pos = 0;
		do {
			uint16_t bits = read_bits(src, &bitbuf, &bitnum, left, &err);
			if (err != 0)
				return -1;

			if (left < 7) {
				uint32_t i = 1 << (left - 1);
				uint32_t j = bits & 0xffff;
				if (i != j)
					goto unpack_byte;
				bits = (read_bits(src, &bitbuf, &bitnum, 3, &err)
								+ 1) & 0xff;
				if (err != 0)
					return -1;

				left = ((uint8_t)bits < left) ?  (uint8_t)bits :
						(uint8_t)((bits + 1) & 0xff);
				goto next;
			}

			if (left < 9) {
				uint16_t i = (0xff >> (9 - left)) + 4;
				uint16_t j = i - 8;

				if ((bits <= j) || (bits > i))
					goto unpack_byte;

				bits -= j;
				left = ((uint8_t)(bits & 0xff) < left) ?
						(uint8_t)(bits & 0xff) :
						(uint8_t)((bits + 1) & 0xff);
				goto next;
			}

			if (left >= 10)
				goto skip_byte;

			if (bits >= 256) {
				left = (uint8_t) (bits + 1) & 0xff;
				goto next;
			}

		    unpack_byte:
			if (left < 8) {
				uint8_t shift = 8 - left;
				signed char c = (signed char)(bits << shift);
				c >>= shift;
				bits = (uint16_t) c;
			}
			bits += temp;
			temp = (uint8_t)bits;
			temp2 += temp;
			dst[pos] = it215 ? temp2 : temp;

		    skip_byte:
			pos++;

		    next:
			/* if (slen <= 0)
				return -1 */;
		} while (pos < d);

		/* Move On */
		block_count -= d;
		len -= d;
		dst += d;
	}

	return 0;
}

int itsex_decompress16(HIO_HANDLE *src, int16_t *dst, uint32_t len, int it215)
{
	/* uint32_t size = 0; */
	uint32_t block_count = 0;
	uint32_t bitbuf = 0;
	int bitnum = 0;
	uint8_t left = 0;
	int16_t temp = 0, temp2 = 0;
	uint32_t d, pos;
	int err = 0;

	while (len) {
		if (!block_count) {
			block_count = 0x4000;
			/*size =*/ hio_read16l(src);
			left = 17;
			temp = temp2 = 0;
			bitbuf = bitnum = 0;
		}

		d = block_count;
		if (d > len)
			d = len;

		/* Unpacking */
		pos = 0;
		do {
			uint32_t bits = read_bits(src, &bitbuf, &bitnum, left, &err);
			if (err != 0)
				return -1;

			if (left < 7) {
				uint32_t i = 1 << (left - 1);
				uint32_t j = bits;

				if (i != j)
					goto unpack_byte;

				bits = read_bits(src, &bitbuf, &bitnum, 4, &err) + 1;
				if (err != 0)
					return -1;

				left = ((uint8_t)(bits & 0xff) < left) ?
						(uint8_t)(bits & 0xff) :
						(uint8_t)((bits + 1) & 0xff);
				goto next;
			}

			if (left < 17) {
				uint32_t i = (0xffff >> (17 - left)) + 8;
				uint32_t j = (i - 16) & 0xffff;

				if ((bits <= j) || (bits > (i & 0xffff)))
					goto unpack_byte;

				bits -= j;
				left = ((uint8_t)(bits & 0xff) < left) ?
						(uint8_t)(bits & 0xff) :
						(uint8_t)((bits + 1) & 0xff);
				goto next;
			}

			if (left >= 18)
				goto skip_byte;

			if (bits >= 0x10000) {
				left = (uint8_t)(bits + 1) & 0xff;
				goto next;
			}

		    unpack_byte:
			if (left < 16) {
				uint8_t shift = 16 - left;
				int16_t c = (int16_t)(bits << shift);
				c >>= shift;
				bits = (uint32_t) c;
			}
			bits += temp;
			temp = (int16_t)bits;
			temp2 += temp;
			dst[pos] = (it215) ? temp2 : temp;

		    skip_byte:
			pos++;

		    next:
			/* if (slen <= 0)
				return -1 */;
		} while (pos < d);

		/* Move On */
		block_count -= d;
		len -= d;
		dst += d;
		if (len <= 0)
			break;
	}

	return 0;
}

#endif /* LIBXMP_CORE_DISABLE_IT */
