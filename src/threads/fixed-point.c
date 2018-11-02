#include "threads/fixed-point.h"

#define p 17
#define q 14
#define f (1 << q)

int int2fixed(int n)
{
		return n * f;
}

int fixed2int(int x)
{
		if (x > 0) return (x + f / 2) / f;
			return (x - f / 2) / f;	
}

int fixedAdd(int x, int y)
{
		return x + y;
}

int fixedSub(int x, int y)
{
		return x - y;
}

int fixedAddInt(int x, int n)
{
		return x + n * f;
}

int fixedSubInt(int x, int n)
{
		return x - n * f;
}

int fixedMul(int x, int y)
{
		return ((long long int) x) * y / f;
}

int fixedMulInt(int x, int n)
{
		return x * n;
}

int fixedDiv(int x, int y)
{
		return ((long long int) x) * f / y;
}

int fixedDivInt(int x, int n)
{
		return x / n;
}

