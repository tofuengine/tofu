#include "cbdataio.h"
#include "dataio.h"

uint8 cbread8(CBFILE *cb, int *err)
{
	uint8 b;
	size_t read = cb->func.read(cb->ud, &b, sizeof(uint8));
	if (read != sizeof(uint8)) {
		*err = EOF;
		return 0xFF;
	}
	*err = 0;
	return b;
}

int8 cbread8s(CBFILE *cb, int *err)
{
	int8 b;
	size_t read = cb->func.read(cb->ud, &b, sizeof(int8));
	if (read != sizeof(int8)) {
		*err = EOF;
		return 0;
	}
	*err = 0;
	return b;
}

uint16 cbread16l(CBFILE *cb, int *err)
{
	uint8 b[sizeof(uint16)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffff;
	}
	*err = 0;
	return readmem16l(b);
}

uint16 cbread16b(CBFILE *cb, int *err)
{
	uint8 b[sizeof(uint16)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffff;
	}
	*err = 0;
	return readmem16b(b);
}

uint32 cbread32l(CBFILE *cb, int *err)
{
	uint8 b[sizeof(uint32)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffffffff;
	}
	*err = 0;
	return readmem32l(b);
}

uint32 cbread32b(CBFILE *cb, int *err)
{
	uint8 b[sizeof(uint32)];
	size_t read = cb->func.read(cb->ud, b, sizeof(b));
	if (read != sizeof(b)) {
		*err = EOF;
		return 0xffffffff;
	}
	*err = 0;
	return readmem32b(b);
}
