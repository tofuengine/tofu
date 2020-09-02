#include "mdataio.h"

#include "dataio.h"

#include <stdio.h>
#include <limits.h>

static inline ptrdiff_t CAN_READ(MFILE *m)
{
	if (m->size >= 0)
		return m->pos >= 0 ? m->size - m->pos : 0;

	return INT_MAX;
}

uint8_t mread8(MFILE *m, int *err)
{
	*err = 0;
	uint8_t x = 0xff;
	mread(&x, 1, 1, m);
	return x;
}

int8_t mread8s(MFILE *m, int *err)
{
	*err = 0;
	return (int8_t)mgetc(m);
}

uint16_t mread16l(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 2) {
		uint16_t n = readmem16l(m->start + m->pos);
		m->pos += 2;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffff;
	}
}

uint16_t mread16b(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 2) {
		uint16_t n = readmem16b(m->start + m->pos);
		m->pos += 2;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffff;
	}
}

uint32_t mread32l(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 4) {
		uint32_t n = readmem32l(m->start + m->pos);
		m->pos += 4;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffffffff;
	}
}

uint32_t mread32b(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 4) {
		uint32_t n = readmem32b(m->start + m->pos);
		m->pos += 4;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffffffff;
	}
}
