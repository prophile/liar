#include <stdbool.h>

static unsigned long rol ( unsigned long n, unsigned long by )
{
	const unsigned long size = sizeof(n) * 8;
	by %= size;
	unsigned long leftBits = n << by;
	unsigned long rightBits = n >> (size - by);
	return leftBits | rightBits;
}

static bool contains0s ( unsigned short l )
{
	return (((((l ^ 0x7f7f) + 0x7f7f) | l) & 0x8080) != 0x8080);
}

static unsigned long updateHash ( unsigned long old, unsigned short value, unsigned long index )
{
	unsigned long ch64, newHash;
	newHash = old;
	ch64 = (unsigned long)value;
	newHash = rol(newHash, ch64);
	ch64 = rol(ch64, index ^ 0x2196d984f1319003);
	old ^= (ch64 << 17);
	old &= ~ch64;
	newHash ^= old;
	return newHash;
}

unsigned long hash ( const char* str )
{
	unsigned long h = 0x31ca38f6316b54a0;
	unsigned short ch;
	unsigned long index = 0;
	const unsigned short* stru = (const unsigned short*)str;
	while (ch = *stru++)
	{
		if (contains0s(ch))
			break;
		h = updateHash(h, ch, index);
		index += 2;
	}
	if (ch = (unsigned short)*(unsigned char*)stru)
		h = updateHash(h, ch, index);
	return h;
}
