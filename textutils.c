/*
    scoretext reads text from a web server and calculates readability and other statistics
    Copyright (C) 2019 Christian Lockley

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "textutils.h"

const char *vowelDigraphs[] = {
	"ai", "ay", "au", "aw", "ea", "ee", "ei", "eu", "ew",
	"air", "ear", "eer", "eir", "eur",
	"ie", "oa", "oi", "oy", "oo", "ou", "ow", "ue", "ui", "uy", "ian",
	"ion", "ial"
};

static __thread unsigned int maxStrSize = 0;

void registerWord(struct wordCxt *cxt, char *w, size_t len, size_t syllaCount) {
	JSLI(cxt->PValue, cxt->PJArray, w);

	if (maxStrSize < len) {
		maxStrSize = len;
	}

	++(cxt->PValue->count);
	cxt->PValue->syllables = syllaCount;
}

void freeWordJudy(struct wordCxt *cxt) {
	JSLFA(cxt->Bytes, cxt->PJArray);
}

size_t uniqueWords(struct wordCxt * cxt) {
	char * tmp = malloc(maxStrSize);
	tmp[0] = '\0';
	JSLF(cxt->PValue, cxt->PJArray, tmp);
	size_t u = 0;
	while ((void*)cxt->PValue != NULL) {
		++u;
		JSLN(cxt->PValue, cxt->PJArray, tmp); 
	}
	free(tmp);
	return u;
}

void calcScores(double words, double sentences, double characters, double syllables,  double pollysyllables, double * avg, double *ari, double *fleschKincaid, double *smogScore, double * colemanLiau) {
	var wordsOverSentences = words / sentences;
	*ari = (4.71 * (characters / words)) + ((.5 * wordsOverSentences) - 21.43);
	*fleschKincaid = .39 * wordsOverSentences + (11.8 * (syllables / words) - 15.59);
	*smogScore = (1.0430 * sqrt(pollysyllables * 30.0 / sentences)) + 3.1291;
	*colemanLiau = .0588 * (((characters / words)) * 100.0) - (.269 * ((sentences / words)) * 100.0) - 15.8;
	*avg = (*smogScore + *fleschKincaid + *ari + *colemanLiau) / 4.0;
}

bool isVowel(int c) {
	c = tolower(c);
	return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

bool isWebsite(const void *word, size_t len) {
	return memmem(word, len, "://", 3)
	|| memmem(word, len, "www", 3)
	|| memmem(word, len, ".com", 4)
	|| (*(char*)word == '&' && *(char*)(word + 1) == '\0');
}

bool isNewSentence(const void *word, size_t len) {
	return memmem(word, len, "?", 1) || memmem(word, len, "!", 1) || memmem(word, len, ".", 1);
}

struct time calcReadingTime(size_t words) {
	register double sec = ((double)words / 225.0) * 60.0;
	return (struct time) {
	sec / 3600, floor(fmod(sec, 3600) / 60.0), (int)sec % 60};
}

struct time calcSpeakingTime(size_t words) {
	register double sec = ((double)words / 125.0) * 60.0;
	return (struct time) {
	sec / 3600, floor(fmod(sec, 3600) / 60.0), (int)sec % 60};
}

bool matchesLetter(char n, char *a) {
	n = tolower(n);
	for (size_t i = 0; a[i] != '\0'; ++i)
		if (tolower(n) == tolower(a[i]))
			return true;
	return false;
}

bool charEqual(char a, char b) {
	if (tolower(a) == tolower(b))
		return true;
	return false;
}

bool charEqualByRef(char *a, char *b) {
	return charEqual(*a, *b);
}

ssize_t countSyllables(char *w, size_t s) {
	ssize_t num = 0;
	ssize_t dc = 0;
	ssize_t numberOfVowelLetters = 0;
	bool hasY = false;

	if (s == 1)
		return 1;

	if (s < 4 && (isVowel(w[0]) || isVowel(w[1]) || isVowel(w[2]))) {
		return 1;
	}

	for (size_t i = 0; i < s; i += 2) {
		if (tolower(w[i]) == 'y' || tolower(w[i])) {
			hasY = true;
		}
		if (isVowel(w[i])) {
			++num;
		}
		if (isVowel(w[i + 1])) {
			++num;
		}
		if ((!isVowel(w[i]) && !isVowel(w[i + 1]))
		    && (tolower(w[i]) == tolower(w[i + 1])) && w[i + 2] != '\0') {
			++dc;
		}
	}

	if (num == 0) {
		if (hasY == false) {
			return s;
		} else {
			return 1;
		}
	}

	numberOfVowelLetters = num;
	bool onlyDigraph = false;

	if (numberOfVowelLetters > 0) {
		for (size_t i = 0; i < ARRAY_SIZE(vowelDigraphs); ++i) {
			if (strcasestr(w, vowelDigraphs[i])) {
				--num;
			}
		}
		if (num == 0) {
			num = numberOfVowelLetters;
		}
		if (num == 1) {
			onlyDigraph = true;
		}
	}

	bool skipFinalECheck = false;

	if (s > 3 && strcasestr(w + s - 3, "le") != NULL) {
		if (isVowel(w[s - 3])) {
			--num;
		}
		skipFinalECheck = true;
	}
	if (s > 4 && strcasestr(w + s - 4, "les") != NULL) {
		if (isVowel(w[s - 4])) {
			--num;
		}
		goto finalCheck;
	}

	if (s > 3 && strcasestr(w + s - 3, "es") != NULL && !matchesLetter(w[s - 4], "cgxsz")) {
		if (!(s > 4 && strcasestr(w + s - 4, "ies"))) {
			--num;
		}
		goto finalCheck;
	}

	if (s > 5 && strcasestr(w + s - 5, "ings") != NULL && (matchesLetter(w[s - 6], "aeiou") || onlyDigraph)) {
		++num;
	}
	if (s > 6 && strcasestr(w + s - 5, "ings") != NULL && charEqual(w[s - 6], w[s - 7])) {
		--num;
		goto finalCheck;
	}
	if (s > 4 && strcasestr(w + s - 4, "ing") != NULL && ((matchesLetter(w[s - 4], "aeiou") || onlyDigraph))
	    && *(strcasestr(w + s - 4, "ing") + 1) == '\0') {
		++num;
	}
	if (s > 6 && strcasestr(w + s - 6, "ing") != NULL && charEqual(w[s - 5], w[s - 6])
	    && *(strcasestr(w + s - 6, "ing") + 1) == '\0') {
		--num;
		goto finalCheck;
	}
	if (s > 3 && strcasestr(w + s - 3, "ed") != NULL && !isVowel(*(strcasestr(w + s - 3, "ed") - 1))
	    && !matchesLetter(*(strcasestr(w + s - 3, "ed") - 1), "td")) {
		--num;
	}
 finalCheck:
	if (s > 3 && w[s - 1] == 'e' && isVowel(w[s - 3]) && !skipFinalECheck) {
		--num;
	}
	if (numberOfVowelLetters == dc) {
		num = dc;
	}
	if (s >= 4 && (tolower(w[s - 3]) == tolower(w[s - 4]))
	    && tolower(w[s - 2]) == 'y') {
		++num;
	}
	if (num <= 0)
		num = 1;

	return num;
}

size_t trim(char *szWrite) {
	size_t len = 0;
	const char *szWriteOrig = szWrite;
	char *szLastSpace = szWrite, *szRead = szWrite;
	int bNotSpace;

	while (*szRead != '\0') {
		bNotSpace = !isspace(*szRead);
		if ((szWrite != szWriteOrig) || bNotSpace) {
			*szWrite = *szRead;
			szWrite++;
			if (bNotSpace) {
				szLastSpace = szWrite;
				++len;
			}
		}
		szRead++;
	}
	*szLastSpace = '\0';
	return len;
}