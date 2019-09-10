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
  var writehere = (FILE *)userp;
  var retcode = fwrite(contents, size, nmemb, writehere);
  var nwrite = (curl_off_t)retcode;

  return retcode;
}

bool loadAndReadPDFFile(char * buf, size_t len, char ** ret) {
    struct curl_httppost *formHead = NULL;
	struct curl_httppost *formTail = NULL;

    var magic = magic_open(MAGIC_MIME_TYPE);

    magic_load(magic, NULL);

    if (strcmp(magic_buffer(magic, buf, len), "application/pdf") != 0) {
        magic_close(magic);
        return false;
    }

    var curl = curl_easy_init();

	curl_formadd(&formHead, &formTail, CURLFORM_COPYNAME, "uploaded_file",
		     CURLFORM_PTRCONTENTS, buf,
		     CURLFORM_CONTENTSLENGTH, len, CURLFORM_END);

	size_t sz = 0;
	var f = open_memstream(ret, &sz);

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, readResponse);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)f);
	curl_easy_setopt(curl, CURLOPT_URL, DOCCONVERSIONURL);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formHead);

	curl_easy_perform(curl);

    curl_easy_reset(curl);

    curl_formfree(formHead);

	fclose(f);

    return true;
}