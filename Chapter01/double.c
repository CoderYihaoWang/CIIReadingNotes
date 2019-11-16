static char rcsid[] = "$Id: double.c 6 2007-01-22 00:45:22Z drhanson $";

/// this program outputs all continuous double words in the files that are passed as args
/// in default it takes stdin as input file
/// tripled words are counted twice, etc
/// if the doubled words are seperated by \n, the larger line number is shown
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
int linenum;
int getword(FILE *, char *, int);
void doubleword(char *, FILE *);
int main(int argc, char *argv[]) {
	int i;
	for (i = 1; i < argc; i++) {
		FILE *fp = fopen(argv[i], "r");
		if (fp == NULL) {
			fprintf(stderr, "%s: can't open '%s' (%s)\n", /// argv[0] shows the program name
				argv[0], argv[i], strerror(errno)); /// errno: a built-in global variable
			return EXIT_FAILURE;                    /// strerror: shows the string corresponding to errno
		} else {
			doubleword(argv[i], fp); /// name of the file is passed, as there are no other means to get it
			fclose(fp);
		}
	}
	if (argc == 1) doubleword(NULL, stdin);
	return EXIT_SUCCESS;
}
/// the size is passed in, as the array becomes a pointer once passed in as an arg
/// therefore cannot get its size by sizeof buf / sizeof *buf
int getword(FILE *fp, char *buf, int size) { 
	int c;
	c = getc(fp);
	for ( ; c != EOF && isspace(c); c = getc(fp))
		if (c == '\n')
			linenum++;
	{ /// braces to limit i's scope
		int i = 0;
		for ( ; c != EOF && !isspace(c); c = getc(fp))
			if (i < size - 1)
				buf[i++] = tolower(c);
		if (i < size) /// ?? can i be equal to size ??
			buf[i] = '\0';
	} /// i is discarded here
	if (c != EOF) /// if c == EOF: unnecessary to unget
		ungetc(c, fp);
	return buf[0] != '\0'; /// returns 0 if no more words can be read in
}
/// doubleword is called once per file
void doubleword(char *name, FILE *fp) {
	char prev[128], word[128];
	linenum = 1;
	prev[0] = '\0';
	while (getword(fp, word, sizeof word)) {
		if (isalpha(word[0]) && strcmp(prev, word)==0)
			{
				if (name) /// if name is NULL, the program defaults to take stdin as input
					printf("%s:", name);
				printf("%d: %s\n", linenum, word);
			}
		strcpy(prev, word);
	}
}
