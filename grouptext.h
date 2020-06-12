#ifndef GROUPTEXT_H
#define GROUPTEXT_H 1
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
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
void calcScores(size_t words, size_t sentences, size_t characters, size_t syllables,  size_t pollysyllables, _Decimal64 * avg, _Decimal64 *ari, _Decimal64 *fleschKincaid, _Decimal64 *smogScore, _Decimal64 * colemanLiau);
#endif