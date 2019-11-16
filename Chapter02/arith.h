/* $Id: arith.h 6 2007-01-22 00:45:22Z drhanson $ */
extern int Arith_max(int x, int y); /// extern: declared in .h, defined in other files (.c)
extern int Arith_min(int x, int y); /// Arith_xxx: used as namespace, similar to :: operand in c++
extern int Arith_div(int x, int y);
extern int Arith_mod(int x, int y);
extern int Arith_ceiling(int x, int y);
extern int Arith_floor  (int x, int y);
