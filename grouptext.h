#ifndef GROUPTEXT_H
#define GROUPTEXT_H 1
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <Judy.h>
struct time {
	int h, m, s;
};
//On 32-bit systems replace uint32_t with uint16_t in the following struct
struct __attribute__((__packed__)) wordData {
	uint32_t count;
	uint32_t syllables;
};
struct wordCxt {
	Pvoid_t PJArray;
	struct wordData * PValue;
	Word_t Bytes;
};
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))
#define var __auto_type

void registerWord(struct wordCxt *cxt, char *w, size_t len, size_t syllaCount);
bool isVowel(int c);
struct time calcReadingTime(size_t words);
struct time calcSpeakingTime(size_t words);
bool matchesLetter(char n, char *a);
bool charEqual(char a, char b);
bool charEqualByRef(char *a, char *b);
ssize_t countSyllables(char *w, size_t s);
size_t trim(char *szWrite);
bool isWebsite(const void *, size_t);
bool isNewSentence(const void *, size_t);
void calcScores(double words, double sentences, double characters, double syllables,  double pollysyllables, double * avg, double *ari, double *fleschKincaid, double *smogScore, double * colemanLiau);
size_t uniqueWords(struct wordCxt * cxt);
void freeWordJudy(struct wordCxt *cxt);
#endif