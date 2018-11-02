#ifndef FIXED-POINT_H
#define FIXED-POINT_H

#define p 17
#define q 14
#define f (1 << q)

int int2fixed(int n);
int fixed2int(int x);
int fixedAdd(int x, int y);
int fixedSub(int x, int y);
int fixedAddInt(int x, int n);
int fixedSubInt(int x, int n);
int fixedMul(int x, int y);
int fixedMulInt(int x, int n);
int fixedDiv(int x, int y);
int fixedDivInt(int x, int n);

#endif

