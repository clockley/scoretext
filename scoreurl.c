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
#include <opc/opc.h>
#include <unistd.h>
#include <sys/wait.h>
#include "json.h"
#include "grouptext.h"
#include "pool.h"
#include "base64.h"

#define SCRAPER_PATH "/usr/local/bin/scoreurl.py"

//https://jineshkj.wordpress.com/2006/12/22/how-to-capture-stdin-stdout-and-stderr-of-child-program/
  
/* since pipes are unidirectional, we need two pipes.
   one for data to flow from parent's stdout to child's
   stdin and the other for child's stdout to flow to
   parent's stdin */
 
#define NUM_PIPES          2
static int outfd[2];
static int infd[2];
#define PARENT_WRITE_PIPE  0
#define PARENT_READ_PIPE   1
 
int pipes[NUM_PIPES][2];
 
/* always in a pipe[], pipe[0] is for read and 
   pipe[1] is for write */
#define READ_FD  0
#define WRITE_FD 1
 
#define PARENT_READ_FD  ( pipes[PARENT_READ_PIPE][READ_FD]   )
#define PARENT_WRITE_FD ( pipes[PARENT_WRITE_PIPE][WRITE_FD] )
 
#define CHILD_READ_FD   ( pipes[PARENT_WRITE_PIPE][READ_FD]  )
#define CHILD_WRITE_FD  ( pipes[PARENT_READ_PIPE][WRITE_FD]  )
 
#include <unistd.h>
#include <stdio.h>
 
/* since pipes are unidirectional, we need two pipes.
   one for data to flow from parent's stdout to child's
   stdin and the other for child's stdout to flow to
   parent's stdin */
 
#define NUM_PIPES          2
 
#define PARENT_WRITE_PIPE  0
#define PARENT_READ_PIPE   1
 
int pipes[NUM_PIPES][2];
 
/* always in a pipe[], pipe[0] is for read and 
   pipe[1] is for write */
#define READ_FD  0
#define WRITE_FD 1
 
#define PARENT_READ_FD  ( pipes[PARENT_READ_PIPE][READ_FD]   )
#define PARENT_WRITE_FD ( pipes[PARENT_WRITE_PIPE][WRITE_FD] )
#define CHILD_READ_FD   ( pipes[PARENT_WRITE_PIPE][READ_FD]  )
#define CHILD_WRITE_FD  ( pipes[PARENT_READ_PIPE][WRITE_FD]  )

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

static const char *jsonFmt = "\"S\":[%li,%li,%li,%li,%li,\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%.1f\",\"%02i:%02i:%02i\",\"%02i:%02i:%02i\",%li]}";
static FILE * fp = NULL;
static pthread_mutex_t libOPCMutex = PTHREAD_MUTEX_INITIALIZER;

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

bool loadAndReadfile(char *val, size_t valsz, char ** buf) {
	pthread_mutex_lock(&libOPCMutex);

	mceTextReader_t reader = {0};
	opc_error_t err = {0};
	size_t size = 0;
	opcContainer * container = opcContainerOpenMem(val, valsz, OPC_OPEN_READ_ONLY, NULL);
	if (!container)
		return false;

	opcRelation rel = opcRelationFind(container, OPC_PART_INVALID, NULL, "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument");

	if (rel == OPC_RELATION_INVALID) {
		opcContainerClose(container, OPC_CLOSE_NOW);
		return false;
	}

	opcPart mainPart = opcRelationGetInternalTarget(container, OPC_PART_INVALID, rel);
	if (mainPart != OPC_PART_INVALID) {
		if (xmlStrcmp(opcPartGetType(container, mainPart), "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml") != 0) {
			opcContainerClose(container, OPC_CLOSE_NOW);
			return false;
		}
	}

	if (mainPart == OPC_PART_INVALID) {
		opcContainerClose(container, OPC_CLOSE_NOW);
		return false;
	}

	err = opcXmlReaderOpen(container, &reader, "/word/document.xml", NULL, NULL, 0);
	if (err != OPC_ERROR_NONE) {
		opcContainerClose(container, OPC_CLOSE_NOW);
		return false;
	}
	FILE * fm = open_memstream(buf, &size);

    mce_start_document(&reader) {
        mce_start_element(&reader, NULL, NULL) {
            dumpText(&reader, fm);
        } mce_end_element(&reader);
    } mce_end_document(&reader);
    mceTextReaderCleanup(&reader);
	opcContainerClose(container, OPC_CLOSE_NOW);

	fclose(fm);

	pthread_mutex_unlock(&libOPCMutex);

	return true;
}

static void * processLine(void *a) {
	struct kreq * req = a;
	size_t words = 0;
	size_t characters = 0;
	size_t sentences = 0;
	size_t syllables = 0;
	size_t pollysyllables = 0;
	size_t paragraph = 0;
	bool b64 = false;
	char *b = NULL;
	char * buf = NULL;
	char * tmp = NULL;
	if (!req->fields) {
		goto endRequest;
	}

	flockfile(fp);

	dprintf(PARENT_WRITE_FD, "%s\n", req->fields->val);

    char *buffer = NULL;
	size_t s = 0;
	FILE * fm = open_memstream(&buf, &s);
	size_t x = 0;
	size_t rsz = 0;
	while ((rsz = getline(&buffer, &x, fp)) != -1) {
		if (memcmp("\0EOF\n", buffer, 5) == 0) {
			fclose(fm);
			break;
		} else if (memcmp("\0B64\n", buffer, 5) == 0) {
			b64 = true;
			fclose(fm);
			break;
		}
		fwrite(buffer, rsz, 1, fm);
	}

	funlockfile(fp);

	free(buffer);

	if (b64) {
		b64 = false;
		size_t decodedLen = 0;
		char * decodedFile = base64_decode(buf, s, &decodedLen);
		if (decodedFile != NULL) {
			char *file = NULL;
			loadAndReadfile(decodedFile, decodedLen, &file);
			if (file == NULL) {
				loadAndReadPDFFile(decodedFile, decodedLen, &file);
			}
			free(decodedFile);
			if (file != NULL) {
				free(buf);
				buf = file;
			}
		}
	}

	char * tmp2 = json_encode_string(buf);
	khttp_write(req, tmp, asprintf(&tmp, "{\"C\":%s,", tmp2));
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

	khttp_write(req, b,
		    asprintf(&b, jsonFmt, words, characters, sentences,
			     syllables, pollysyllables, smogScore, fleschKincaid, ari, colemanLiau, avg, rt.h, rt.m, rt.s, st.h, st.m, st.s, paragraph));
 endRequest:
	khttp_free(req);
	free(buf);
	free(b);
	return NULL;
}

int main(void) {
	struct kreq req = { 0 };
	struct kfcgi *fcgi = NULL;
     
    pipe(pipes[PARENT_READ_PIPE]);
    pipe(pipes[PARENT_WRITE_PIPE]);

    if(!fork()) {
        char *argv[]={SCRAPER_PATH, 0};
        dup2(CHILD_READ_FD, STDIN_FILENO);
        dup2(CHILD_WRITE_FD, STDOUT_FILENO);
        close(CHILD_READ_FD);
        close(CHILD_WRITE_FD);
        close(PARENT_READ_FD);
        close(PARENT_WRITE_FD);   
        execv(argv[0], argv);
    }
	
	fp = fdopen(PARENT_READ_FD, "r");

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
	wait(NULL);
	khttp_fcgi_child_free(fcgi);
	ThreadPoolCancel();
}