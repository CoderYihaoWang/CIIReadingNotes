static char rcsid[] = "$Id: arith.c 6 2007-01-22 00:45:22Z drhanson $";
#include "arith.h"

/// this implementation is not generic, it only deals with int
int Arith_max(int x, int y) {
	return x > y ? x : y;
}
int Arith_min(int x, int y) {
	return x > y ? y : x;
}
int Arith_div(int x, int y) {
	if (-13/5 == -2   /// probing whether the machine round a number downwards or to zero
	&&	(x < 0) != (y < 0) && x%y != 0) /// (x<0) != (y<0): of different signs, ==> (x<0)^(y<0)
		return x/y - 1;
	else
		return x/y;
}
int Arith_mod(int x, int y) {
	if (-13/5 == -2
	&&	(x < 0) != (y < 0) && x%y != 0)
		return x%y + y;
	else
		return x%y;
}
int Arith_floor(int x, int y) {
	return Arith_div(x, y);
}
int Arith_ceiling(int x, int y) {
	return Arith_div(x, y) + (x%y != 0);
}