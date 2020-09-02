#include "cbdataio.h"
#include "dataio.h"

uint8_t cbread8(CBFILE *cb, int *err)
{
	uint8_t b;
	size_t read = cb->func.read(cb->ud, &b, sizeof(uint8_t));
	if (read != sizeof(uint8_t)) {
		*err = EOF;
		return 0xFF;
	}
	*err = 0;
	return b;
}

int8_t cbread8s(CBFILE *cb, int *err)
{
	int8_t b;
	size_t read = cb->func.read(cb->ud, &b, sizeof(int8_t));
	if (read != sizeof(int8_t)) {
		*err = EOF;
		return 0;
	}
	*err = 0;
	return b;
}

uint16_t cbread16l(CBFILE *cb, int *err)
{
	uint8_t b[sizeof(uint16_t)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffff;
	}
	*err = 0;
	return readmem16l(b);
}

uint16_t cbread16b(CBFILE *cb, int *err)
{
	uint8_t b[sizeof(uint16_t)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffff;
	}
	*err = 0;
	return readmem16b(b);
}

uint32_t cbread32l(CBFILE *cb, int *err)
{
	uint8_t b[sizeof(uint32_t)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffffffff;
	}
	*err = 0;
	return readmem32l(b);
}

uint32_t cbread32b(CBFILE *cb, int *err)
{
	uint8_t b[sizeof(uint32_t)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffffffff;
	}
	*err = 0;
	return readmem32b(b);
}
