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

static const char *jsonFmt = "{\"S\":[%li,%li,%li,%li,%li,\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%02i:%02i:%02i\",\"%02i:%02i:%02i\",%li, %i]}";

static void * processLine(void *a) {
	struct kreq * req = a;
	size_t words = 1;
	size_t characters = 0;
	size_t sentences = 0;
	size_t syllables = 0;
	size_t pollysyllables = 0;
	size_t paragraph = 0;
	struct wordCxt cxt = {0};
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

					if (isWebsite(word, len)) {
						++words;
						++syllables;
						++characters;
						goto nextWord;
					}
					if (isdigit(*word) || *(word) == '\0') {
						if (isNewSentence(word, len)) {
							++sentences;
						}
						goto nextWord;
					}
					if (isNewSentence(word, len)) {
						++sentences;
					}
					len = trim(word);
					if (len == 0)
						goto nextWord;
					ssize_t c = countSyllables(word, len);
					registerWord(&cxt, word, len, c);

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

	double ari, fleschKincaid, smogScore, colemanLiau, avg;
	struct time rt = calcReadingTime(words), st = calcSpeakingTime(words);
	calcScores(words, sentences, characters, syllables, pollysyllables, &avg, &ari, &fleschKincaid, &smogScore, &colemanLiau);
	khttp_write(req, b,
		    asprintf(&b, jsonFmt, words, characters, sentences,
			     syllables, pollysyllables, smogScore, fleschKincaid, ari, colemanLiau, avg, rt.h, rt.m, rt.s, st.h, st.m, st.s, paragraph, uniqueWords(&cxt)));
	freeWordJudy(&cxt);
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
