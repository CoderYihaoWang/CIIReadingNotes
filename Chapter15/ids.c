static char rcsid[] = "$Id: ids.c 6 2007-01-22 00:45:22Z drhanson $";
#include <stdlib.h>
#include <stdio.h>
#include "fmt.h"
#include "str.h"
/// get tokens
int main(int argc, char *argv[]) {
	char line[512];
	static char set[] = "0123456789_"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	/// that is what user defined formattor is for from Fmt
	/// to use it with Str or Text or other data structure
	/// defined by the author, or possibly by the user
	Fmt_register('S', Str_fmt);
	while (fgets(line, sizeof line, stdin) != NULL) {
		int i = 1, j;
		/// i: get the first position in line which is a letter
		/// j: get the first position after i which is not an alnum
		while ((i = Str_upto(line, i, 0, &set[10])) > 0){
			j = Str_many(line, i, 0, set);
			Fmt_print("%S\n", line, i, j);
			i = j;
		}
	}
	return EXIT_SUCCESS;
}
