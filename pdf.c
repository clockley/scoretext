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
#include "pdf.h"


static size_t readResponse(void *contents, size_t size, size_t nmemb,
			   void *userp)
{
	size_t realsize = size * nmemb;
	MemoryStruct *mem = (MemoryStruct *) userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
        abort();
	}

	mem->memory = ptr;
	memmove(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

bool loadAndReadPDFFile(char * buf, uint32_t len, char ** ret) {
    struct curl_httppost *formHead = NULL;
	struct curl_httppost *formTail = NULL;
    __thread static MemoryStruct chunk = {.size = 0 };
    chunk.memory = malloc(1);

	magic_t magic = magic_open(MAGIC_MIME_TYPE);

    magic_load(magic, NULL);

    if (strcmp(magic_buffer(magic, buf, len), "application/pdf") != 0) {
        magic_close(magic);
        return false;
    }

	curl_formadd(&formHead, &formTail, CURLFORM_PTRNAME, "uploaded_file",
		     CURLFORM_PTRCONTENTS, buf,
		     CURLFORM_CONTENTSLENGTH, len, CURLFORM_END);

	var curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, readResponse);
	curl_easy_setopt(curl, CURLOPT_URL, DOCCONVERSIONURL);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formHead);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    ret = &chunk.memory;
    return true;
}