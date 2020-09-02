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

uint8 mread8(MFILE *m, int *err)
{
	*err = 0;
	uint8 x = 0xff;
	mread(&x, 1, 1, m);
	return x;
}

int8 mread8s(MFILE *m, int *err)
{
	*err = 0;
	return (int8)mgetc(m);
}

uint16 mread16l(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 2) {
		uint16 n = readmem16l(m->start + m->pos);
		m->pos += 2;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffff;
	}
}

uint16 mread16b(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 2) {
		uint16 n = readmem16b(m->start + m->pos);
		m->pos += 2;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffff;
	}
}

uint32 mread32l(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 4) {
		uint32 n = readmem32l(m->start + m->pos);
		m->pos += 4;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffffffff;
	}
}

uint32 mread32b(MFILE *m, int *err)
{
	*err = 0;
	ptrdiff_t can_read = CAN_READ(m);
	if (can_read >= 4) {
		uint32 n = readmem32b(m->start + m->pos);
		m->pos += 4;
		return n;
	} else {
		*err = EOF;
		m->pos += can_read;
		return 0xffffffff;
	}
}
