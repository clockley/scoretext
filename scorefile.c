/*
    scoretext reads text from a web server and calculates readability and other statistics
    Copyright (C) 2018-2019 Christian Lockley

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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <math.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <kcgi.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <opc/opc.h>
#include "json.h"
#include <magic.h>
#include <unistd.h>
#include <stdio.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

static const char *jsonFmt = "\"S\":[%li,%li,%li,%li,%li,\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%02i:%02i:%02i\",\"%02i:%02i:%02i\",%li]}";

static const char *vowelDigraphs[] = {
	"ai", "ay", "au", "aw", "ea", "ee", "ei", "eu", "ew",
	"air", "ear", "eer", "eir", "eur",
	"ie", "oa", "oi", "oy", "oo", "ou", "ow", "ue", "ui", "uy", "ian",
	"ion", "ial"
};

static inline bool isVowel(int c) {
	c = tolower(c);
	return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

bool matchesLetter(char n, char *a) {
	n = tolower(n);
	for (size_t i = 0; a[i] != '\0'; ++i)
		if (n == a[i])
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

static ssize_t countSyllables(char *w, size_t s) {
	ssize_t num = 0;
	ssize_t dc = 0;
	ssize_t numberOfVowelLetters = 0;

	if (s == 1)
		return 1;

	if (s < 4 && (isVowel(w[0]) || isVowel(w[1]) || isVowel(w[2]))) {
		return 1;
	}

	for (size_t i = 0; i < s; i += 2) {
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

	if (num == 0)
		return s;

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

	if (s > 3 && strcasestr(w + s - 3, "es") != NULL && !matchesLetter(w[s - 3], "cgxsz")) {
		if (!(s > 4 && strcasestr(w + s - 4, "ies"))) {
			--num;
		}
		goto finalCheck;
	}

	if (s > 5 && strcasestr(w + s - 5, "ings") != NULL && (matchesLetter(w[s - 5], "aeiou") || onlyDigraph)) {
		++num;
	}
	if (s > 7 && strcasestr(w + s - 7, "ings") != NULL && charEqual(w[s - 6], w[s - 7])) {
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
	if (s > 3 && w[s - 2] == 'e' && isVowel(w[s - 4]) && !skipFinalECheck) {
		--num;
	}
	if (numberOfVowelLetters == dc) {
		num = dc;
	}
	if (s >= 4 && (tolower(w[s - 3]) == tolower(w[s - 4]))
	    && tolower(w[s - 2]) == 'y') {
		++num;
	}
	if (num < 0)
		num = 1;

	return num;
}

size_t trim(char *szWrite) {
	size_t len = 0;
	const char *szWriteOrig = szWrite;
	char *szLastSpace = szWrite, *szRead = szWrite;
	int bNotSpace;

	while (*szRead != '\0') {
		bNotSpace = !(*szRead <= 64 || *szRead >= 123 || !(*szRead < 90 || *szRead > 97));
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

struct time {
	int h, m, s;
};

struct time calcReadingTime(size_t words) {
	register double sec = ((double)words / 225.0) * 60.0;
	return (struct time) {sec / 3600, floor(fmod(sec, 3600) / 60.0), (int)sec % 60};
}

struct time calcSpeakingTime(size_t words) {
	register double sec = ((double)words / 125.0) * 60.0;
	return (struct time) {sec / 3600, floor(fmod(sec, 3600) / 60.0), (int)sec % 60};
}

static void dumpText(mceTextReader_t *reader, FILE * fp) {
    mce_skip_attributes(reader);
    mce_start_children(reader) {
        mce_start_element(reader, _X("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), _X("t")) {
            mce_skip_attributes(reader);
            mce_start_children(reader) {
                mce_start_text(reader) {
					fputs_unlocked(xmlTextReaderConstValue(reader->reader), fp);
                } mce_end_text(reader);
            } mce_end_children(reader);
        } mce_end_element(reader);
        mce_start_element(reader, _X("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), _X("p")) {
            dumpText(reader, fp);
			fputc_unlocked('\n', fp);
        } mce_end_element(reader);
        mce_start_element(reader, NULL, NULL) {
            dumpText(reader, fp);
        } mce_end_element(reader);
    } mce_end_children(reader);
}

void processFile(struct kreq req) {
	size_t words = 0;
	size_t characters = 0;
	size_t sentences = 0;
	size_t syllables = 0;
	size_t pollysyllables = 0;
	size_t paragraph = 0;
	char *buf = NULL;
	char *b = NULL;
	size_t size = 0;
	mceTextReader_t reader = {0};
	opc_error_t err = {0};

	if (!req.fields)
		goto endRequest;

	opcContainer * container = opcContainerOpenMem(req.fields->val, req.fields->valsz, OPC_OPEN_READ_ONLY, NULL);
	if (!container)
		goto endRequest;

	opcRelation rel = opcRelationFind(container, OPC_PART_INVALID, NULL, "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument");

	if (rel == OPC_RELATION_INVALID)
		goto endRequest;

	opcPart mainPart = opcRelationGetInternalTarget(container, OPC_PART_INVALID, rel);
	if (mainPart != OPC_PART_INVALID)
		if (xmlStrcmp(opcPartGetType(container, mainPart), "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml") != 0)
			goto endRequest;

	if (mainPart == OPC_PART_INVALID)
		goto endRequest;

	err = opcXmlReaderOpen(container, &reader, "/word/document.xml", NULL, NULL, 0);
	if (err != OPC_ERROR_NONE) {
		opcContainerClose(container, OPC_CLOSE_NOW);
		goto endRequest;
	}
	FILE * fm = open_memstream(&buf, &size);

    mce_start_document(&reader) {
        mce_start_element(&reader, NULL, NULL) {
            dumpText(&reader, fm);
        } mce_end_element(&reader);
    } mce_end_document(&reader);
    mceTextReaderCleanup(&reader);
	opcContainerClose(container, OPC_CLOSE_NOW);

	fclose(fm);

	if (buf == NULL) {
		goto endRequest;
	}

	char * tmp = NULL;
	char * tmp2 = json_encode_string(buf);
	khttp_write(&req, tmp, asprintf(&tmp, "{\"C\":%s,", tmp2));
	free(tmp);
	free(tmp2);

	char * doubleNewline = strsep(&buf, "\n\n");
	while (doubleNewline) {
		if (*doubleNewline == '\0')
			goto nextDouble;
		else if (!isblank(*doubleNewline))
			++paragraph;
		char *line = strsep(&doubleNewline, "\n");
		while (line) {
			if (isblank(*line))
				++paragraph;

			char *lastSpace = strrchr(line, ' ');

			if (lastSpace != NULL && !(strstr(lastSpace, ".") || strstr(lastSpace, "?") || strstr(lastSpace, "!")))
				++sentences;

			char *dash = strsep(&line, "-");

			while (dash) {
				char *word = strsep(&dash, " ");

				while (word) {
					size_t len = strlen(word);

					if (memmem(word, len, "://", 3)
						|| memmem(word, len, "www", 3)
						|| memmem(word, len, ".com", 4)
						|| (*word == '&' && *(word + 1) == '\0')) {
						++words;
						++syllables;
						++characters;
						goto nextWord;
					}
					if (isdigit(*word) || *(word) == '\0') {
						if (memmem(word, len, "?", 1)
							|| memmem(word, len, "!", 1)
							|| memmem(word, len, ".", 1)) {
							++sentences;
						}
						goto nextWord;
					}
					ssize_t c = 0;

					if (memmem(word, len, ".", 1)
						|| memmem(word, len, "?", 1)
						|| memmem(word, len, "!", 1)) {
						size_t num;
						len = trim(word);
						if (len == 0)
							goto nextWord;
						c = countSyllables(word, len);
						++sentences;
					} else {
						len = trim(word);
						if (len == 0)
							goto nextWord;
						c = countSyllables(word, len);
					}

					if (c >= 3) {
						++pollysyllables;
					}

					syllables += c;

					characters += len;
					++words;
					nextWord:
					word = strsep(&dash, " ");
				}
				dash = strsep(&line, "-");
			}
			nextLine:
			line = strsep(&doubleNewline, "\n");
		}
		nextDouble:
		doubleNewline = strsep(&buf, "\n\n");
	}

	double wordsOverSentences = ((double)words / (double)sentences);
	double ari = (4.71 * ((double)characters / (double)words)) + ((.5 * wordsOverSentences) - 21.43);
	double fleschKincaid = .39 * wordsOverSentences + (11.8 * ((double)syllables / (double)words) - 15.59);
	double smogScore = ((double)1.0430 * (double)sqrt(pollysyllables * (double)30.0 / (double)sentences)) + 3.1291;
	double colemanLiau = .0588 * ((((double)characters / (double)words)) * 100.0) - (.269 * (((double)sentences / (double)words)) * 100.0) - 15.8;
	double avg = (smogScore + fleschKincaid + ari + colemanLiau) / 4;
	struct time rt = calcReadingTime(words), st = calcSpeakingTime(words);

	khttp_write(&req, b,
		    asprintf(&b, jsonFmt, words, characters, sentences,
			     syllables, pollysyllables, smogScore, fleschKincaid, ari, colemanLiau, avg, rt.h, rt.m, rt.s, st.h, st.m, st.s, paragraph));

 endRequest:
	free(buf);
	free(b);
	khttp_free(&req);
}

int main(void) {
	struct kreq req = { 0 };
	struct kfcgi *fcgi = NULL;
	opcInitLibrary();
	if (KCGI_OK != khttp_fcgi_init(&fcgi, NULL, 0, NULL, 0, 0)) {
		return 0;
	}
	while (KCGI_OK == khttp_fcgi_parse(fcgi, &req)) {
		khttp_head(&req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
		khttp_head(&req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
		khttp_body(&req);
		processFile(req);
	}
	wait(NULL);
	khttp_fcgi_child_free(fcgi);
	opcFreeLibrary();
}