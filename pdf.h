#ifndef PDF_H
#define PDF_H
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
#include <unistd.h>
#include <stdio.h>
#include <curl/curl.h>
#include "textutils.h"
#include <magic.h>

#define DOCCONVERSIONURL "http://localhost:4567/uploadPDF"

typedef struct MemoryStruct {
	char *memory;
	size_t size;
} MemoryStruct;


bool loadAndReadPDFFile(char *, size_t, char **);
#endif