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
#include "grouptext.h"
#include "pool.h"

static const char *jsonFmt = "{\"S\":[%li,%li,%li,%li,%li,\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%02i:%02i:%02i\",\"%02i:%02i:%02i\",%li]}";

static void * processLine(void *a) {
	struct kreq * req = a;
	size_t words = 0;
	size_t characters = 0;
	size_t sentences = 0;
	size_t syllables = 0;
	size_t pollysyllables = 0;
	size_t paragraph = 0;
	char *b = NULL;

	if (!req->fields) {
		goto endRequest;
	}
	char * doubleNewline = strsep(&req->fields->val, "\r\n\r\n");
	while (doubleNewline) {
		if (*doubleNewline == '\0')
			goto nextDouble;
		else if (!isblank(*doubleNewline))
			++paragraph;
		char *line = strsep(&doubleNewline, "\r\n");
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
							|| word[len - 1] == '.') {
							++sentences;
						}
						goto nextWord;
					}
					ssize_t c = 0;

					if (memmem(word, len, ".", 1)
						|| memmem(word, len, "?", 1)
						|| memmem(word, len, "!", 1)) {
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
			line = strsep(&doubleNewline, "\r\n");
		}
		nextDouble:
		doubleNewline = strsep(&req->fields->val, "\r\n\r\n");
	}

	double wordsOverSentences = ((double)words / (double)sentences);
	double ari = (4.71 * ((double)characters / (double)words)) + ((.5 * wordsOverSentences) - 21.43);
	double fleschKincaid = .39 * wordsOverSentences + (11.8 * ((double)syllables / (double)words) - 15.59);
	double smogScore = ((double)1.0430 * (double)sqrt(pollysyllables * (double)30.0 / (double)sentences)) + 3.1291;
	double colemanLiau = .0588 * ((((double)characters / (double)words)) * 100.0) - (.269 * (((double)sentences / (double)words)) * 100.0) - 15.8;
	double avg = (smogScore + fleschKincaid + ari + colemanLiau) / 4.0;
	struct time rt = calcReadingTime(words), st = calcSpeakingTime(words);

	khttp_write(req, b,
		    asprintf(&b, jsonFmt, words, characters, sentences,
			     syllables, pollysyllables, smogScore, fleschKincaid, ari, colemanLiau, avg, rt.h, rt.m, rt.s, st.h, st.m, st.s, paragraph));
 endRequest:
	khttp_free(req);
	free(b);
	return NULL;
}

int main(void) {
	struct kreq req = {0};
	struct kfcgi *fcgi = NULL;

	ThreadPoolNew();

	if (KCGI_OK != khttp_fcgi_init(&fcgi, NULL, 0, NULL, 0, 0)) {
		return 0;
	}

	while (KCGI_OK == khttp_fcgi_parse(fcgi, &req)) {
		khttp_head(&req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
		khttp_head(&req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
		khttp_body(&req);
		ThreadPoolAddTask(processLine, memmove(calloc(1, sizeof(struct kreq)), &req, sizeof(struct kreq)), true);
	}

	ThreadPoolCancel();

	khttp_fcgi_child_free(fcgi);
}
