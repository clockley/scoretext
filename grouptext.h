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
struct time {
	int h, m, s;
};
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))
#define var __auto_type
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
#endif