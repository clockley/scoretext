#ifndef NER_H
#define NER_H 1
#include "textutils.h"
#define _GNU_SOURCE
bool initNer(void);
bool freeNer(void);
bool loadText(char*);
bool freeText(void);
char * getWords(void);
#endif